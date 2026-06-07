#if defined(_MSC_VER)
#	include <excpt.h>
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string_view>

#include <Windows.h>

// Stock CommonLibOB64 in this project does not expose po3's local
// MagicShaderHitEffect / ProcessLists headers. This file intentionally uses
// the runtime addresses and layout recovered from powerof3's Oblivion
// Remastered Unread Books Glow DLL.
#include "RE/T/TESFullName.h"
#include "RE/T/TESObjectCONT.h"
#include "RE/T/TESObjectREFR.h"

namespace
{
	// ============================================================================
	// LootGlow v0.1.0 release cleanup
	//
	// Proven path:
	//   - Hook TESObjectREFR::LoadGraphics vfunc[0x59].
	//   - Detect containers only.
	//   - Construct MagicShaderHitEffect with the runtime constructor.
	//   - Call effect vfunc +0x38 Init/start.
	//   - Insert effect +0x18 into ProcessLists::magicEffectList at +0x90/+0x98.
	//
	// Release behavior:
	//   - Loaded containers are scanned for the configured gold form.
	//   - Gold-containing containers receive a stacked shader glow.
	//   - The hover/menu path re-checks inventory changes and removes the glow once
	//     gold has been looted.
	//
	// Removed from this cleaned build:
	//   - Runtime TESEffectShader scanner.
	//   - Shader visual sweep slot assignment.
	//   - MagicShaderHitEffect resolver observer hook.
	//   - effect +0x30 and effect +0x80 mutation/instrumentation.
	//   - Detect Life actor-path experiments and hotkeys.
	// ============================================================================

	// Release settings. Defaults keep the runtime log quiet for container-dense
	// areas. Optional INI
	// override paths are searched from the executable directory and parent game
	// directories.
	//
	// Example:
	//   [LootGlow]
	//   GoldFormID=0000000F
	//   ShaderFormID=000C793F
	//   StackCount=8
	//   DebugLogging=0
	constexpr RE::TESFormID kDefaultWinnerShaderFormID = 0x000C793F;
	constexpr RE::TESFormID kDefaultGoldFormID = 0x0000000F;
	constexpr std::uint32_t kDefaultGlowStackCount = 8;
	constexpr std::uint32_t kMaxGlowStackCount = 16;
	constexpr std::uint32_t kMaxTrackedRefs = 1024;

	struct Settings
	{
		RE::TESFormID winnerShaderFormID{ kDefaultWinnerShaderFormID };
		RE::TESFormID goldFormID{ kDefaultGoldFormID };
		std::uint32_t glowStackCount{ kDefaultGlowStackCount };
		bool debugLogging{ false };
	};

	static Settings g_settings{};

	constexpr std::uintptr_t kMagicShaderHitEffectSize = 0xA0;
	constexpr std::uintptr_t kMagicShaderHitEffectBSTempEffectOffset = 0x18;
	constexpr std::uintptr_t kProcessListsMagicEffectHeadItemOffset = 0x90;
	constexpr std::uintptr_t kProcessListsMagicEffectHeadNextOffset = 0x98;
	constexpr std::uintptr_t kBSTempEffectRefCountOffset = 0x08;

	struct TrackedRef
	{
		std::uintptr_t ref{ 0 };
		std::uint32_t refFormID{ 0 };
		std::uint32_t baseFormID{ 0 };
		std::uint32_t loadHits{ 0 };
		std::uintptr_t loadGraphicsNode{ 0 };
		bool applied{ false };
		bool hoverSeen{ false };
		bool lastHasGold{ false };
		std::int32_t lastGoldTotalCount{ -1 };
		std::uint32_t activeStacks{ 0 };
		std::array<void*, kMaxGlowStackCount> effects{};
		char name[96]{};
	};

	struct Counters
	{
		std::uint64_t loadGraphicsHits{ 0 };
		std::uint64_t containerLoadHits{ 0 };
		std::uint64_t uniqueContainers{ 0 };
		std::uint64_t applyAttempts{ 0 };
		std::uint64_t applySuccesses{ 0 };
		std::uint64_t applyFailures{ 0 };
		std::uint64_t removeAttempts{ 0 };
		std::uint64_t removeSuccesses{ 0 };
		std::uint64_t skippedAlreadyApplied{ 0 };
		std::uint64_t skippedAlreadyRemoved{ 0 };
		std::uint64_t shaderResolveFailures{ 0 };
	};

	static std::array<TrackedRef, kMaxTrackedRefs> g_trackedRefs{};
	static Counters g_counters{};
	static RE::TESEffectShader* g_shader{ nullptr };

	std::uint32_t ClampStackCount(std::uint32_t a_value)
	{
		if (a_value == 0) {
			return 1;
		}
		if (a_value > kMaxGlowStackCount) {
			return kMaxGlowStackCount;
		}
		return a_value;
	}

	RE::TESFormID ReadHexFormIDSetting(const char* a_path, const char* a_key, RE::TESFormID a_default)
	{
		char buffer[64]{};
		const auto chars = GetPrivateProfileStringA("LootGlow", a_key, "", buffer, static_cast<DWORD>(sizeof(buffer)), a_path);
		if (chars == 0) {
			return a_default;
		}

		char* end = nullptr;
		const auto parsed = std::strtoul(buffer, &end, 16);
		if (end == buffer) {
			return a_default;
		}

		return static_cast<RE::TESFormID>(parsed);
	}

	bool TryLoadSettingsFromIni(const char* a_path)
	{
		if (GetFileAttributesA(a_path) == INVALID_FILE_ATTRIBUTES) {
			return false;
		}

		g_settings.winnerShaderFormID = ReadHexFormIDSetting(a_path, "ShaderFormID", kDefaultWinnerShaderFormID);
		g_settings.goldFormID = ReadHexFormIDSetting(a_path, "GoldFormID", kDefaultGoldFormID);
		g_settings.glowStackCount = ClampStackCount(static_cast<std::uint32_t>(GetPrivateProfileIntA("LootGlow", "StackCount", kDefaultGlowStackCount, a_path)));
		g_settings.debugLogging = GetPrivateProfileIntA("LootGlow", "DebugLogging", 0, a_path) != 0;

		REX::INFO("LootGlow settings loaded from {}", a_path);
		return true;
	}

	bool TryIniPathWithPrefix(const char* a_prefix, const char* a_suffix)
	{
		if (!a_prefix || !a_suffix) {
			return false;
		}

		char iniPath[MAX_PATH]{};
		const int written = std::snprintf(iniPath, sizeof(iniPath), "%s%s", a_prefix, a_suffix);
		if (written <= 0 || written >= static_cast<int>(sizeof(iniPath))) {
			return false;
		}

		return TryLoadSettingsFromIni(iniPath);
	}

	bool TryLoadSettingsFromExeRelatedPaths()
	{
		char exeDir[MAX_PATH]{};
		const DWORD len = GetModuleFileNameA(nullptr, exeDir, static_cast<DWORD>(sizeof(exeDir)));
		if (len == 0 || len >= sizeof(exeDir)) {
			return false;
		}

		char* slashBack = std::strrchr(exeDir, '\\');
		char* slashForward = std::strrchr(exeDir, '/');
		char* slash = slashBack > slashForward ? slashBack : slashForward;
		if (!slash) {
			return false;
		}

		*(slash + 1) = '\0';

		// Try the executable directory and then walk upward. This covers common
		// Oblivion Remastered layouts such as:
		//   ...\OblivionRemastered\Binaries\Win64\OBSE\Plugins\LootGlow.ini
		//   ...\OblivionRemastered\Data\OBSE\Plugins\LootGlow.ini
		//   ...\OblivionRemastered\OBSE\Plugins\LootGlow.ini
		char current[MAX_PATH]{};
		std::snprintf(current, sizeof(current), "%s", exeDir);

		for (std::uint32_t depth = 0; depth < 6; ++depth) {
			if (TryIniPathWithPrefix(current, "OBSE\\Plugins\\LootGlow.ini")) {
				return true;
			}
			if (TryIniPathWithPrefix(current, "Data\\OBSE\\Plugins\\LootGlow.ini")) {
				return true;
			}

			// Move current to its parent directory while preserving a trailing slash.
			const std::size_t lenCurrent = std::strlen(current);
			if (lenCurrent <= 3) {
				break;
			}

			current[lenCurrent - 1] = '\0';
			char* prevBack = std::strrchr(current, '\\');
			char* prevForward = std::strrchr(current, '/');
			char* prev = prevBack > prevForward ? prevBack : prevForward;
			if (!prev) {
				break;
			}
			*(prev + 1) = '\0';
		}

		return false;
	}

	void LoadSettings()
	{
			// Search multiple realistic locations instead of assuming the
		// process working directory or a single game-root shape.
		if (TryLoadSettingsFromExeRelatedPaths()) {
			return;
		}

		if (TryLoadSettingsFromIni("Data\\OBSE\\Plugins\\LootGlow.ini")) {
			return;
		}

		if (TryLoadSettingsFromIni("OBSE\\Plugins\\LootGlow.ini")) {
			return;
		}

		if (TryLoadSettingsFromIni("LootGlow.ini")) {
			return;
		}

		g_settings = Settings{};
		REX::INFO("LootGlow settings: no LootGlow.ini found; using built-in defaults");
	}

	using AllocFn = void* (*)(std::uint64_t);
	using MagicShaderCtorFn = void* (*)(void*, RE::TESObjectREFR*, RE::TESEffectShader*, float, std::uint32_t);
	using EffectInitFn = bool (*)(void*);
	using ProcessListsGetterFn = void* (*)();
	using FinishMagicShaderHitEffectFn = void (*)(void*, RE::TESObjectREFR*, RE::TESEffectShader*);

	bool LooksPointerish(std::uintptr_t a_value)
	{
		return a_value > 0x0000000000010000ULL &&
		       a_value < 0x0000800000000000ULL &&
		       (a_value % alignof(void*) == 0);
	}

	void CopyName(char (&a_dst)[96], std::string_view a_name)
	{
		for (auto& ch : a_dst) {
			ch = 0;
		}

		const auto count = a_name.size() < 95 ? a_name.size() : 95;
		for (std::size_t i = 0; i < count; ++i) {
			a_dst[i] = a_name[i];
		}
	}

	RE::TESEffectShader* ResolveWinnerShader()
	{
		if (g_shader) {
			return g_shader;
		}

		g_shader = RE::TESForm::LookupByID<RE::TESEffectShader>(g_settings.winnerShaderFormID);
		if (g_shader) {
			if (g_settings.debugLogging) {
				REX::INFO("LootGlow shader resolved: ptr={:016X}, formID={:08X}",
					reinterpret_cast<std::uintptr_t>(g_shader),
					g_settings.winnerShaderFormID);
			}
		} else {
			++g_counters.shaderResolveFailures;
			REX::INFO("LootGlow shader lookup failed/not ready: formID={:08X}", g_settings.winnerShaderFormID);
		}

		return g_shader;
	}

	void* AllocateGameObject(std::uint64_t a_size)
	{
		REL::Relocation<AllocFn> fn{ REL::Offset{ 0x04702CB0 } };  // FUN_144702CB0 / allocator used by po3 path
		return fn(a_size);
	}

	void* ConstructMagicShaderHitEffect(RE::TESObjectREFR* a_ref, RE::TESEffectShader* a_shader, float a_duration)
	{
		void* mem = AllocateGameObject(kMagicShaderHitEffectSize);
		if (!mem) {
			return nullptr;
		}

		REL::Relocation<MagicShaderCtorFn> ctor{ REL::Offset{ 0x068AAE60 } };  // MagicShaderHitEffect ctor / REL ID 0x655A5 in po3 DLL
		return ctor(mem, a_ref, a_shader, a_duration, 0xFFFFFFFFu);
	}

	bool InitMagicShaderHitEffect(void* a_effect)
	{
		if (!a_effect) {
			return false;
		}

		auto** vtable = *reinterpret_cast<void***>(a_effect);
		if (!vtable) {
			return false;
		}

		auto init = reinterpret_cast<EffectInitFn>(vtable[7]);  // vtable +0x38
		return init ? init(a_effect) : false;
	}

	void* GetProcessLists()
	{
		REL::Relocation<ProcessListsGetterFn> fn{ REL::Offset{ 0x0674B050 } };
		return fn();
	}

	void AddRefBSTempEffect(std::uintptr_t a_tempEffect)
	{
		if (!LooksPointerish(a_tempEffect)) {
			return;
		}

#if defined(_MSC_VER)
		__try {
			::InterlockedIncrement(reinterpret_cast<volatile LONG*>(a_tempEffect + kBSTempEffectRefCountOffset));
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		}
#else
		auto* refCount = reinterpret_cast<std::int32_t*>(a_tempEffect + kBSTempEffectRefCountOffset);
		++(*refCount);
#endif
	}

	bool EmplaceFrontMagicEffectListPO3(void* a_processLists, void* a_effect)
	{
		const auto processLists = reinterpret_cast<std::uintptr_t>(a_processLists);
		const auto effect = reinterpret_cast<std::uintptr_t>(a_effect);

		if (!LooksPointerish(processLists) || !LooksPointerish(effect)) {
			REX::INFO("LootGlow list insert failed: invalid processLists/effect pointers. processLists={:016X}, effect={:016X}",
				processLists,
				effect);
			return false;
		}

		const auto newTempEffect = effect + kMagicShaderHitEffectBSTempEffectOffset;
		const auto headItemAddr = processLists + kProcessListsMagicEffectHeadItemOffset;
		const auto headNextAddr = processLists + kProcessListsMagicEffectHeadNextOffset;

#if defined(_MSC_VER)
		__try {
#endif
			auto& headItem = *reinterpret_cast<std::uintptr_t*>(headItemAddr);
			auto& headNext = *reinterpret_cast<std::uintptr_t*>(headNextAddr);

			// Mirrors po3/CommonLibOB64 BSSimpleList<NiPointer<BSTempEffect>>::emplace_front.
			if (headItem != 0) {
				void* nodeMem = AllocateGameObject(0x10);
				const auto node = reinterpret_cast<std::uintptr_t>(nodeMem);
				if (!LooksPointerish(node)) {
					REX::INFO("LootGlow list insert failed: could not allocate list node");
					return false;
				}

				*reinterpret_cast<std::uintptr_t*>(node + 0x00) = headItem;
				headItem = 0;
				*reinterpret_cast<std::uintptr_t*>(node + 0x08) = headNext;
				headNext = 0;
				headNext = node;
			}

			headItem = newTempEffect;
			AddRefBSTempEffect(newTempEffect);
			return true;
#if defined(_MSC_VER)
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			REX::INFO("LootGlow list insert failed: exception while writing ProcessLists magicEffectList");
			return false;
		}
#endif
	}

	void FinishMagicShaderHitEffect(RE::TESObjectREFR* a_ref, RE::TESEffectShader* a_shader)
	{
		void* processLists = GetProcessLists();
		if (!processLists || !a_ref || !a_shader) {
			return;
		}

		REL::Relocation<FinishMagicShaderHitEffectFn> fn{ REL::Offset{ 0x06741E60 } };  // FinishMagicShaderHitEffect / REL ID 0x6425F in po3 DLL
		fn(processLists, a_ref, a_shader);
	}

	TrackedRef* FindTrackedRef(std::uintptr_t a_ref)
	{
		for (auto& entry : g_trackedRefs) {
			if (entry.ref == a_ref) {
				return &entry;
			}
		}
		return nullptr;
	}

	std::uint32_t CountActiveGlowingRefs()
	{
		std::uint32_t count = 0;
		for (const auto& entry : g_trackedRefs) {
			if (entry.ref != 0 && entry.applied) {
				++count;
			}
		}
		return count;
	}

	TrackedRef* TrackContainer(RE::TESObjectREFR* a_ref, RE::TESObjectCONT* a_container, std::string_view a_name, std::uintptr_t a_loadGraphicsNode)
	{
		const auto ref = reinterpret_cast<std::uintptr_t>(a_ref);
		if (!LooksPointerish(ref)) {
			return nullptr;
		}

		if (auto* existing = FindTrackedRef(ref)) {
			++existing->loadHits;
			if (LooksPointerish(a_loadGraphicsNode)) {
				existing->loadGraphicsNode = a_loadGraphicsNode;
			}
			return existing;
		}

		for (auto& entry : g_trackedRefs) {
			if (entry.ref == 0) {
				entry.ref = ref;
				entry.refFormID = a_ref ? a_ref->GetFormID() : 0;
				entry.baseFormID = a_container ? a_container->GetFormID() : 0;
				entry.loadHits = 1;
				entry.loadGraphicsNode = a_loadGraphicsNode;
				entry.applied = false;
				entry.hoverSeen = false;
				entry.lastHasGold = false;
				entry.lastGoldTotalCount = -1;
				entry.activeStacks = 0;
				entry.effects.fill(nullptr);
				CopyName(entry.name, a_name);
				++g_counters.uniqueContainers;


				return &entry;
			}
		}

		REX::INFO("[LootGlow] tracking table full at capacity {}; container ref={:016X} was not tracked", kMaxTrackedRefs, ref);
		return nullptr;
	}


	bool RemoveGlowFromContainer(RE::TESObjectREFR* a_ref, TrackedRef* a_entry, const char* a_reason)
	{
		if (!a_ref || !a_entry) {
			return false;
		}

		if (!a_entry->applied) {
			++g_counters.skippedAlreadyRemoved;
			return true;
		}

		auto* shader = ResolveWinnerShader();
		if (!shader) {
			++g_counters.applyFailures;
			REX::INFO("LootGlow remove failed: winner shader formID={:08X} did not resolve for ref={:016X}, reason={}",
				g_settings.winnerShaderFormID,
				a_entry->ref,
				a_reason ? a_reason : "<none>");
			return false;
		}

		++g_counters.removeAttempts;

		// This is the same recovered ProcessLists finish wrapper that was safe to call
		// before re-applying. It should finish matching MagicShaderHitEffect
		// instances for this ref/shader pair. We also clear the stored stack handles so
		// the cache state agrees with the runtime intent even if the game defers cleanup.
		FinishMagicShaderHitEffect(a_ref, shader);

		const auto oldStacks = a_entry->activeStacks;
		a_entry->applied = false;
		a_entry->activeStacks = 0;
		a_entry->effects.fill(nullptr);
		++g_counters.removeSuccesses;

		if (g_settings.debugLogging) {
			REX::INFO("LootGlow removed/finished winner shader formID={:08X}: ref={:016X}, refForm={:08X}, baseForm={:08X}, previousStacks={}, activeGlowingRefs={}, reason={}, name={}",
				g_settings.winnerShaderFormID,
				a_entry->ref,
				a_entry->refFormID,
				a_entry->baseFormID,
				oldStacks,
				CountActiveGlowingRefs(),
				a_reason ? a_reason : "<none>",
				a_entry->name[0] ? a_entry->name : "<unnamed>");
		}

		return true;
	}

	bool ApplyGlowToContainer(RE::TESObjectREFR* a_ref, TrackedRef* a_entry)
	{
		if (!a_ref || !a_entry) {
			return false;
		}

		if (a_entry->applied) {
			++g_counters.skippedAlreadyApplied;
			return true;
		}

		auto* shader = ResolveWinnerShader();
		if (!shader) {
			++g_counters.applyFailures;
			REX::INFO("LootGlow apply failed: winner shader formID={:08X} did not resolve for ref={:016X}, name={}",
				g_settings.winnerShaderFormID,
				a_entry->ref,
				a_entry->name[0] ? a_entry->name : "<unnamed>");
			return false;
		}

		++g_counters.applyAttempts;

		// Remove any previous instance of this same shader for this ref, then add a clean stack.
		FinishMagicShaderHitEffect(a_ref, shader);

		void* processLists = GetProcessLists();
		if (!processLists) {
			++g_counters.applyFailures;
			REX::INFO("LootGlow apply failed: ProcessLists singleton was null for ref={:016X}", a_entry->ref);
			return false;
		}

		a_entry->activeStacks = 0;
		a_entry->effects.fill(nullptr);

		std::uint32_t stackSuccesses = 0;
		for (std::uint32_t stackIndex = 0; stackIndex < g_settings.glowStackCount; ++stackIndex) {
			void* effect = ConstructMagicShaderHitEffect(a_ref, shader, -1.0f);
			if (!effect) {
				REX::INFO("LootGlow stack {}/{} failed: constructor returned null for ref={:016X}",
					stackIndex + 1,
					g_settings.glowStackCount,
					a_entry->ref);
				continue;
			}

			if (!InitMagicShaderHitEffect(effect)) {
				REX::INFO("LootGlow stack {}/{} failed: Init returned false for ref={:016X}",
					stackIndex + 1,
					g_settings.glowStackCount,
					a_entry->ref);
				continue;
			}

			if (!EmplaceFrontMagicEffectListPO3(processLists, effect)) {
				REX::INFO("LootGlow stack {}/{} failed: po3 list insert failed for ref={:016X}",
					stackIndex + 1,
					g_settings.glowStackCount,
					a_entry->ref);
				continue;
			}

			a_entry->effects[stackIndex] = effect;
			++stackSuccesses;
		}

		if (stackSuccesses == 0) {
			++g_counters.applyFailures;
			REX::INFO("LootGlow apply failed: no stacks inserted for ref={:016X}", a_entry->ref);
			return false;
		}

		a_entry->applied = true;
		a_entry->activeStacks = stackSuccesses;
		++g_counters.applySuccesses;

		if (g_settings.debugLogging) {
			REX::INFO("LootGlow applied winner shader formID={:08X}: ref={:016X}, refForm={:08X}, baseForm={:08X}, loadNode={:016X}, stacks={}/{}, activeGlowingRefs={}, name={}",
				g_settings.winnerShaderFormID,
				a_entry->ref,
				a_entry->refFormID,
				a_entry->baseFormID,
				a_entry->loadGraphicsNode,
				stackSuccesses,
				g_settings.glowStackCount,
				CountActiveGlowingRefs(),
				a_entry->name[0] ? a_entry->name : "<unnamed>");
		}


		return true;
	}
}


namespace RE
{
	class ContainerObject
	{
	public:
		std::int32_t count;
		RE::TESBoundObject* type;
	};
}

namespace LootGlow::GoldSelection
{
	using GetContainerFn = RE::TESContainer* (*)(RE::TESObjectREFR*);
	using SetInfoForRefFn = bool (*)(RE::TESObjectREFR*, bool, bool);

	RE::TESContainer* GetContainer(RE::TESObjectREFR* a_ref)
	{
		if (!a_ref) {
			return nullptr;
		}

		REL::Relocation<GetContainerFn> fn{ REL::ID(405511) };
		return fn(a_ref);
	}

	RE::InventoryChanges* GetInventoryChanges(RE::TESObjectREFR* a_ref)
	{
		if (!a_ref) {
			return nullptr;
		}

#if defined(_MSC_VER)
		__try {
#endif
			auto* containerChanges = reinterpret_cast<RE::ExtraContainerChanges*>(
				a_ref->extra.GetExtraData(RE::EXTRA_DATA_TYPE::ContainerChanges));
			return containerChanges ? containerChanges->changes : nullptr;
#if defined(_MSC_VER)
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			REX::INFO("[LootGlow] GetInventoryChanges exception for ref={:016X}", reinterpret_cast<std::uintptr_t>(a_ref));
			return nullptr;
		}
#endif
	}

	RE::ItemChange* FindItemChange(RE::InventoryChanges* a_changes, RE::TESForm* a_type)
	{
		if (!a_changes || !a_changes->list || !a_type) {
			return nullptr;
		}

		for (auto it = a_changes->list->begin(); it != a_changes->list->end(); ++it) {
			auto* change = *it;
			if (change && change->object == a_type) {
				return change;
			}
		}

		return nullptr;
	}

	bool ContainerContains(RE::TESContainer* a_container, RE::TESForm* a_form)
	{
		if (!a_container || !a_form) {
			return false;
		}

		using ContainsFn = bool (*)(RE::TESContainer*, RE::TESForm*);
		REL::Relocation<ContainsFn> fn{ REL::ID(411806) };
		return fn(a_container, a_form);
	}

	std::uint32_t GetFormID(RE::TESForm* a_form)
	{
		return a_form ? static_cast<std::uint32_t>(a_form->GetFormID()) : 0;
	}


	TrackedRef* EnsureTrackedHoverContainer(RE::TESObjectREFR* a_ref)
	{
		const auto ref = reinterpret_cast<std::uintptr_t>(a_ref);
		if (!LooksPointerish(ref)) {
			return nullptr;
		}

		if (auto* existing = ::FindTrackedRef(ref)) {
			return existing;
		}

		auto* baseObject = a_ref ? a_ref->GetObjectReference() : nullptr;
		auto* objectContainer = baseObject ? baseObject->As<RE::TESObjectCONT>() : nullptr;
		if (!objectContainer) {
			return nullptr;
		}

		std::string_view name{};
		if (auto* rawName = RE::TESFullName::GetFullName(objectContainer)) {
			name = rawName;
		}

		return ::TrackContainer(a_ref, objectContainer, name, 0);
	}

	bool IsExactGoldForm(RE::TESForm* a_form)
	{
		return GetFormID(a_form) == g_settings.goldFormID;
	}

	bool InspectAndApplyGoldGlow(RE::TESObjectREFR* a_ref, const char* a_source)
	{
		if (!a_ref) {
			return false;
		}

		const char* source = (a_source && a_source[0]) ? a_source : "unknown";

		auto* container = GetContainer(a_ref);
		if (!container) {
			return false;  // Keep this branch narrow: containers only.
		}

		auto* changes = GetInventoryChanges(a_ref);

		std::int32_t goldTotalCount = 0;
		bool hasGold = false;

		for (auto it = container->objectList.begin(); it != container->objectList.end(); ++it) {
			auto* obj = reinterpret_cast<RE::ContainerObject*>(*it);
			if (!obj || !obj->type) {
				continue;
			}

			auto* change = FindItemChange(changes, obj->type);
			const auto baseCount = static_cast<std::int32_t>(std::abs(obj->count));
			const auto changeCount = change ? change->count : 0;
			const auto totalCount = baseCount + changeCount;

			if (totalCount > 0 && IsExactGoldForm(obj->type)) {
				hasGold = true;
				goldTotalCount += totalCount;
			}
		}

		if (changes && changes->list) {
			for (auto it = changes->list->begin(); it != changes->list->end(); ++it) {
				auto* change = *it;
				if (!change || !change->object) {
					continue;
				}

				if (ContainerContains(container, change->object)) {
					continue;
				}

				const auto totalCount = change->count;
				if (totalCount > 0 && IsExactGoldForm(change->object)) {
					hasGold = true;
					goldTotalCount += totalCount;
				}
			}
		}

		auto* entry = EnsureTrackedHoverContainer(a_ref);
		if (!entry) {
			if (hasGold && g_settings.debugLogging) {
				REX::INFO("[LootGlow] {}: gold detected on untracked/non-container ref; skipping by design: ref={:016X}, goldTotalCount={}",
					source,
					reinterpret_cast<std::uintptr_t>(a_ref),
					goldTotalCount);
			}
			return false;
		}

		const bool stateChanged = !entry->hoverSeen || entry->lastHasGold != hasGold || entry->lastGoldTotalCount != goldTotalCount;
		if (stateChanged && g_settings.debugLogging) {
			REX::INFO("[LootGlow] {} classification: ref={:016X}, refForm={:08X}, baseForm={:08X}, goldTotalCount={}, hasGold={}, applied={}, name={}",
				source,
				reinterpret_cast<std::uintptr_t>(a_ref),
				GetFormID(a_ref),
				entry->baseFormID,
				goldTotalCount,
				hasGold,
				entry->applied,
				entry->name[0] ? entry->name : "<unnamed>");
		}
		if (stateChanged) {
			entry->hoverSeen = true;
			entry->lastHasGold = hasGold;
			entry->lastGoldTotalCount = goldTotalCount;
		}

		if (!hasGold) {
			if (entry->applied) {
				if (g_settings.debugLogging) {
					REX::INFO("[LootGlow] {}: GOLD REMOVED/ABSENT -> finishing glow: ref={:016X}, refForm={:08X}, goldTotalCount={}",
					source,
					reinterpret_cast<std::uintptr_t>(a_ref),
					GetFormID(a_ref),
					goldTotalCount);
				}
				return ::RemoveGlowFromContainer(a_ref, entry, "hovered container has no positive-count gold");
			}

			return false;
		}

		if (entry->applied) {
			return true;
		}

		if (g_settings.debugLogging) {
			REX::INFO("[LootGlow] {}: GOLD CONFIRMED -> applying glow: ref={:016X}, refForm={:08X}, goldTotalCount={}",
			source,
			reinterpret_cast<std::uintptr_t>(a_ref),
			GetFormID(a_ref),
			goldTotalCount);
		}

		return ::ApplyGlowToContainer(a_ref, entry);
	}


}

struct Hook_SetInfoForRef_GoldSelection
{
	static bool SetInfoForRef(RE::TESObjectREFR* a_ref, bool a_arg2, bool a_arg3)
	{
		const bool result = SetInfoForRefHook(a_ref, a_arg2, a_arg3);
		LootGlow::GoldSelection::InspectAndApplyGoldGlow(a_ref, "hover-update");
		return result;
	}

	static inline REL::THook SetInfoForRefHook{
		"LootGlow_SetInfoForRef_GoldCorrection",
		REL::ID(406425),
		0x63,
		SetInfoForRef
	};
};


struct Hook_LoadGraphics_GoldSelection
{
	static std::uintptr_t LoadGraphicsFunc(RE::TESObjectREFR* a_ref)
	{
		auto result = LoadGraphicsFuncHook(a_ref);
		++g_counters.loadGraphicsHits;

		auto* baseObject = a_ref ? a_ref->GetObjectReference() : nullptr;
		if (!baseObject) {
			return result;
		}

		auto* container = baseObject->As<RE::TESObjectCONT>();
		if (!container) {
			return result;
		}

		++g_counters.containerLoadHits;

		std::string_view name{};
		if (auto* rawName = RE::TESFullName::GetFullName(container)) {
			name = rawName;
		}

		// LoadGraphics tracks loaded container refs and immediately scans for gold.
		// SetInfoForRef remains as the correction/update path for containers whose
		// runtime inventory changes after load.
		if (TrackContainer(a_ref, container, name, result)) {
			LootGlow::GoldSelection::InspectAndApplyGoldGlow(a_ref, "auto-scan");
		}

		return result;
	}

	static inline REL::THookVFT LoadGraphicsFuncHook{
		RE::TESObjectREFR::VTABLE[0],
		0x59,
		LoadGraphicsFunc
	};
};

OBSE_PLUGIN_PRELOAD(const OBSE::PreLoadInterface* a_obse)
{
	OBSE::Init(a_obse, {
		.trampoline = true,
		.trampolineSize = 1 << 10,
	});

	return true;
}

OBSE_PLUGIN_LOAD(const OBSE::LoadInterface* a_obse)
{
	OBSE::Init(a_obse, {
		.trampoline = true,
		.trampolineSize = 1 << 10,
	});

	LoadSettings();

	REX::INFO("========================");
	REX::INFO("LootGlow v0.1.0 initialized");
	REX::INFO("Configured shader={:08X}, goldFormID={:08X}, stacks={}, debugLogging={}",
		g_settings.winnerShaderFormID,
		g_settings.goldFormID,
		g_settings.glowStackCount,
		g_settings.debugLogging);
	REX::INFO("Gold-containing loaded containers will glow automatically; hover/open updates remove glow after gold is looted.");
	REX::INFO("========================");
	return true;
}

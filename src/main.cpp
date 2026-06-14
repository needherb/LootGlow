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
#include <type_traits>
#include <utility>

#include <Windows.h>

// Stock CommonLibOB64 in this project does not expose po3's local
// MagicShaderHitEffect / ProcessLists headers. This file intentionally uses
// the runtime addresses and layout recovered from powerof3's Oblivion
// Remastered Unread Books Glow DLL.
#include "RE/T/TESFullName.h"
#if __has_include("RE/T/TESObjectMISC.h")
#	include "RE/T/TESObjectMISC.h"
#	define LOOTGLOW_HAS_TESOBJECTMISC_HEADER 1
#else
#	define LOOTGLOW_HAS_TESOBJECTMISC_HEADER 0
#endif
#if __has_include("RE/T/TESObjectWEAP.h")
#	include "RE/T/TESObjectWEAP.h"
#	define LOOTGLOW_HAS_TESOBJECTWEAP_HEADER 1
#else
#	define LOOTGLOW_HAS_TESOBJECTWEAP_HEADER 0
#endif
#if __has_include("RE/T/TESObjectARMO.h")
#	include "RE/T/TESObjectARMO.h"
#	define LOOTGLOW_HAS_TESOBJECTARMO_HEADER 1
#else
#	define LOOTGLOW_HAS_TESOBJECTARMO_HEADER 0
#endif
#if __has_include("RE/T/TESObjectBOOK.h")
#	include "RE/T/TESObjectBOOK.h"
#	define LOOTGLOW_HAS_TESOBJECTBOOK_HEADER 1
#else
#	define LOOTGLOW_HAS_TESOBJECTBOOK_HEADER 0
#endif
#if __has_include("RE/A/AlchemyItem.h")
#	include "RE/A/AlchemyItem.h"
#	define LOOTGLOW_HAS_ALCHEMYITEM_HEADER 1
#else
#	define LOOTGLOW_HAS_ALCHEMYITEM_HEADER 0
#endif
#if __has_include("RE/I/IngredientItem.h")
#	include "RE/I/IngredientItem.h"
#	define LOOTGLOW_HAS_INGREDIENTITEM_HEADER 1
#else
#	define LOOTGLOW_HAS_INGREDIENTITEM_HEADER 0
#endif
#if __has_include("RE/T/TESAmmo.h")
#	include "RE/T/TESAmmo.h"
#	define LOOTGLOW_HAS_TESAMMO_HEADER 1
#else
#	define LOOTGLOW_HAS_TESAMMO_HEADER 0
#endif
#if __has_include("RE/T/TESSoulGem.h")
#	include "RE/T/TESSoulGem.h"
#	define LOOTGLOW_HAS_TESSOULGEM_HEADER 1
#else
#	define LOOTGLOW_HAS_TESSOULGEM_HEADER 0
#endif
#if __has_include("RE/T/TESKey.h")
#	include "RE/T/TESKey.h"
#	define LOOTGLOW_HAS_TESKEY_HEADER 1
#else
#	define LOOTGLOW_HAS_TESKEY_HEADER 0
#endif
#if __has_include("RE/T/TESObjectLIGH.h")
#	include "RE/T/TESObjectLIGH.h"
#	define LOOTGLOW_HAS_TESOBJECTLIGH_HEADER 1
#else
#	define LOOTGLOW_HAS_TESOBJECTLIGH_HEADER 0
#endif
#if __has_include("RE/T/TESObjectCLOT.h")
#	include "RE/T/TESObjectCLOT.h"
#	define LOOTGLOW_HAS_TESOBJECTCLOT_HEADER 1
#else
#	define LOOTGLOW_HAS_TESOBJECTCLOT_HEADER 0
#endif
#if __has_include("RE/T/TESObjectAPPA.h")
#	include "RE/T/TESObjectAPPA.h"
#	define LOOTGLOW_HAS_TESOBJECTAPPA_HEADER 1
#else
#	define LOOTGLOW_HAS_TESOBJECTAPPA_HEADER 0
#endif
#if __has_include("RE/T/TESEnchantableForm.h")
#	include "RE/T/TESEnchantableForm.h"
#	define LOOTGLOW_HAS_TESENCHANTABLEFORM_HEADER 1
#else
#	define LOOTGLOW_HAS_TESENCHANTABLEFORM_HEADER 0
#endif
#if __has_include("RE/E/EnchantmentItem.h")
#	include "RE/E/EnchantmentItem.h"
#	define LOOTGLOW_HAS_ENCHANTMENTITEM_HEADER 1
#else
#	define LOOTGLOW_HAS_ENCHANTMENTITEM_HEADER 0
#endif
#if __has_include("RE/M/MagicItem.h")
#	include "RE/M/MagicItem.h"
#	define LOOTGLOW_HAS_MAGICITEM_HEADER 1
#else
#	define LOOTGLOW_HAS_MAGICITEM_HEADER 0
#endif
#include "RE/T/TESObjectCONT.h"
#include "RE/T/TESObjectREFR.h"

namespace
{
	// Runtime addresses/layouts mirror the proven MagicShaderHitEffect path used by
	// the current stable LootGlow release. Keep these narrow and documented because
	// CommonLibOB64 does not currently expose this full effect path directly.
	constexpr RE::TESFormID kDefaultWinnerShaderFormID = 0x000C793F;
	constexpr RE::TESFormID kDefaultGoldFormID = 0x0000000F;
	constexpr RE::TESFormID kDefaultLockpickFormID = 0x0000000A;
	constexpr std::uint32_t kDefaultGlowStackCount = 8;
	constexpr std::uint32_t kDefaultGoldCountThreshold = 100;
	constexpr RE::TESFormID kDefaultHighValueShaderFormID = 0x000C793E;
	constexpr std::uint32_t kDefaultHighValueGlowStackCount = 6;
	constexpr std::uint32_t kDefaultHighValueThreshold = 250;
	constexpr RE::TESFormID kDefaultLockpickShaderFormID = 0x0014A0A2;  // STRP / Soul Trap hit shader; chosen for readable lockpick glow
	constexpr std::uint32_t kDefaultLockpickGlowStackCount = 4;
	constexpr std::uint32_t kMaxGlowStackCount = 16;
	constexpr std::uint32_t kMaxTrackedRefs = 8192;
	constexpr std::uint32_t kMaxDelayedRescans = 256;
	constexpr std::uint64_t kDelayedRescanDelayMs = 500;
	constexpr std::uint64_t kStatsLogIntervalMs = 30000;
	constexpr std::uint64_t kStatsMilestoneInterval = 25;

	// Public INI stays intentionally small. Additional fields below are advanced
	// compatibility/dev toggles and retain their safe built-in defaults unless a
	// user manually adds them to LootGlow.ini.
	struct Settings
	{
		RE::TESFormID winnerShaderFormID{ kDefaultWinnerShaderFormID };
		RE::TESFormID goldFormID{ kDefaultGoldFormID };
		std::uint32_t goldCountThreshold{ kDefaultGoldCountThreshold };
		std::uint32_t goldLogging{ 0 };
		bool lockpickMode{ true };
		RE::TESFormID lockpickFormID{ kDefaultLockpickFormID };
		std::uint32_t lockpickCountThreshold{ 1 };
		RE::TESFormID lockpickShaderFormID{ kDefaultLockpickShaderFormID };
		std::uint32_t lockpickGlowStackCount{ kDefaultLockpickGlowStackCount };
		std::uint32_t lockpickLogging{ 0 };
		std::uint32_t glowStackCount{ kDefaultGlowStackCount };
		bool debugLogging{ false };

		bool highValueMode{ true };
		std::uint32_t highValueLogging{ 0 };
		bool highValueIncludeGold{ false };
		bool highValueAggregateMode{ true };
		std::uint32_t highValueThreshold{ kDefaultHighValueThreshold };
		RE::TESFormID highValueShaderFormID{ kDefaultHighValueShaderFormID };
		std::uint32_t highValueGlowStackCount{ kDefaultHighValueGlowStackCount };
	};

	static Settings g_settings{};

	bool GoldEventLoggingEnabled()
	{
		return g_settings.debugLogging || g_settings.goldLogging >= 1;
	}

	bool GoldSummaryLoggingEnabled()
	{
		return g_settings.debugLogging || g_settings.goldLogging >= 2;
	}

	bool HighValueEventLoggingEnabled()
	{
		return g_settings.debugLogging || g_settings.highValueLogging >= 1;
	}

	bool HighValueSummaryLoggingEnabled()
	{
		return g_settings.debugLogging || g_settings.highValueLogging >= 2;
	}

	bool LockpickEventLoggingEnabled()
	{
		return g_settings.debugLogging || g_settings.lockpickLogging >= 1;
	}

	bool LockpickSummaryLoggingEnabled()
	{
		return g_settings.debugLogging || g_settings.lockpickLogging >= 2;
	}

	constexpr std::uintptr_t kMagicShaderHitEffectSize = 0xA0;
	constexpr std::uintptr_t kMagicShaderHitEffectBSTempEffectOffset = 0x18;
	constexpr std::uintptr_t kProcessListsMagicEffectHeadItemOffset = 0x90;
	constexpr std::uintptr_t kProcessListsMagicEffectHeadNextOffset = 0x98;
	constexpr std::uintptr_t kBSTempEffectRefCountOffset = 0x08;

	struct GlowStackState
	{
		bool applied{ false };
		std::uint32_t activeStacks{ 0 };
		std::array<void*, kMaxGlowStackCount> effects{};
	};

	struct TrackedRef
	{
		std::uintptr_t ref{ 0 };
		std::uint32_t refFormID{ 0 };
		std::uint32_t baseFormID{ 0 };
		std::uint32_t loadHits{ 0 };
		std::uintptr_t loadGraphicsNode{ 0 };
		GlowStackState gold{};
		GlowStackState highValue{};
		GlowStackState lockpick{};
		bool hoverSeen{ false };
		bool lastHasGold{ false };
		bool lastHasHighValue{ false };
		bool lastHasLockpicks{ false };
		std::int32_t lastGoldTotalCount{ -1 };
		std::int32_t lastLockpickTotalCount{ -1 };
		std::uint64_t lastKnownFullValueTotal{ 0 };
		std::uint32_t lastHighestFullValueEach{ 0 };
		std::uint64_t firstSeenMs{ 0 };
		std::uint64_t lastSeenMs{ 0 };
		std::uint64_t lastScanMs{ 0 };
		std::uint64_t lastApplyMs{ 0 };
		std::uint64_t lastRemoveMs{ 0 };
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
		std::uint64_t trackingTableFull{ 0 };
		std::uint64_t pointerReuseResets{ 0 };
		std::uint64_t repairAttempts{ 0 };
		std::uint64_t repairSuccesses{ 0 };
		std::uint64_t delayedRescansQueued{ 0 };
		std::uint64_t delayedRescansProcessed{ 0 };
		std::uint64_t delayedRescanQueueFull{ 0 };
		std::uint64_t delayedRescanExceptions{ 0 };
		std::uint64_t scanCalls{ 0 };
		std::uint64_t scanGoldPresent{ 0 };
		std::uint64_t scanGoldAbsent{ 0 };
	};

	struct DelayedRescan
	{
		std::uintptr_t ref{ 0 };
		std::uint64_t dueMs{ 0 };
		char reason[48]{};
	};

	static std::array<TrackedRef, kMaxTrackedRefs> g_trackedRefs{};
	static std::array<DelayedRescan, kMaxDelayedRescans> g_delayedRescans{};
	static Counters g_counters{};
	static RE::TESEffectShader* g_shader{ nullptr };
	static RE::TESEffectShader* g_highValueShader{ nullptr };
	static RE::TESEffectShader* g_lockpickShader{ nullptr };
	static std::uint64_t g_lastStatsLogMs{ 0 };
	static std::uint64_t g_lastStatsUniqueContainers{ 0 };
	static std::uint64_t g_lastStatsApplySuccesses{ 0 };
	static std::uint64_t g_lastStatsRemoveSuccesses{ 0 };
	static std::uint64_t g_lastStatsDelayedProcessed{ 0 };

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
		g_settings.goldCountThreshold = static_cast<std::uint32_t>(GetPrivateProfileIntA("LootGlow", "GoldCountThreshold", kDefaultGoldCountThreshold, a_path));
		if (g_settings.goldCountThreshold == 0) {
			g_settings.goldCountThreshold = 1;
		}
		g_settings.goldLogging = static_cast<std::uint32_t>(GetPrivateProfileIntA("LootGlow", "GoldLogging", 0, a_path));
		g_settings.lockpickMode = GetPrivateProfileIntA("LootGlow", "LockpickMode", 1, a_path) != 0;
		g_settings.lockpickFormID = ReadHexFormIDSetting(a_path, "LockpickFormID", kDefaultLockpickFormID);
		g_settings.lockpickCountThreshold = static_cast<std::uint32_t>(GetPrivateProfileIntA("LootGlow", "LockpickCountThreshold", 1, a_path));
		if (g_settings.lockpickCountThreshold == 0) {
			g_settings.lockpickCountThreshold = 1;
		}
		g_settings.lockpickShaderFormID = ReadHexFormIDSetting(a_path, "LockpickShaderFormID", kDefaultLockpickShaderFormID);
		g_settings.lockpickGlowStackCount = ClampStackCount(static_cast<std::uint32_t>(GetPrivateProfileIntA("LootGlow", "LockpickStackCount", kDefaultLockpickGlowStackCount, a_path)));
		g_settings.lockpickLogging = static_cast<std::uint32_t>(GetPrivateProfileIntA("LootGlow", "LockpickLogging", 0, a_path));
		g_settings.glowStackCount = ClampStackCount(static_cast<std::uint32_t>(GetPrivateProfileIntA("LootGlow", "StackCount", kDefaultGlowStackCount, a_path)));
		g_settings.debugLogging = GetPrivateProfileIntA("LootGlow", "DebugLogging", 0, a_path) != 0;
		g_settings.highValueMode = GetPrivateProfileIntA("LootGlow", "HighValueMode", 1, a_path) != 0;
		g_settings.highValueLogging = static_cast<std::uint32_t>(GetPrivateProfileIntA("LootGlow", "HighValueLogging", 0, a_path));
		g_settings.highValueIncludeGold = GetPrivateProfileIntA("LootGlow", "HighValueIncludeGold", 0, a_path) != 0;
		g_settings.highValueAggregateMode = GetPrivateProfileIntA("LootGlow", "HighValueAggregateMode", 1, a_path) != 0;
		g_settings.highValueThreshold = static_cast<std::uint32_t>(GetPrivateProfileIntA("LootGlow", "HighValueThreshold", kDefaultHighValueThreshold, a_path));
		g_settings.highValueShaderFormID = ReadHexFormIDSetting(a_path, "HighValueShaderFormID", kDefaultHighValueShaderFormID);
		g_settings.highValueGlowStackCount = ClampStackCount(static_cast<std::uint32_t>(GetPrivateProfileIntA("LootGlow", "HighValueStackCount", kDefaultHighValueGlowStackCount, a_path)));

		if (g_settings.debugLogging) {
			REX::INFO("LootGlow settings loaded from {}", a_path);
		}
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

	std::uint64_t NowMs()
	{
		return static_cast<std::uint64_t>(::GetTickCount64());
	}

	void ClearTrackedRef(TrackedRef& a_entry)
	{
		a_entry = TrackedRef{};
	}

	template <std::size_t N>
	void CopyName(char (&a_dst)[N], std::string_view a_name)
	{
		static_assert(N > 0);
		for (auto& ch : a_dst) {
			ch = 0;
		}

		const auto maxChars = N - 1;
		const auto count = a_name.size() < maxChars ? a_name.size() : maxChars;
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

	RE::TESEffectShader* ResolveHighValueShader()
	{
		if (g_highValueShader) {
			return g_highValueShader;
		}

		g_highValueShader = RE::TESForm::LookupByID<RE::TESEffectShader>(g_settings.highValueShaderFormID);
		if (g_highValueShader) {
			if (g_settings.debugLogging) {
				REX::INFO("LootGlow high-value shader resolved: ptr={:016X}, formID={:08X}",
					reinterpret_cast<std::uintptr_t>(g_highValueShader),
					g_settings.highValueShaderFormID);
			}
		} else {
			++g_counters.shaderResolveFailures;
			REX::INFO("LootGlow high-value shader lookup failed/not ready: formID={:08X}", g_settings.highValueShaderFormID);
		}

		return g_highValueShader;
	}

	RE::TESEffectShader* ResolveLockpickShader()
	{
		if (g_lockpickShader) {
			return g_lockpickShader;
		}

		g_lockpickShader = RE::TESForm::LookupByID<RE::TESEffectShader>(g_settings.lockpickShaderFormID);
		if (g_lockpickShader) {
			if (g_settings.debugLogging) {
				REX::INFO("LootGlow lockpick shader resolved: ptr={:016X}, formID={:08X}",
					reinterpret_cast<std::uintptr_t>(g_lockpickShader),
					g_settings.lockpickShaderFormID);
			}
		} else {
			++g_counters.shaderResolveFailures;
			REX::INFO("LootGlow lockpick shader lookup failed/not ready: formID={:08X}", g_settings.lockpickShaderFormID);
		}

		return g_lockpickShader;
	}

	void* AllocateGameObject(std::uint64_t a_size)
	{
		REL::Relocation<AllocFn> fn{ REL::ID(303328) };  // Address Library 1.512.105.0: 144702CB0 / allocator used by po3 path
		return fn(a_size);
	}

	void* ConstructMagicShaderHitEffect(RE::TESObjectREFR* a_ref, RE::TESEffectShader* a_shader, float a_duration)
	{
		void* mem = AllocateGameObject(kMagicShaderHitEffectSize);
		if (!mem) {
			return nullptr;
		}

		REL::Relocation<MagicShaderCtorFn> ctor{ REL::ID(415141) };  // Address Library 1.512.105.0: 1468AAE60 / MagicShaderHitEffect ctor
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
		REL::Relocation<ProcessListsGetterFn> fn{ REL::ID(410297) };  // Address Library 1.512.105.0: 14674B050
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

		REL::Relocation<FinishMagicShaderHitEffectFn> fn{ REL::ID(410207) };  // Address Library 1.512.105.0: 146741E60 / FinishMagicShaderHitEffect
		fn(processLists, a_ref, a_shader);
	}

	void FinishGlowStackEffects(RE::TESObjectREFR* a_ref, RE::TESEffectShader* a_shader, const GlowStackState& a_state, std::uint32_t a_fallbackStackCount)
	{
		if (!a_ref || !a_shader) {
			return;
		}

		// ProcessLists::FinishMagicShaderHitEffect removes one matching hit effect per call
		// on some shader paths. LootGlow may intentionally stack multiple copies of the
		// same shader for visibility, so removals need to drain the whole category stack.
		auto attempts = a_state.activeStacks > 0 ? a_state.activeStacks : a_fallbackStackCount;
		if (attempts == 0) {
			attempts = 1;
		}
		if (attempts > kMaxGlowStackCount) {
			attempts = kMaxGlowStackCount;
		}

		for (std::uint32_t i = 0; i < attempts; ++i) {
			FinishMagicShaderHitEffect(a_ref, a_shader);
		}
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
			if (entry.ref != 0 &&
				(entry.gold.applied || entry.highValue.applied || entry.lockpick.applied)) {
				++count;
			}
		}
		return count;
	}

	std::uint32_t CountTrackedRefs()
	{
		std::uint32_t count = 0;
		for (const auto& entry : g_trackedRefs) {
			if (entry.ref != 0) {
				++count;
			}
		}
		return count;
	}

	bool MilestoneReached(std::uint64_t a_current, std::uint64_t& a_lastReported)
	{
		if (a_current >= a_lastReported + kStatsMilestoneInterval) {
			a_lastReported = a_current;
			return true;
		}
		return false;
	}

	bool ImportantStabilityReason(const char* a_reason)
	{
		if (!a_reason) {
			return false;
		}
		return std::strcmp(a_reason, "table-full") == 0 ||
			std::strcmp(a_reason, "delayed-queue-full") == 0 ||
			std::strcmp(a_reason, "pointer-reuse-reset") == 0 ||
			std::strcmp(a_reason, "delayed-exception") == 0;
	}

	void MaybeLogTrackingStats(const char* a_reason, bool a_force = false)
	{
		const auto now = NowMs();
		const bool milestone =
			MilestoneReached(g_counters.uniqueContainers, g_lastStatsUniqueContainers) ||
			MilestoneReached(g_counters.applySuccesses, g_lastStatsApplySuccesses) ||
			MilestoneReached(g_counters.removeSuccesses, g_lastStatsRemoveSuccesses) ||
			MilestoneReached(g_counters.delayedRescansProcessed, g_lastStatsDelayedProcessed);

		// Production logs should stay quiet. Stability summaries are useful while
		// debugging, but persistent counters such as delayedRescanExceptions can make
		// normal play logs noisy if they are treated as a permanent warning state.
		// Outside DebugLogging, only forced one-off serious events are reported.
		if (!g_settings.debugLogging) {
			if (!a_force || !ImportantStabilityReason(a_reason)) {
				return;
			}
		}

		if (!a_force && !milestone && g_lastStatsLogMs != 0 && now - g_lastStatsLogMs < kStatsLogIntervalMs) {
			return;
		}

		g_lastStatsLogMs = now;
		REX::INFO("[LootGlow] stability summary reason={}, tracked={}/{}, glowing={}, loadGraphicsHits={}, containerLoadHits={}, scans={} present={} absent={}, applies={}/{}, removes={}, skippedApplied={}, skippedRemoved={}, tableFull={}, queueFull={}, reuseResets={}, repairs={}/{}, delayedQueued={}, delayedProcessed={}, delayedExceptions={}",
			a_reason ? a_reason : "periodic",
			CountTrackedRefs(),
			kMaxTrackedRefs,
			CountActiveGlowingRefs(),
			g_counters.loadGraphicsHits,
			g_counters.containerLoadHits,
			g_counters.scanCalls,
			g_counters.scanGoldPresent,
			g_counters.scanGoldAbsent,
			g_counters.applySuccesses,
			g_counters.applyAttempts,
			g_counters.removeSuccesses,
			g_counters.skippedAlreadyApplied,
			g_counters.skippedAlreadyRemoved,
			g_counters.trackingTableFull,
			g_counters.delayedRescanQueueFull,
			g_counters.pointerReuseResets,
			g_counters.repairSuccesses,
			g_counters.repairAttempts,
			g_counters.delayedRescansQueued,
			g_counters.delayedRescansProcessed,
			g_counters.delayedRescanExceptions);
	}

	void QueueDelayedRescan(RE::TESObjectREFR* a_ref, const char* a_reason)
	{
		const auto ref = reinterpret_cast<std::uintptr_t>(a_ref);
		if (!LooksPointerish(ref)) {
			return;
		}

		const auto due = NowMs() + kDelayedRescanDelayMs;
		DelayedRescan* slot = nullptr;
		for (auto& entry : g_delayedRescans) {
			if (entry.ref == ref) {
				slot = &entry;
				break;
			}
			if (!slot && entry.ref == 0) {
				slot = &entry;
			}
		}

		if (!slot) {
			++g_counters.delayedRescanQueueFull;
			MaybeLogTrackingStats("delayed-queue-full", true);
			if (g_settings.debugLogging) {
				REX::INFO("[LootGlow] delayed rescan queue full; ref={:016X}, reason={}", ref, a_reason ? a_reason : "<none>");
			}
			return;
		}

		slot->ref = ref;
		slot->dueMs = due;
		CopyName(slot->reason, a_reason ? std::string_view(a_reason) : std::string_view("delayed"));
		++g_counters.delayedRescansQueued;
	}

	TrackedRef* TrackContainer(RE::TESObjectREFR* a_ref, RE::TESObjectCONT* a_container, std::string_view a_name, std::uintptr_t a_loadGraphicsNode)
	{
		const auto ref = reinterpret_cast<std::uintptr_t>(a_ref);
		if (!LooksPointerish(ref)) {
			return nullptr;
		}

		const auto now = NowMs();
		const auto currentRefFormID = a_ref ? a_ref->GetFormID() : 0;
		const auto currentBaseFormID = a_container ? a_container->GetFormID() : 0;

		if (auto* existing = FindTrackedRef(ref)) {
			const bool refFormChanged = existing->refFormID != 0 && currentRefFormID != 0 && existing->refFormID != currentRefFormID;
			const bool baseFormChanged = existing->baseFormID != 0 && currentBaseFormID != 0 && existing->baseFormID != currentBaseFormID;
			if (refFormChanged || baseFormChanged) {
				++g_counters.pointerReuseResets;
				MaybeLogTrackingStats("pointer-reuse-reset", true);
				if (g_settings.debugLogging) {
					REX::INFO("[LootGlow] tracked ref pointer appears reused; resetting slot ptr={:016X}, oldRefForm={:08X}, newRefForm={:08X}, oldBaseForm={:08X}, newBaseForm={:08X}",
						ref,
						existing->refFormID,
						currentRefFormID,
						existing->baseFormID,
						currentBaseFormID);
				}
				ClearTrackedRef(*existing);
			} else {
				++existing->loadHits;
				existing->lastSeenMs = now;
				if (LooksPointerish(a_loadGraphicsNode)) {
					existing->loadGraphicsNode = a_loadGraphicsNode;
				}
				return existing;
			}
		}

		for (auto& entry : g_trackedRefs) {
			if (entry.ref == 0) {
				entry.ref = ref;
				entry.refFormID = currentRefFormID;
				entry.baseFormID = currentBaseFormID;
				entry.loadHits = 1;
				entry.loadGraphicsNode = a_loadGraphicsNode;
				entry.gold = GlowStackState{};
				entry.highValue = GlowStackState{};
				entry.lockpick = GlowStackState{};
				entry.hoverSeen = false;
				entry.lastHasGold = false;
				entry.lastHasHighValue = false;
				entry.lastHasLockpicks = false;
				entry.lastGoldTotalCount = -1;
				entry.lastLockpickTotalCount = -1;
				entry.firstSeenMs = now;
				entry.lastSeenMs = now;
				entry.lastScanMs = 0;
				entry.lastApplyMs = 0;
				entry.lastRemoveMs = 0;
				CopyName(entry.name, a_name);
				++g_counters.uniqueContainers;

				MaybeLogTrackingStats("track-new");
				return &entry;
			}
		}

		++g_counters.trackingTableFull;
		REX::INFO("[LootGlow] tracking table full at capacity {}; container ref={:016X}, refForm={:08X}, baseForm={:08X} was not tracked",
			kMaxTrackedRefs,
			ref,
			currentRefFormID,
			currentBaseFormID);
		MaybeLogTrackingStats("table-full");
		return nullptr;
	}


	bool RemoveGlowStack(RE::TESObjectREFR* a_ref, TrackedRef* a_entry, GlowStackState& a_state, RE::TESEffectShader* a_shader, RE::TESFormID a_shaderFormID, const char* a_label, const char* a_reason, bool a_logEvent)
	{
		if (!a_ref || !a_entry) {
			return false;
		}

		if (!a_state.applied) {
			return true;
		}

		if (!a_shader) {
			REX::INFO("LootGlow {} remove failed: shader formID={:08X} did not resolve for ref={:016X}, reason={}",
				a_label,
				a_shaderFormID,
				a_entry->ref,
				a_reason ? a_reason : "<none>");
			return false;
		}

		FinishGlowStackEffects(a_ref, a_shader, a_state, 1);

		const auto previousStacks = a_state.activeStacks;
		a_state = GlowStackState{};
		a_entry->lastRemoveMs = NowMs();

		if (a_logEvent) {
			REX::INFO("[LootGlow] {} glow removed: ref={:016X}, refForm={:08X}, baseForm={:08X}, previousStacks={}, reason={}, name={}",
				a_label,
				a_entry->ref,
				a_entry->refFormID,
				a_entry->baseFormID,
				previousStacks,
				a_reason ? a_reason : "<none>",
				a_entry->name[0] ? a_entry->name : "<unnamed>");
		}

		return true;
	}

	bool ApplyGlowStack(RE::TESObjectREFR* a_ref, TrackedRef* a_entry, GlowStackState& a_state, RE::TESEffectShader* a_shader, RE::TESFormID a_shaderFormID, std::uint32_t a_stackCount, const char* a_label, bool a_logEvent)
	{
		if (!a_ref || !a_entry) {
			return false;
		}

		if (a_state.applied && a_state.activeStacks >= a_stackCount) {
			return true;
		}

		if (!a_shader) {
			REX::INFO("LootGlow {} apply failed: shader formID={:08X} did not resolve for ref={:016X}, name={}",
				a_label,
				a_shaderFormID,
				a_entry->ref,
				a_entry->name[0] ? a_entry->name : "<unnamed>");
			return false;
		}

		FinishGlowStackEffects(a_ref, a_shader, a_state, a_stackCount);

		void* processLists = GetProcessLists();
		if (!processLists) {
			REX::INFO("LootGlow {} apply failed: ProcessLists singleton was null for ref={:016X}", a_label, a_entry->ref);
			return false;
		}

		a_state.activeStacks = 0;
		a_state.effects.fill(nullptr);

		std::uint32_t stackSuccesses = 0;
		for (std::uint32_t stackIndex = 0; stackIndex < a_stackCount; ++stackIndex) {
			void* effect = ConstructMagicShaderHitEffect(a_ref, a_shader, -1.0f);
			if (!effect) {
				REX::INFO("LootGlow {} stack {}/{} failed: constructor returned null for ref={:016X}",
					a_label,
					stackIndex + 1,
					a_stackCount,
					a_entry->ref);
				continue;
			}

			if (!InitMagicShaderHitEffect(effect)) {
				REX::INFO("LootGlow {} stack {}/{} failed: init returned false for ref={:016X}",
					a_label,
					stackIndex + 1,
					a_stackCount,
					a_entry->ref);
				continue;
			}

			if (!EmplaceFrontMagicEffectListPO3(processLists, effect)) {
				REX::INFO("LootGlow {} stack {}/{} failed: list insert failed for ref={:016X}",
					a_label,
					stackIndex + 1,
					a_stackCount,
					a_entry->ref);
				continue;
			}

			a_state.effects[stackSuccesses] = effect;
			++stackSuccesses;
		}

		if (stackSuccesses == 0) {
			REX::INFO("LootGlow {} apply failed: no stacks inserted for ref={:016X}", a_label, a_entry->ref);
			return false;
		}

		a_state.applied = true;
		a_state.activeStacks = stackSuccesses;
		a_entry->lastApplyMs = NowMs();

		if (a_logEvent) {
			REX::INFO("[LootGlow] {} glow applied: ref={:016X}, refForm={:08X}, baseForm={:08X}, shader={:08X}, stacks={}/{}, name={}",
				a_label,
				a_entry->ref,
				a_entry->refFormID,
				a_entry->baseFormID,
				a_shaderFormID,
				stackSuccesses,
				a_stackCount,
				a_entry->name[0] ? a_entry->name : "<unnamed>");
		}

		return true;
	}

	bool RemoveGoldGlowFromContainer(RE::TESObjectREFR* a_ref, TrackedRef* a_entry, const char* a_reason)
	{
		if (!a_entry || !a_entry->gold.applied) {
			++g_counters.skippedAlreadyRemoved;
			return true;
		}

		auto* shader = ResolveWinnerShader();
		if (!shader) {
			++g_counters.applyFailures;
			return false;
		}

		++g_counters.removeAttempts;
		const bool removed = RemoveGlowStack(a_ref, a_entry, a_entry->gold, shader, g_settings.winnerShaderFormID, "gold", a_reason, g_settings.debugLogging);
		if (removed) {
			++g_counters.removeSuccesses;
			MaybeLogTrackingStats("remove-success");
		}
		return removed;
	}

	bool ApplyGoldGlowToContainer(RE::TESObjectREFR* a_ref, TrackedRef* a_entry)
	{
		if (!a_entry) {
			return false;
		}

		bool didRepair = false;
		if (a_entry->gold.applied) {
			if (a_entry->gold.activeStacks >= g_settings.glowStackCount) {
				++g_counters.skippedAlreadyApplied;
				return true;
			}
			didRepair = true;
			++g_counters.repairAttempts;
			RemoveGoldGlowFromContainer(a_ref, a_entry, "repair stale/incomplete gold glow state");
		}

		auto* shader = ResolveWinnerShader();
		if (!shader) {
			++g_counters.applyFailures;
			return false;
		}

		++g_counters.applyAttempts;
		const bool applied = ApplyGlowStack(a_ref, a_entry, a_entry->gold, shader, g_settings.winnerShaderFormID, g_settings.glowStackCount, "gold", g_settings.debugLogging);
		if (applied) {
			if (didRepair) {
				++g_counters.repairSuccesses;
			}
			++g_counters.applySuccesses;
			MaybeLogTrackingStats(didRepair ? "repair-apply-success" : "apply-success");
		} else {
			++g_counters.applyFailures;
		}
		return applied;
	}

	bool RemoveHighValueGlowFromContainer(RE::TESObjectREFR* a_ref, TrackedRef* a_entry, const char* a_reason, bool a_forceFinish = false)
	{
		auto* shader = ResolveHighValueShader();
		if (a_forceFinish && a_ref && a_entry && shader && !a_entry->highValue.applied) {
			// Safety cleanup mirrors the lockpick path. If tracking was reset or
			// otherwise believes the category is inactive while the previous scan
			// still qualified, drain a full high-value stack from this ref.
			GlowStackState fallback{};
			fallback.activeStacks = g_settings.highValueGlowStackCount;
			FinishGlowStackEffects(a_ref, shader, fallback, g_settings.highValueGlowStackCount);
			a_entry->highValue = GlowStackState{};
			a_entry->lastRemoveMs = NowMs();
			if (HighValueEventLoggingEnabled()) {
				REX::INFO("[LootGlow] high-value glow safety cleanup: ref={:016X}, refForm={:08X}, baseForm={:08X}, reason={}, name={}",
					a_entry->ref,
					a_entry->refFormID,
					a_entry->baseFormID,
					a_reason ? a_reason : "<none>",
					a_entry->name[0] ? a_entry->name : "<unnamed>");
			}
			return true;
		}

		return RemoveGlowStack(a_ref, a_entry, a_entry->highValue, shader, g_settings.highValueShaderFormID, "high-value", a_reason, HighValueEventLoggingEnabled());
	}

	bool ApplyHighValueGlowToContainer(RE::TESObjectREFR* a_ref, TrackedRef* a_entry)
	{
		auto* shader = ResolveHighValueShader();
		return ApplyGlowStack(a_ref, a_entry, a_entry->highValue, shader, g_settings.highValueShaderFormID, g_settings.highValueGlowStackCount, "high-value", HighValueEventLoggingEnabled());
	}

	bool RemoveLockpickGlowFromContainer(RE::TESObjectREFR* a_ref, TrackedRef* a_entry, const char* a_reason, bool a_forceFinish = false)
	{
		auto* shader = ResolveLockpickShader();
		if (a_forceFinish && a_ref && a_entry && shader && !a_entry->lockpick.applied) {
			GlowStackState fallback{};
			fallback.activeStacks = g_settings.lockpickGlowStackCount;
			FinishGlowStackEffects(a_ref, shader, fallback, g_settings.lockpickGlowStackCount);
			a_entry->lockpick = GlowStackState{};
			a_entry->lastRemoveMs = NowMs();
			if (LockpickEventLoggingEnabled()) {
				REX::INFO("[LootGlow] lockpick glow safety cleanup: ref={:016X}, refForm={:08X}, baseForm={:08X}, reason={}, name={}",
					a_entry->ref,
					a_entry->refFormID,
					a_entry->baseFormID,
					a_reason ? a_reason : "<none>",
					a_entry->name[0] ? a_entry->name : "<unnamed>");
			}
			return true;
		}

		return RemoveGlowStack(a_ref, a_entry, a_entry->lockpick, shader, g_settings.lockpickShaderFormID, "lockpick", a_reason, LockpickEventLoggingEnabled());
	}

	bool ApplyLockpickGlowToContainer(RE::TESObjectREFR* a_ref, TrackedRef* a_entry)
	{
		auto* shader = ResolveLockpickShader();
		return ApplyGlowStack(a_ref, a_entry, a_entry->lockpick, shader, g_settings.lockpickShaderFormID, g_settings.lockpickGlowStackCount, "lockpick", LockpickEventLoggingEnabled());
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

	struct ItemValueResult
	{
		bool available{ false };
		std::uint32_t value{ 0 };
	};

	template <class, class = void>
	struct HasValueMember : std::false_type
	{};

	template <class T>
	struct HasValueMember<T, std::void_t<decltype(std::declval<T*>()->value)>> : std::true_type
	{};

	template <class T>
	ItemValueResult TryReadValueAs(RE::TESForm* a_form)
	{
		if (!a_form) {
			return {};
		}

		auto* typed = a_form->As<T>();
		if (!typed) {
			return {};
		}

		if constexpr (HasValueMember<T>::value) {
			return { true, static_cast<std::uint32_t>(typed->value) };
		} else {
			return {};
		}
	}

	ItemValueResult TryReadAlchemyItemValue(RE::TESForm* a_form)
	{
#if LOOTGLOW_HAS_ALCHEMYITEM_HEADER
		if (!a_form) {
			return {};
		}

		auto* potion = a_form->As<RE::AlchemyItem>();
		if (!potion) {
			return {};
		}

		// ALCH records do not inherit TESValueForm. Their exposed base/display
		// value lives in AlchemyItemData::costOverride at AlchemyItem + 0x118.
		// This fixes valuable potions being counted as unknown high-value items.
		if (potion->data.costOverride > 0) {
			return { true, static_cast<std::uint32_t>(potion->data.costOverride) };
		}

#if LOOTGLOW_HAS_MAGICITEM_HEADER
#if defined(_MSC_VER)
		__try {
#endif
			auto* magicItem = dynamic_cast<RE::MagicItem*>(potion);
			if (magicItem) {
				const auto cost = magicItem->GetCost(nullptr);
				if (cost > 0.0F) {
					return { true, static_cast<std::uint32_t>(cost + 0.5F) };
				}
			}
#if defined(_MSC_VER)
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return {};
		}
#endif
#endif
#else
		(void)a_form;
#endif

		return {};
	}

	ItemValueResult GetItemBaseGoldValue(RE::TESForm* a_form)
	{
		if (!a_form) {
			return {};
		}

#if LOOTGLOW_HAS_ALCHEMYITEM_HEADER
		if (auto value = TryReadAlchemyItemValue(a_form); value.available) { return value; }
#endif

#if LOOTGLOW_HAS_TESOBJECTMISC_HEADER
		if (auto value = TryReadValueAs<RE::TESObjectMISC>(a_form); value.available) { return value; }
#endif
#if LOOTGLOW_HAS_TESOBJECTWEAP_HEADER
		if (auto value = TryReadValueAs<RE::TESObjectWEAP>(a_form); value.available) { return value; }
#endif
#if LOOTGLOW_HAS_TESOBJECTARMO_HEADER
		if (auto value = TryReadValueAs<RE::TESObjectARMO>(a_form); value.available) { return value; }
#endif
#if LOOTGLOW_HAS_TESOBJECTBOOK_HEADER
		if (auto value = TryReadValueAs<RE::TESObjectBOOK>(a_form); value.available) { return value; }
#endif
#if LOOTGLOW_HAS_ALCHEMYITEM_HEADER
		if (auto value = TryReadValueAs<RE::AlchemyItem>(a_form); value.available) { return value; }
#endif
#if LOOTGLOW_HAS_INGREDIENTITEM_HEADER
		if (auto value = TryReadValueAs<RE::IngredientItem>(a_form); value.available) { return value; }
#endif
#if LOOTGLOW_HAS_TESAMMO_HEADER
		if (auto value = TryReadValueAs<RE::TESAmmo>(a_form); value.available) { return value; }
#endif
#if LOOTGLOW_HAS_TESSOULGEM_HEADER
		if (auto value = TryReadValueAs<RE::TESSoulGem>(a_form); value.available) { return value; }
#endif
#if LOOTGLOW_HAS_TESKEY_HEADER
		if (auto value = TryReadValueAs<RE::TESKey>(a_form); value.available) { return value; }
#endif
#if LOOTGLOW_HAS_TESOBJECTCLOT_HEADER
		if (auto value = TryReadValueAs<RE::TESObjectCLOT>(a_form); value.available) { return value; }
#endif
#if LOOTGLOW_HAS_TESOBJECTAPPA_HEADER
		if (auto value = TryReadValueAs<RE::TESObjectAPPA>(a_form); value.available) { return value; }
#endif
#if LOOTGLOW_HAS_TESOBJECTLIGH_HEADER
		if (auto value = TryReadValueAs<RE::TESObjectLIGH>(a_form); value.available) { return value; }
#endif

		return {};
	}

#if LOOTGLOW_HAS_TESENCHANTABLEFORM_HEADER
	template <class T>
	RE::TESEnchantableForm* TryGetEnchantableFromConcrete(RE::TESForm* a_form)
	{
		if (!a_form) {
			return nullptr;
		}

		auto* typed = a_form->As<T>();
		if (!typed) {
			return nullptr;
		}

		return dynamic_cast<RE::TESEnchantableForm*>(typed);
	}
#endif

	RE::TESEnchantableForm* GetEnchantableFormForValue(RE::TESForm* a_form)
	{
		if (!a_form) {
			return nullptr;
		}

#if LOOTGLOW_HAS_TESENCHANTABLEFORM_HEADER && LOOTGLOW_HAS_TESOBJECTWEAP_HEADER
		if (auto* enchantable = TryGetEnchantableFromConcrete<RE::TESObjectWEAP>(a_form)) { return enchantable; }
#endif
#if LOOTGLOW_HAS_TESENCHANTABLEFORM_HEADER && LOOTGLOW_HAS_TESOBJECTARMO_HEADER
		if (auto* enchantable = TryGetEnchantableFromConcrete<RE::TESObjectARMO>(a_form)) { return enchantable; }
#endif
#if LOOTGLOW_HAS_TESENCHANTABLEFORM_HEADER && LOOTGLOW_HAS_TESOBJECTBOOK_HEADER
		if (auto* enchantable = TryGetEnchantableFromConcrete<RE::TESObjectBOOK>(a_form)) { return enchantable; }
#endif
#if LOOTGLOW_HAS_TESENCHANTABLEFORM_HEADER && LOOTGLOW_HAS_TESOBJECTCLOT_HEADER
		if (auto* enchantable = TryGetEnchantableFromConcrete<RE::TESObjectCLOT>(a_form)) { return enchantable; }
#endif
#if LOOTGLOW_HAS_TESENCHANTABLEFORM_HEADER && LOOTGLOW_HAS_TESAMMO_HEADER
		if (auto* enchantable = TryGetEnchantableFromConcrete<RE::TESAmmo>(a_form)) { return enchantable; }
#endif

		return nullptr;
	}

	float TryGetMagicItemCost(RE::EnchantmentItem* a_enchantment)
	{
#if LOOTGLOW_HAS_MAGICITEM_HEADER && LOOTGLOW_HAS_ENCHANTMENTITEM_HEADER
		if (!a_enchantment) {
			return 0.0F;
		}

#if defined(_MSC_VER)
		__try {
#endif
			auto* magicItem = dynamic_cast<RE::MagicItem*>(a_enchantment);
			if (!magicItem) {
				return 0.0F;
			}
			return magicItem->GetCost(nullptr);
#if defined(_MSC_VER)
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return 0.0F;
		}
#endif
#else
		(void)a_enchantment;
		return 0.0F;
#endif
	}

	std::uint32_t EstimateFullGoldValue(RE::TESForm* a_form)
	{
		const auto baseValue = GetItemBaseGoldValue(a_form);
		if (!baseValue.available) {
			return 0;
		}

		std::uint32_t fullValue = baseValue.value;

#if LOOTGLOW_HAS_TESENCHANTABLEFORM_HEADER && LOOTGLOW_HAS_ENCHANTMENTITEM_HEADER
		auto* enchantable = GetEnchantableFormForValue(a_form);
		if (!enchantable || !enchantable->formEnchanting) {
			return fullValue;
		}

		auto* enchantment = enchantable->formEnchanting;
		const auto magicItemCost = TryGetMagicItemCost(enchantment);
		const auto exposedCost = enchantment->data.costOverride > 0 ? static_cast<float>(enchantment->data.costOverride) : 0.0F;
		const auto rawCost = magicItemCost > 0.0F ? magicItemCost : exposedCost;
		const auto amountOfEnchantment = static_cast<float>(enchantable->amountOfEnchantment);
		const auto enchantValue = static_cast<std::uint32_t>((0.4F * (rawCost + amountOfEnchantment)) + 0.5F);
		fullValue += enchantValue;
#endif

		return fullValue;
	}

	struct ContainerValueSummary
	{
		std::uint64_t knownFullValueTotal{ 0 };
		std::uint32_t highestFullValueEach{ 0 };
		std::uint32_t knownItems{ 0 };
		std::uint32_t unknownItems{ 0 };
	};
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

	bool IsExactLockpickForm(RE::TESForm* a_form)
	{
		return GetFormID(a_form) == g_settings.lockpickFormID;
	}

	std::int32_t NormalizeBaseContainerCount(std::int32_t a_count)
	{
		return a_count < 0 ? -a_count : a_count;
	}

	std::int32_t EffectiveInventoryCount(std::int32_t a_baseCount, std::int32_t a_changeCount, bool a_hasChange)
	{
		// Reconcile editor/base container stacks with runtime InventoryChanges. This
		// is intentionally shared by gold, high-value loot, and lockpicks so all three
		// categories agree about what is currently inside the container.
		//
		// Important discovered edge case: a world chest that spawned with lockpicks
		// produced baseCount=2 and changeCount=0 after the player removed them.
		// Treating that as base+0 kept the lockpick glow alive forever. Therefore,
		// when a base item has a non-negative change entry, treat that entry as the
		// current remaining stack count. Negative entries remain removal deltas.
		if (!a_hasChange) {
			return a_baseCount > 0 ? a_baseCount : 0;
		}

		if (a_changeCount >= 0) {
			return a_changeCount;
		}

		const auto absDelta = -a_changeCount;
		if (absDelta > a_baseCount) {
			return a_baseCount + absDelta;
		}

		const auto total = a_baseCount + a_changeCount;
		return total > 0 ? total : 0;
	}

	std::int32_t EffectiveInventoryCountFromChangeOnly(std::int32_t a_changeCount)
	{
		// Runtime-only InventoryChanges entries represent the current stack count
		// for items that are not part of the base/editor container contents.
		//
		// A negative or zero count here means the runtime stack is gone. Earlier
		// development builds treated negative change-only counts as positive
		// removal deltas, which could make a looted high-value item continue to
		// qualify and leave its glow behind. Base/container items still use
		// EffectiveInventoryCount(), where negative values are reconciled against
		// the base count.
		return a_changeCount > 0 ? a_changeCount : 0;
	}

	bool InspectAndApplyGoldGlow(RE::TESObjectREFR* a_ref, const char* a_source)
	{
		if (!a_ref) {
			return false;
		}

		const char* source = (a_source && a_source[0]) ? a_source : "unknown";
		++g_counters.scanCalls;

		auto* container = GetContainer(a_ref);
		if (!container) {
			return false;  // Keep this branch narrow: containers only.
		}

		auto* changes = GetInventoryChanges(a_ref);

		std::int32_t goldTotalCount = 0;
		std::int32_t lockpickTotalCount = 0;
		bool hasGold = false;
		ContainerValueSummary valueSummary{};

		for (auto it = container->objectList.begin(); it != container->objectList.end(); ++it) {
			auto* obj = reinterpret_cast<RE::ContainerObject*>(*it);
			if (!obj || !obj->type) {
				continue;
			}

			auto* change = FindItemChange(changes, obj->type);
			const auto baseCount = NormalizeBaseContainerCount(obj->count);
			const auto changeCount = change ? change->count : 0;
			const auto totalCount = EffectiveInventoryCount(baseCount, changeCount, change != nullptr);

			const bool isGold = IsExactGoldForm(obj->type);
			const bool isLockpick = IsExactLockpickForm(obj->type);
			if (totalCount > 0 && isGold) {
				goldTotalCount += totalCount;
			}
			if (totalCount > 0 && isLockpick) {
				lockpickTotalCount += totalCount;
			}

			if (g_settings.highValueMode && totalCount > 0 && (!isGold || g_settings.highValueIncludeGold)) {
				const auto fullValueEach = EstimateFullGoldValue(obj->type);
				if (fullValueEach > 0) {
					++valueSummary.knownItems;
					valueSummary.knownFullValueTotal += static_cast<std::uint64_t>(fullValueEach) * static_cast<std::uint64_t>(totalCount);
					if (fullValueEach > valueSummary.highestFullValueEach) {
						valueSummary.highestFullValueEach = fullValueEach;
					}
				} else {
					++valueSummary.unknownItems;
				}
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

				const auto totalCount = EffectiveInventoryCountFromChangeOnly(change->count);
				const bool isGold = IsExactGoldForm(change->object);
				const bool isLockpick = IsExactLockpickForm(change->object);
				if (totalCount > 0 && isGold) {
					goldTotalCount += totalCount;
				}
				if (totalCount > 0 && isLockpick) {
					lockpickTotalCount += totalCount;
				}

				if (g_settings.highValueMode && totalCount > 0 && (!isGold || g_settings.highValueIncludeGold)) {
					const auto fullValueEach = EstimateFullGoldValue(change->object);
					if (fullValueEach > 0) {
						++valueSummary.knownItems;
						valueSummary.knownFullValueTotal += static_cast<std::uint64_t>(fullValueEach) * static_cast<std::uint64_t>(totalCount);
						if (fullValueEach > valueSummary.highestFullValueEach) {
							valueSummary.highestFullValueEach = fullValueEach;
						}
					} else {
						++valueSummary.unknownItems;
					}
				}
			}
		}

		hasGold = goldTotalCount > 0 && static_cast<std::uint32_t>(goldTotalCount) >= g_settings.goldCountThreshold;
		const bool hasLockpicks = lockpickTotalCount > 0 && static_cast<std::uint32_t>(lockpickTotalCount) >= g_settings.lockpickCountThreshold;

		if (hasGold) {
			++g_counters.scanGoldPresent;
		} else {
			++g_counters.scanGoldAbsent;
		}

		// HighValueAggregateMode=1 keeps the original "valuable container overall"
		// behavior. HighValueAggregateMode=0 requires at least one individual item to
		// meet the threshold, avoiding glow from large piles of low-value clutter.
		const bool qualifiesByTotal = g_settings.highValueMode && g_settings.highValueAggregateMode && valueSummary.knownFullValueTotal >= g_settings.highValueThreshold;
		const bool qualifiesBySingle = g_settings.highValueMode && valueSummary.highestFullValueEach >= g_settings.highValueThreshold;
		const bool hasHighValue = qualifiesBySingle || qualifiesByTotal;

		auto* entry = EnsureTrackedHoverContainer(a_ref);
		if (!entry) {
			if ((hasGold || hasLockpicks) && g_settings.debugLogging) {
				REX::INFO("[LootGlow] {}: loot detected on untracked/non-container ref; skipping by design: ref={:016X}, goldTotalCount={}, goldThreshold={}, lockpickTotalCount={}, lockpickThreshold={}",
					source,
					reinterpret_cast<std::uintptr_t>(a_ref),
					goldTotalCount,
					g_settings.goldCountThreshold,
					lockpickTotalCount,
					g_settings.lockpickCountThreshold);
			}
			return false;
		}

		entry->lastScanMs = NowMs();

		const bool previousHasHighValue = entry->hoverSeen && entry->lastHasHighValue;
		const bool previousHasLockpicks = entry->hoverSeen && entry->lastHasLockpicks;

		const bool stateChanged =
			!entry->hoverSeen ||
			entry->lastHasGold != hasGold ||
			entry->lastGoldTotalCount != goldTotalCount ||
			entry->lastHasHighValue != hasHighValue ||
			entry->lastHasLockpicks != hasLockpicks ||
			entry->lastLockpickTotalCount != lockpickTotalCount ||
			entry->lastKnownFullValueTotal != valueSummary.knownFullValueTotal ||
			entry->lastHighestFullValueEach != valueSummary.highestFullValueEach;

		if (stateChanged && GoldSummaryLoggingEnabled()) {
			REX::INFO("[LootGlow] {} classification: ref={:016X}, refForm={:08X}, baseForm={:08X}, goldTotalCount={}, goldThreshold={}, hasGold={}, lockpickTotalCount={}, lockpickThreshold={}, hasLockpicks={}, highValue={}, knownFullValueTotal={}, highestFullValueEach={}, goldApplied={}, highValueApplied={}, lockpickApplied={}, name={}",
				source,
				reinterpret_cast<std::uintptr_t>(a_ref),
				GetFormID(a_ref),
				entry->baseFormID,
				goldTotalCount,
				g_settings.goldCountThreshold,
				hasGold,
				lockpickTotalCount,
				g_settings.lockpickCountThreshold,
				hasLockpicks,
				hasHighValue,
				valueSummary.knownFullValueTotal,
				valueSummary.highestFullValueEach,
				entry->gold.applied,
				entry->highValue.applied,
				entry->lockpick.applied,
				entry->name[0] ? entry->name : "<unnamed>");
		}
		if (stateChanged && g_settings.highValueMode && HighValueSummaryLoggingEnabled()) {
			REX::INFO("[LootGlow] {} high-value summary: ref={:016X}, refForm={:08X}, knownFullValueTotal={}, highestFullValueEach={}, threshold={}, hasHighValue={}, qualifiesByTotal={}, qualifiesBySingle={}, knownItems={}, unknownItems={}, includeGold={}, aggregateMode={}",
				source,
				reinterpret_cast<std::uintptr_t>(a_ref),
				GetFormID(a_ref),
				valueSummary.knownFullValueTotal,
				valueSummary.highestFullValueEach,
				g_settings.highValueThreshold,
				hasHighValue,
				qualifiesByTotal,
				qualifiesBySingle,
				valueSummary.knownItems,
				valueSummary.unknownItems,
				g_settings.highValueIncludeGold,
				g_settings.highValueAggregateMode);
		}

		if (stateChanged && (LockpickEventLoggingEnabled() || LockpickSummaryLoggingEnabled())) {
			REX::INFO("[LootGlow] {} lockpick classification: ref={:016X}, refForm={:08X}, baseForm={:08X}, lockpickTotalCount={}, lockpickThreshold={}, hasLockpicks={}, previousHasLockpicks={}, previousLockpickTotalCount={}, name={}",
				source,
				reinterpret_cast<std::uintptr_t>(a_ref),
				GetFormID(a_ref),
				entry->baseFormID,
				lockpickTotalCount,
				g_settings.lockpickCountThreshold,
				hasLockpicks,
				entry->lastHasLockpicks,
				entry->lastLockpickTotalCount,
				entry->name[0] ? entry->name : "<unnamed>");
		}

		if (stateChanged) {
			entry->hoverSeen = true;
			entry->lastHasGold = hasGold;
			entry->lastHasHighValue = hasHighValue;
			entry->lastHasLockpicks = hasLockpicks;
			entry->lastGoldTotalCount = goldTotalCount;
			entry->lastLockpickTotalCount = lockpickTotalCount;
			entry->lastKnownFullValueTotal = valueSummary.knownFullValueTotal;
			entry->lastHighestFullValueEach = valueSummary.highestFullValueEach;
		}

		bool changedGlow = false;

		if (!hasGold) {
			if (entry->gold.applied) {
				if (GoldEventLoggingEnabled()) {
					REX::INFO("[LootGlow] {}: gold glow removed: ref={:016X}, refForm={:08X}, goldTotalCount={}, goldThreshold={}",
						source,
						reinterpret_cast<std::uintptr_t>(a_ref),
						GetFormID(a_ref),
						goldTotalCount,
						g_settings.goldCountThreshold);
				}
				changedGlow = ::RemoveGoldGlowFromContainer(a_ref, entry, "hovered container gold count below threshold") || changedGlow;
			}
		} else if (!entry->gold.applied || entry->gold.activeStacks < g_settings.glowStackCount) {
			if (GoldEventLoggingEnabled()) {
				REX::INFO("[LootGlow] {}: gold glow applied: ref={:016X}, refForm={:08X}, goldTotalCount={}, goldThreshold={}",
					source,
					reinterpret_cast<std::uintptr_t>(a_ref),
					GetFormID(a_ref),
					goldTotalCount,
					g_settings.goldCountThreshold);
			}
			changedGlow = ::ApplyGoldGlowToContainer(a_ref, entry) || changedGlow;
		}

		if (g_settings.highValueMode) {
			if (!hasHighValue) {
				if (entry->highValue.applied || previousHasHighValue) {
					changedGlow = ::RemoveHighValueGlowFromContainer(a_ref, entry, "container no longer meets high-value threshold", previousHasHighValue) || changedGlow;
				}
			} else if (!entry->highValue.applied || entry->highValue.activeStacks < g_settings.highValueGlowStackCount) {
				changedGlow = ::ApplyHighValueGlowToContainer(a_ref, entry) || changedGlow;
			}
		} else if (entry->highValue.applied) {
			changedGlow = ::RemoveHighValueGlowFromContainer(a_ref, entry, "high-value mode disabled") || changedGlow;
		}

		if (g_settings.lockpickMode) {
			if (!hasLockpicks) {
				if (entry->lockpick.applied || previousHasLockpicks) {
					changedGlow = ::RemoveLockpickGlowFromContainer(a_ref, entry, "container no longer meets lockpick threshold", previousHasLockpicks) || changedGlow;
				}
			} else if (!entry->lockpick.applied || entry->lockpick.activeStacks < g_settings.lockpickGlowStackCount) {
				changedGlow = ::ApplyLockpickGlowToContainer(a_ref, entry) || changedGlow;
			}
		} else if (entry->lockpick.applied) {
			changedGlow = ::RemoveLockpickGlowFromContainer(a_ref, entry, "lockpick mode disabled") || changedGlow;
		}

		return changedGlow || hasGold || hasHighValue || hasLockpicks;
	}

	void ProcessDelayedRescans()
	{
		const auto now = NowMs();
		for (auto& pending : g_delayedRescans) {
			if (pending.ref == 0 || pending.dueMs > now) {
				continue;
			}

			const auto ref = pending.ref;
			char reason[48]{};
			CopyName(reason, pending.reason[0] ? std::string_view(pending.reason) : std::string_view("delayed-rescan"));
			pending = DelayedRescan{};
			++g_counters.delayedRescansProcessed;
			MaybeLogTrackingStats("delayed-processed");

			if (!LooksPointerish(ref)) {
				continue;
			}

			auto* refr = reinterpret_cast<RE::TESObjectREFR*>(ref);
#if defined(_MSC_VER)
			__try {
#endif
				InspectAndApplyGoldGlow(refr, reason);
#if defined(_MSC_VER)
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				++g_counters.delayedRescanExceptions;
				MaybeLogTrackingStats("delayed-exception", true);
				REX::INFO("[LootGlow] delayed rescan skipped after exception; stale ref={:016X}, reason={}", ref, reason);
			}
#endif
		}
	}

}

struct Hook_SetInfoForRef_GoldSelection
{
	static bool SetInfoForRef(RE::TESObjectREFR* a_ref, bool a_arg2, bool a_arg3)
	{
		LootGlow::GoldSelection::ProcessDelayedRescans();
		const bool result = SetInfoForRefHook(a_ref, a_arg2, a_arg3);
		LootGlow::GoldSelection::InspectAndApplyGoldGlow(a_ref, "hover-update");
		QueueDelayedRescan(a_ref, "delayed-hover-update");
		LootGlow::GoldSelection::ProcessDelayedRescans();
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
		LootGlow::GoldSelection::ProcessDelayedRescans();

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
			QueueDelayedRescan(a_ref, "delayed-auto-scan");
		}

		MaybeLogTrackingStats("loadgraphics");

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

	REX::INFO("LootGlow v0.3.2 V77D production initialized");
	REX::INFO("Gold glow: threshold={} gold", g_settings.goldCountThreshold);
	REX::INFO("High-value glow: enabled={}, threshold={} value, aggregateMode={}", g_settings.highValueMode, g_settings.highValueThreshold, g_settings.highValueAggregateMode);
	REX::INFO("Lockpick glow: enabled={}, formID={:08X}, threshold={}, shader={:08X}, stacks={}",
		g_settings.lockpickMode,
		g_settings.lockpickFormID,
		g_settings.lockpickCountThreshold,
		g_settings.lockpickShaderFormID,
		g_settings.lockpickGlowStackCount);
	if (g_settings.debugLogging) {
		REX::INFO("Advanced settings: goldShader={:08X}, goldStacks={}, highValueShader={:08X}, highValueStacks={}, includeGold={}, aggregateMode={}, goldLogging={}, highValueLogging={}, lockpickMode={}, lockpickFormID={:08X}, lockpickThreshold={}, lockpickShader={:08X}, lockpickStacks={}, lockpickLogging={}, debugLogging={}",
			g_settings.winnerShaderFormID,
			g_settings.glowStackCount,
			g_settings.highValueShaderFormID,
			g_settings.highValueGlowStackCount,
			g_settings.highValueIncludeGold,
			g_settings.highValueAggregateMode,
			g_settings.goldLogging,
			g_settings.highValueLogging,
			g_settings.lockpickMode,
			g_settings.lockpickFormID,
			g_settings.lockpickCountThreshold,
			g_settings.lockpickShaderFormID,
			g_settings.lockpickGlowStackCount,
			g_settings.lockpickLogging,
			g_settings.debugLogging);
	}
	if (g_settings.debugLogging) {
		MaybeLogTrackingStats("startup", true);
	}
	return true;
}

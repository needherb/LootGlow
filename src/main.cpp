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
	// v0.4.0 release: four-tier loot highlighting with configurable Insane secondary shader.
	//
	// This file intentionally drops the old independent gold/high-value/lockpick
	// visual state machine. A scan computes one desired visual plan, then all owned
	// visuals are rebuilt only when that plan changes. The value tier remains
	// exclusive. In this cleanup build, lockpick glow is standalone-only because
	// STRP does not coexist reliably with the value-tier shaders in v0.4.0.
	enum class LootTier : std::uint8_t
	{
		None = 0,
		Low,
		Medium,
		High,
		Insane
	};

	constexpr RE::TESFormID kDefaultGoldFormID = 0x0000000F;
	constexpr RE::TESFormID kDefaultLockpickFormID = 0x0000000A;
	constexpr RE::TESFormID kDefaultLowTierShaderFormID = 0x000C7939;      // White low-tier default
	constexpr RE::TESFormID kDefaultMediumTierShaderFormID = 0x000C793E;   // Green medium-tier default
	constexpr RE::TESFormID kDefaultHighTierShaderFormID = 0x0008B95F;     // Blue high-tier default
	constexpr RE::TESFormID kDefaultInsaneTierShaderFormID = 0x000C793F;   // Red/pink + gold primary insane-tier default
	constexpr RE::TESFormID kDefaultInsaneTierSecondaryShaderFormID = 0x0018B576;  // Default extra Insane accent shader
	constexpr RE::TESFormID kDefaultLockpickShaderFormID = 0x0014A0A2;    // STRP / purple Soul Trap hit effect
	constexpr std::uint32_t kDefaultLowTierThreshold = 25;
	constexpr std::uint32_t kDefaultMediumTierThreshold = 100;
	constexpr std::uint32_t kDefaultHighTierThreshold = 250;
	constexpr std::uint32_t kDefaultInsaneTierThreshold = 500;
	constexpr std::uint32_t kDefaultLowTierStackCount = 4;
	constexpr std::uint32_t kDefaultMediumTierStackCount = 1;
	constexpr std::uint32_t kDefaultHighTierStackCount = 1;
	constexpr std::uint32_t kDefaultInsaneTierStackCount = 1;
	constexpr std::uint32_t kDefaultInsaneTierSecondaryStackCount = 1;
	constexpr std::uint32_t kDefaultLockpickStackCount = 4;
	constexpr std::uint32_t kMaxGlowStackCount = 16;
	constexpr std::uint32_t kMaxTrackedRefs = 8192;
	constexpr std::uint64_t kStatsLogIntervalMs = 30000;
	constexpr std::uint64_t kStatsMilestoneInterval = 25;
	constexpr std::uint64_t kLockpickRefreshIntervalMs = 1200;
	constexpr std::uint64_t kLoadVisualRefreshCooldownMs = 12000;

	constexpr std::uintptr_t kMagicShaderHitEffectSize = 0xA0;
	constexpr std::uintptr_t kMagicShaderHitEffectBSTempEffectOffset = 0x18;
	constexpr std::uintptr_t kProcessListsMagicEffectHeadItemOffset = 0x90;
	constexpr std::uintptr_t kProcessListsMagicEffectHeadNextOffset = 0x98;
	constexpr std::uintptr_t kBSTempEffectRefCountOffset = 0x08;

	struct Settings
	{
		RE::TESFormID goldFormID{ kDefaultGoldFormID };
		bool valueAggregateMode{ true };
		bool debugLogging{ false };
		bool visualRefreshMode{ true };
		bool lockpickMode{ true };
		RE::TESFormID lockpickFormID{ kDefaultLockpickFormID };
		RE::TESFormID lockpickShaderFormID{ kDefaultLockpickShaderFormID };
		std::uint32_t lockpickStackCount{ kDefaultLockpickStackCount };

		bool lowTierEnabled{ true };
		std::uint32_t lowTierThreshold{ kDefaultLowTierThreshold };
		RE::TESFormID lowTierShaderFormID{ kDefaultLowTierShaderFormID };
		std::uint32_t lowTierStackCount{ kDefaultLowTierStackCount };

		bool mediumTierEnabled{ true };
		std::uint32_t mediumTierThreshold{ kDefaultMediumTierThreshold };
		RE::TESFormID mediumTierShaderFormID{ kDefaultMediumTierShaderFormID };
		std::uint32_t mediumTierStackCount{ kDefaultMediumTierStackCount };

		bool highTierEnabled{ true };
		std::uint32_t highTierThreshold{ kDefaultHighTierThreshold };
		RE::TESFormID highTierShaderFormID{ kDefaultHighTierShaderFormID };
		std::uint32_t highTierStackCount{ kDefaultHighTierStackCount };

		bool insaneTierEnabled{ true };
		std::uint32_t insaneTierThreshold{ kDefaultInsaneTierThreshold };
		RE::TESFormID insaneTierShaderFormID{ kDefaultInsaneTierShaderFormID };
		std::uint32_t insaneTierStackCount{ kDefaultInsaneTierStackCount };
		bool insaneTierSecondaryEnabled{ true };
		RE::TESFormID insaneTierSecondaryShaderFormID{ kDefaultInsaneTierSecondaryShaderFormID };
		std::uint32_t insaneTierSecondaryStackCount{ kDefaultInsaneTierSecondaryStackCount };
	};

	struct GlowStackState
	{
		bool applied{ false };
		std::uint32_t activeStacks{ 0 };
	};

	struct DesiredVisualPlan
	{
		LootTier valueTier{ LootTier::None };
		bool lockpickGlow{ false };
	};

	bool SamePlan(const DesiredVisualPlan& a_lhs, const DesiredVisualPlan& a_rhs)
	{
		return a_lhs.valueTier == a_rhs.valueTier && a_lhs.lockpickGlow == a_rhs.lockpickGlow;
	}

	struct TrackedRef
	{
		std::uintptr_t ref{ 0 };
		std::uint32_t refFormID{ 0 };
		std::uint32_t baseFormID{ 0 };
		GlowStackState valueGlow{};
		GlowStackState insaneSecondaryGlow{};
		GlowStackState lockpickGlow{};
		LootTier appliedTier{ LootTier::None };
		bool appliedLockpickGlow{ false };
		LootTier lastDesiredTier{ LootTier::None };
		bool lastDesiredLockpickGlow{ false };
		bool hasScanned{ false };
		std::uint64_t lastKnownValueTotal{ 0 };
		std::uint32_t lastHighestValueCandidate{ 0 };
		std::int32_t lastGoldTotalCount{ -1 };
		std::int32_t lastLockpickTotalCount{ -1 };
		std::uint64_t lastApplyMs{ 0 };
		std::uint64_t lastLoadRefreshMs{ 0 };
		char name[96]{};
	};

	struct Counters
	{
		std::uint64_t loadGraphicsHits{ 0 };
		std::uint64_t containerLoadHits{ 0 };
		std::uint64_t uniqueContainers{ 0 };
		std::uint64_t scanCalls{ 0 };
		std::uint64_t tierNone{ 0 };
		std::uint64_t tierLow{ 0 };
		std::uint64_t tierMedium{ 0 };
		std::uint64_t tierHigh{ 0 };
		std::uint64_t tierInsane{ 0 };
		std::uint64_t lockpickDesired{ 0 };
		std::uint64_t rebuildAttempts{ 0 };
		std::uint64_t applyAttempts{ 0 };
		std::uint64_t applySuccesses{ 0 };
		std::uint64_t applyFailures{ 0 };
		std::uint64_t removeAttempts{ 0 };
		std::uint64_t removeSuccesses{ 0 };
		std::uint64_t skippedNoChange{ 0 };
		std::uint64_t visualRefreshes{ 0 };
		std::uint64_t shaderResolveFailures{ 0 };
		std::uint64_t trackingTableFull{ 0 };
		std::uint64_t pointerReuseResets{ 0 };
	};


	static Settings g_settings{};
	static std::array<TrackedRef, kMaxTrackedRefs> g_trackedRefs{};
	static Counters g_counters{};
	static RE::TESEffectShader* g_lowTierShader{ nullptr };
	static RE::TESEffectShader* g_mediumTierShader{ nullptr };
	static RE::TESEffectShader* g_highTierShader{ nullptr };
	static RE::TESEffectShader* g_insaneTierShader{ nullptr };
	static RE::TESEffectShader* g_insaneTierSecondaryShader{ nullptr };
	static RE::TESEffectShader* g_lockpickShader{ nullptr };
	static std::uint64_t g_lastStatsLogMs{ 0 };
	static std::uint64_t g_lastStatsUniqueContainers{ 0 };
	static std::uint64_t g_lastStatsApplySuccesses{ 0 };
	static std::uint64_t g_lastStatsRemoveSuccesses{ 0 };

	const char* TierName(LootTier a_tier)
	{
		switch (a_tier) {
		case LootTier::Low:
			return "Low";
		case LootTier::Medium:
			return "Medium";
		case LootTier::High:
			return "High";
		case LootTier::Insane:
			return "Insane";
		default:
			return "None";
		}
	}

	std::uint32_t ClampStackCount(std::uint32_t a_value)
	{
		if (a_value == 0) {
			return 1;
		}
		return a_value > kMaxGlowStackCount ? kMaxGlowStackCount : a_value;
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
		return end == buffer ? a_default : static_cast<RE::TESFormID>(parsed);
	}

	std::uint32_t ReadPositiveIntSetting(const char* a_path, const char* a_key, std::uint32_t a_default)
	{
		const auto value = static_cast<std::uint32_t>(GetPrivateProfileIntA("LootGlow", a_key, static_cast<int>(a_default), a_path));
		return value == 0 ? 1 : value;
	}

	bool TryLoadSettingsFromIni(const char* a_path)
	{
		if (GetFileAttributesA(a_path) == INVALID_FILE_ATTRIBUTES) {
			return false;
		}

		g_settings.goldFormID = ReadHexFormIDSetting(a_path, "GoldFormID", kDefaultGoldFormID);
		const auto aggregateModeDefault = GetPrivateProfileIntA("LootGlow", "ValueAggregateMode", GetPrivateProfileIntA("LootGlow", "HighValueAggregateMode", 1, a_path), a_path);
		g_settings.valueAggregateMode = GetPrivateProfileIntA("LootGlow", "AggregateMode", aggregateModeDefault, a_path) != 0;
		g_settings.debugLogging = GetPrivateProfileIntA("LootGlow", "DebugLogging", 0, a_path) != 0;
		g_settings.visualRefreshMode = GetPrivateProfileIntA("LootGlow", "VisualRefreshMode", 1, a_path) != 0;

		g_settings.lockpickMode = GetPrivateProfileIntA("LootGlow", "LockpickMode", 0, a_path) != 0;
		g_settings.lockpickFormID = ReadHexFormIDSetting(a_path, "LockpickFormID", kDefaultLockpickFormID);
		g_settings.lockpickShaderFormID = ReadHexFormIDSetting(a_path, "LockpickShaderFormID", kDefaultLockpickShaderFormID);
		g_settings.lockpickStackCount = ClampStackCount(ReadPositiveIntSetting(a_path, "LockpickStackCount", kDefaultLockpickStackCount));

		g_settings.lowTierEnabled = GetPrivateProfileIntA("LootGlow", "LowTierMode", 1, a_path) != 0;
		g_settings.lowTierThreshold = ReadPositiveIntSetting(a_path, "LowTierThreshold", kDefaultLowTierThreshold);
		g_settings.lowTierShaderFormID = ReadHexFormIDSetting(a_path, "LowTierShaderFormID", kDefaultLowTierShaderFormID);
		g_settings.lowTierStackCount = ClampStackCount(ReadPositiveIntSetting(a_path, "LowTierStackCount", kDefaultLowTierStackCount));

		// Compatibility: old GoldCountThreshold / ValueThreshold naturally become the medium tier threshold.
		const auto legacyMediumDefault = static_cast<std::uint32_t>(GetPrivateProfileIntA(
			"LootGlow",
			"ValueThreshold",
			GetPrivateProfileIntA("LootGlow", "GoldCountThreshold", static_cast<int>(kDefaultMediumTierThreshold), a_path),
			a_path));

		g_settings.mediumTierEnabled = GetPrivateProfileIntA("LootGlow", "MediumTierMode", 1, a_path) != 0;
		g_settings.mediumTierThreshold = ReadPositiveIntSetting(a_path, "MediumTierThreshold", legacyMediumDefault == 0 ? kDefaultMediumTierThreshold : legacyMediumDefault);
		g_settings.mediumTierShaderFormID = ReadHexFormIDSetting(a_path, "MediumTierShaderFormID", ReadHexFormIDSetting(a_path, "ValueShaderFormID", ReadHexFormIDSetting(a_path, "ShaderFormID", kDefaultMediumTierShaderFormID)));
		g_settings.mediumTierStackCount = ClampStackCount(ReadPositiveIntSetting(a_path, "MediumTierStackCount", static_cast<std::uint32_t>(GetPrivateProfileIntA("LootGlow", "ValueStackCount", GetPrivateProfileIntA("LootGlow", "StackCount", static_cast<int>(kDefaultMediumTierStackCount), a_path), a_path))));

		g_settings.highTierEnabled = GetPrivateProfileIntA("LootGlow", "HighTierMode", 1, a_path) != 0;
		g_settings.highTierThreshold = ReadPositiveIntSetting(a_path, "HighTierThreshold", kDefaultHighTierThreshold);
		g_settings.highTierShaderFormID = ReadHexFormIDSetting(a_path, "HighTierShaderFormID", kDefaultHighTierShaderFormID);
		g_settings.highTierStackCount = ClampStackCount(ReadPositiveIntSetting(a_path, "HighTierStackCount", kDefaultHighTierStackCount));

		g_settings.insaneTierEnabled = GetPrivateProfileIntA("LootGlow", "InsaneTierMode", 1, a_path) != 0;
		g_settings.insaneTierThreshold = ReadPositiveIntSetting(a_path, "InsaneTierThreshold", kDefaultInsaneTierThreshold);
		g_settings.insaneTierShaderFormID = ReadHexFormIDSetting(a_path, "InsaneTierShaderFormID", kDefaultInsaneTierShaderFormID);
		g_settings.insaneTierStackCount = ClampStackCount(ReadPositiveIntSetting(a_path, "InsaneTierStackCount", kDefaultInsaneTierStackCount));
		g_settings.insaneTierSecondaryEnabled = GetPrivateProfileIntA("LootGlow", "InsaneTierSecondaryMode", 1, a_path) != 0;
		g_settings.insaneTierSecondaryShaderFormID = ReadHexFormIDSetting(a_path, "InsaneTierSecondaryShaderFormID", kDefaultInsaneTierSecondaryShaderFormID);
		g_settings.insaneTierSecondaryStackCount = ClampStackCount(ReadPositiveIntSetting(a_path, "InsaneTierSecondaryStackCount", kDefaultInsaneTierSecondaryStackCount));

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

		char current[MAX_PATH]{};
		std::snprintf(current, sizeof(current), "%s", exeDir);
		for (std::uint32_t depth = 0; depth < 6; ++depth) {
			if (TryIniPathWithPrefix(current, "OBSE\\Plugins\\LootGlow.ini") ||
				TryIniPathWithPrefix(current, "Data\\OBSE\\Plugins\\LootGlow.ini")) {
				return true;
			}

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
		if (TryLoadSettingsFromExeRelatedPaths() ||
			TryLoadSettingsFromIni("Data\\OBSE\\Plugins\\LootGlow.ini") ||
			TryLoadSettingsFromIni("OBSE\\Plugins\\LootGlow.ini") ||
			TryLoadSettingsFromIni("LootGlow.ini")) {
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

	template <std::size_t N>
	void CopyName(char (&a_dst)[N], std::string_view a_name)
	{
		static_assert(N > 0);
		for (auto& ch : a_dst) {
			ch = 0;
		}
		const auto count = a_name.size() < (N - 1) ? a_name.size() : (N - 1);
		for (std::size_t i = 0; i < count; ++i) {
			a_dst[i] = a_name[i];
		}
	}

	void ClearTrackedRef(TrackedRef& a_entry)
	{
		a_entry = TrackedRef{};
	}

	RE::TESEffectShader* ResolveTierShader(LootTier a_tier)
	{
		switch (a_tier) {
		case LootTier::Low:
			if (!g_lowTierShader) {
				g_lowTierShader = RE::TESForm::LookupByID<RE::TESEffectShader>(g_settings.lowTierShaderFormID);
				if (!g_lowTierShader) {
					++g_counters.shaderResolveFailures;
					REX::INFO("LootGlow Low tier shader lookup failed/not ready: formID={:08X}", g_settings.lowTierShaderFormID);
				} else if (g_settings.debugLogging) {
					REX::INFO("LootGlow Low tier shader resolved: ptr={:016X}, formID={:08X}", reinterpret_cast<std::uintptr_t>(g_lowTierShader), g_settings.lowTierShaderFormID);
				}
			}
			return g_lowTierShader;
		case LootTier::Medium:
			if (!g_mediumTierShader) {
				g_mediumTierShader = RE::TESForm::LookupByID<RE::TESEffectShader>(g_settings.mediumTierShaderFormID);
				if (!g_mediumTierShader) {
					++g_counters.shaderResolveFailures;
					REX::INFO("LootGlow Medium tier shader lookup failed/not ready: formID={:08X}", g_settings.mediumTierShaderFormID);
				} else if (g_settings.debugLogging) {
					REX::INFO("LootGlow Medium tier shader resolved: ptr={:016X}, formID={:08X}", reinterpret_cast<std::uintptr_t>(g_mediumTierShader), g_settings.mediumTierShaderFormID);
				}
			}
			return g_mediumTierShader;
		case LootTier::High:
			if (!g_highTierShader) {
				g_highTierShader = RE::TESForm::LookupByID<RE::TESEffectShader>(g_settings.highTierShaderFormID);
				if (!g_highTierShader) {
					++g_counters.shaderResolveFailures;
					REX::INFO("LootGlow High tier shader lookup failed/not ready: formID={:08X}", g_settings.highTierShaderFormID);
				} else if (g_settings.debugLogging) {
					REX::INFO("LootGlow High tier shader resolved: ptr={:016X}, formID={:08X}", reinterpret_cast<std::uintptr_t>(g_highTierShader), g_settings.highTierShaderFormID);
				}
			}
			return g_highTierShader;
		case LootTier::Insane:
			if (!g_insaneTierShader) {
				g_insaneTierShader = RE::TESForm::LookupByID<RE::TESEffectShader>(g_settings.insaneTierShaderFormID);
				if (!g_insaneTierShader) {
					++g_counters.shaderResolveFailures;
					REX::INFO("LootGlow Insane tier shader lookup failed/not ready: formID={:08X}", g_settings.insaneTierShaderFormID);
				} else if (g_settings.debugLogging) {
					REX::INFO("LootGlow Insane tier shader resolved: ptr={:016X}, formID={:08X}", reinterpret_cast<std::uintptr_t>(g_insaneTierShader), g_settings.insaneTierShaderFormID);
				}
			}
			return g_insaneTierShader;
		default:
			return nullptr;
		}
	}

	RE::TESEffectShader* ResolveInsaneTierSecondaryShader()
	{
		if (!g_settings.insaneTierSecondaryEnabled || g_settings.insaneTierSecondaryShaderFormID == 0 || g_settings.insaneTierSecondaryStackCount == 0) {
			return nullptr;
		}

		if (!g_insaneTierSecondaryShader) {
			g_insaneTierSecondaryShader = RE::TESForm::LookupByID<RE::TESEffectShader>(g_settings.insaneTierSecondaryShaderFormID);
			if (!g_insaneTierSecondaryShader) {
				++g_counters.shaderResolveFailures;
				REX::INFO("LootGlow Insane secondary shader lookup failed/not ready: formID={:08X}", g_settings.insaneTierSecondaryShaderFormID);
			} else if (g_settings.debugLogging) {
				REX::INFO("LootGlow Insane secondary shader resolved: ptr={:016X}, formID={:08X}", reinterpret_cast<std::uintptr_t>(g_insaneTierSecondaryShader), g_settings.insaneTierSecondaryShaderFormID);
			}
		}
		return g_insaneTierSecondaryShader;
	}

	RE::TESEffectShader* ResolveLockpickShader()
	{
		if (!g_lockpickShader) {
			g_lockpickShader = RE::TESForm::LookupByID<RE::TESEffectShader>(g_settings.lockpickShaderFormID);
			if (!g_lockpickShader) {
				++g_counters.shaderResolveFailures;
				REX::INFO("LootGlow Lockpick shader lookup failed/not ready: formID={:08X}", g_settings.lockpickShaderFormID);
			} else if (g_settings.debugLogging) {
				REX::INFO("LootGlow Lockpick shader resolved: ptr={:016X}, formID={:08X}", reinterpret_cast<std::uintptr_t>(g_lockpickShader), g_settings.lockpickShaderFormID);
			}
		}
		return g_lockpickShader;
	}

	RE::TESFormID TierShaderFormID(LootTier a_tier)
	{
		return a_tier == LootTier::Low ? g_settings.lowTierShaderFormID :
		       a_tier == LootTier::Medium ? g_settings.mediumTierShaderFormID :
		       a_tier == LootTier::High ? g_settings.highTierShaderFormID :
		       a_tier == LootTier::Insane ? g_settings.insaneTierShaderFormID : 0;
	}

	std::uint32_t TierStackCount(LootTier a_tier)
	{
		return a_tier == LootTier::Low ? g_settings.lowTierStackCount :
		       a_tier == LootTier::Medium ? g_settings.mediumTierStackCount :
		       a_tier == LootTier::High ? g_settings.highTierStackCount :
		       a_tier == LootTier::Insane ? g_settings.insaneTierStackCount : 0;
	}

	LootTier SelectTier(std::uint64_t a_knownValueTotal, std::uint32_t a_highestValueCandidate)
	{
		const auto value = g_settings.valueAggregateMode ? a_knownValueTotal : static_cast<std::uint64_t>(a_highestValueCandidate);
		if (g_settings.insaneTierEnabled && value >= g_settings.insaneTierThreshold) {
			return LootTier::Insane;
		}
		if (g_settings.highTierEnabled && value >= g_settings.highTierThreshold) {
			return LootTier::High;
		}
		if (g_settings.mediumTierEnabled && value >= g_settings.mediumTierThreshold) {
			return LootTier::Medium;
		}
		if (g_settings.lowTierEnabled && value >= g_settings.lowTierThreshold) {
			return LootTier::Low;
		}
		return LootTier::None;
	}

	void* AllocateGameObject(std::uint64_t a_size)
	{
		REL::Relocation<AllocFn> fn{ REL::ID(303328) };  // 1.512.105.0: 144702CB0
		return fn(a_size);
	}

	void* ConstructMagicShaderHitEffect(RE::TESObjectREFR* a_ref, RE::TESEffectShader* a_shader, float a_duration)
	{
		void* mem = AllocateGameObject(kMagicShaderHitEffectSize);
		if (!mem) {
			return nullptr;
		}
		REL::Relocation<MagicShaderCtorFn> ctor{ REL::ID(415141) };  // 1.512.105.0: 1468AAE60
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
		REL::Relocation<ProcessListsGetterFn> fn{ REL::ID(410297) };  // 1.512.105.0: 14674B050
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
			REX::INFO("LootGlow list insert failed: processLists={:016X}, effect={:016X}", processLists, effect);
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
		REL::Relocation<FinishMagicShaderHitEffectFn> fn{ REL::ID(410207) };  // 1.512.105.0: 146741E60
		fn(processLists, a_ref, a_shader);
	}

	void FinishTrackedStack(RE::TESObjectREFR* a_ref, RE::TESEffectShader* a_shader, const GlowStackState& a_state)
	{
		if (!a_ref || !a_shader || !a_state.applied) {
			return;
		}
		auto attempts = a_state.activeStacks > 0 ? a_state.activeStacks : 1u;
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

	std::uint32_t CountGlowingRefs()
	{
		std::uint32_t count = 0;
		for (const auto& entry : g_trackedRefs) {
			if (entry.ref != 0 && ((entry.appliedTier != LootTier::None && entry.valueGlow.applied) || (entry.appliedLockpickGlow && entry.lockpickGlow.applied))) {
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

	void MaybeLogStats(const char* a_reason, bool a_force = false)
	{
		if (!g_settings.debugLogging && !a_force) {
			return;
		}

		const auto now = NowMs();
		const bool milestone =
			MilestoneReached(g_counters.uniqueContainers, g_lastStatsUniqueContainers) ||
			MilestoneReached(g_counters.applySuccesses, g_lastStatsApplySuccesses) ||
			MilestoneReached(g_counters.removeSuccesses, g_lastStatsRemoveSuccesses);

		if (!a_force && !milestone && g_lastStatsLogMs != 0 && now - g_lastStatsLogMs < kStatsLogIntervalMs) {
			return;
		}

		g_lastStatsLogMs = now;
		REX::INFO("[LootGlow] summary reason={}, tracked={}/{}, glowing={}, loads={}/{}, scans={}, tiers={}/{}/{}/{}/{}, lockpick={}, rebuilds={}, applies={}/{}, removes={}, skipped={}, refreshes={}, shaderFails={}, tableFull={}, reuseResets={}",
			a_reason ? a_reason : "stats",
			CountTrackedRefs(),
			kMaxTrackedRefs,
			CountGlowingRefs(),
			g_counters.loadGraphicsHits,
			g_counters.containerLoadHits,
			g_counters.scanCalls,
			g_counters.tierNone,
			g_counters.tierLow,
			g_counters.tierMedium,
			g_counters.tierHigh,
			g_counters.tierInsane,
			g_counters.lockpickDesired,
			g_counters.rebuildAttempts,
			g_counters.applySuccesses,
			g_counters.applyAttempts,
			g_counters.removeSuccesses,
			g_counters.skippedNoChange,
			g_counters.visualRefreshes,
			g_counters.shaderResolveFailures,
			g_counters.trackingTableFull,
			g_counters.pointerReuseResets);
	}

	TrackedRef* TrackContainer(RE::TESObjectREFR* a_ref, RE::TESObjectCONT* a_container, std::string_view a_name)
	{
		const auto ref = reinterpret_cast<std::uintptr_t>(a_ref);
		if (!LooksPointerish(ref)) {
			return nullptr;
		}

		const auto currentRefFormID = a_ref ? a_ref->GetFormID() : 0;
		const auto currentBaseFormID = a_container ? a_container->GetFormID() : 0;

		if (auto* existing = FindTrackedRef(ref)) {
			const bool refFormChanged = existing->refFormID != 0 && currentRefFormID != 0 && existing->refFormID != currentRefFormID;
			const bool baseFormChanged = existing->baseFormID != 0 && currentBaseFormID != 0 && existing->baseFormID != currentBaseFormID;
			if (refFormChanged || baseFormChanged) {
				++g_counters.pointerReuseResets;
				ClearTrackedRef(*existing);
			} else {
				return existing;
			}
		}

		for (auto& entry : g_trackedRefs) {
			if (entry.ref != 0) {
				continue;
			}

			entry.ref = ref;
			entry.refFormID = currentRefFormID;
			entry.baseFormID = currentBaseFormID;
			CopyName(entry.name, a_name);
			++g_counters.uniqueContainers;
			MaybeLogStats("track-new");
			return &entry;
		}

		++g_counters.trackingTableFull;
		REX::INFO("[LootGlow] tracking table full; ref={:016X}, refForm={:08X}, baseForm={:08X}", ref, currentRefFormID, currentBaseFormID);
		MaybeLogStats("table-full", true);
		return nullptr;
	}

	bool RemoveAppliedValueGlow(RE::TESObjectREFR* a_ref, TrackedRef& a_entry, const char* a_reason)
	{
		if (a_entry.appliedTier == LootTier::None || !a_entry.valueGlow.applied) {
			return true;
		}

		auto* shader = ResolveTierShader(a_entry.appliedTier);
		if (!shader) {
			return false;
		}

		++g_counters.removeAttempts;
		const auto oldTier = a_entry.appliedTier;
		const auto oldStacks = a_entry.valueGlow.activeStacks;
		const auto oldSecondaryStacks = a_entry.insaneSecondaryGlow.activeStacks;
		FinishTrackedStack(a_ref, shader, a_entry.valueGlow);
		if (oldTier == LootTier::Insane && a_entry.insaneSecondaryGlow.applied) {
			if (auto* secondaryShader = ResolveInsaneTierSecondaryShader()) {
				FinishTrackedStack(a_ref, secondaryShader, a_entry.insaneSecondaryGlow);
			}
		}
		a_entry.valueGlow = GlowStackState{};
		a_entry.insaneSecondaryGlow = GlowStackState{};
		a_entry.appliedTier = LootTier::None;
		++g_counters.removeSuccesses;

		if (g_settings.debugLogging) {
			REX::INFO("[LootGlow] value glow removed: ref={:016X}, refForm={:08X}, baseForm={:08X}, tier={}, primaryStacks={}, secondaryStacks={}, reason={}, name={}",
				a_entry.ref,
				a_entry.refFormID,
				a_entry.baseFormID,
				TierName(oldTier),
				oldStacks,
				oldSecondaryStacks,
				a_reason ? a_reason : "<none>",
				a_entry.name[0] ? a_entry.name : "<unnamed>");
		}
		return true;
	}

	bool ApplyValueGlow(RE::TESObjectREFR* a_ref, TrackedRef& a_entry, LootTier a_tier)
	{
		if (a_tier == LootTier::None) {
			return true;
		}

		auto* shader = ResolveTierShader(a_tier);
		if (!shader) {
			++g_counters.applyFailures;
			return false;
		}

		void* processLists = GetProcessLists();
		if (!processLists) {
			++g_counters.applyFailures;
			REX::INFO("LootGlow {} tier apply failed: ProcessLists singleton was null for ref={:016X}", TierName(a_tier), a_entry.ref);
			return false;
		}

		const auto stackCount = TierStackCount(a_tier);
		std::uint32_t stackSuccesses = 0;
		std::uint32_t secondaryStackSuccesses = 0;
		a_entry.valueGlow = GlowStackState{};
		a_entry.insaneSecondaryGlow = GlowStackState{};

		++g_counters.applyAttempts;
		for (std::uint32_t stackIndex = 0; stackIndex < stackCount; ++stackIndex) {
			void* effect = ConstructMagicShaderHitEffect(a_ref, shader, -1.0f);
			if (!effect || !InitMagicShaderHitEffect(effect) || !EmplaceFrontMagicEffectListPO3(processLists, effect)) {
				REX::INFO("LootGlow {} tier stack {}/{} failed for ref={:016X}", TierName(a_tier), stackIndex + 1, stackCount, a_entry.ref);
				continue;
			}
			++stackSuccesses;
		}

		if (stackSuccesses == 0) {
			++g_counters.applyFailures;
			return false;
		}

		if (a_tier == LootTier::Insane) {
			if (auto* secondaryShader = ResolveInsaneTierSecondaryShader()) {
				for (std::uint32_t stackIndex = 0; stackIndex < g_settings.insaneTierSecondaryStackCount; ++stackIndex) {
					void* effect = ConstructMagicShaderHitEffect(a_ref, secondaryShader, -1.0f);
					if (!effect || !InitMagicShaderHitEffect(effect) || !EmplaceFrontMagicEffectListPO3(processLists, effect)) {
						REX::INFO("LootGlow Insane secondary tier stack {}/{} failed for ref={:016X}", stackIndex + 1, g_settings.insaneTierSecondaryStackCount, a_entry.ref);
						continue;
					}
					++secondaryStackSuccesses;
				}
			}
		}

		a_entry.valueGlow.applied = true;
		a_entry.valueGlow.activeStacks = stackSuccesses;
		a_entry.insaneSecondaryGlow.applied = secondaryStackSuccesses > 0;
		a_entry.insaneSecondaryGlow.activeStacks = secondaryStackSuccesses;
		a_entry.appliedTier = a_tier;
		a_entry.lastApplyMs = NowMs();
		++g_counters.applySuccesses;

		if (g_settings.debugLogging) {
			REX::INFO("[LootGlow] value glow applied: ref={:016X}, refForm={:08X}, baseForm={:08X}, tier={}, primaryShader={:08X}, primaryStacks={}/{}, secondaryShader={:08X}, secondaryStacks={}/{}, name={}",
				a_entry.ref,
				a_entry.refFormID,
				a_entry.baseFormID,
				TierName(a_tier),
				TierShaderFormID(a_tier),
				stackSuccesses,
				stackCount,
				a_tier == LootTier::Insane ? g_settings.insaneTierSecondaryShaderFormID : 0,
				secondaryStackSuccesses,
				a_tier == LootTier::Insane ? g_settings.insaneTierSecondaryStackCount : 0,
				a_entry.name[0] ? a_entry.name : "<unnamed>");
		}
		return true;
	}

	bool RemoveAppliedLockpickGlow(RE::TESObjectREFR* a_ref, TrackedRef& a_entry, const char* a_reason)
	{
		if (!a_entry.appliedLockpickGlow || !a_entry.lockpickGlow.applied) {
			return true;
		}

		auto* shader = ResolveLockpickShader();
		if (!shader) {
			return false;
		}

		++g_counters.removeAttempts;
		const auto oldStacks = a_entry.lockpickGlow.activeStacks;
		FinishTrackedStack(a_ref, shader, a_entry.lockpickGlow);
		a_entry.lockpickGlow = GlowStackState{};
		a_entry.appliedLockpickGlow = false;
		++g_counters.removeSuccesses;

		if (g_settings.debugLogging) {
			REX::INFO("[LootGlow] lockpick glow removed: ref={:016X}, refForm={:08X}, baseForm={:08X}, stacks={}, reason={}, name={}",
				a_entry.ref,
				a_entry.refFormID,
				a_entry.baseFormID,
				oldStacks,
				a_reason ? a_reason : "<none>",
				a_entry.name[0] ? a_entry.name : "<unnamed>");
		}
		return true;
	}

	bool ApplyLockpickGlow(RE::TESObjectREFR* a_ref, TrackedRef& a_entry)
	{
		// Defensive v0.4.0G guard:
		// A lockpick stack should never be started while a value tier is active.
		// This catches any future accidental mixed-plan request before it reaches UE.
		if (a_entry.appliedTier != LootTier::None) {
			if (g_settings.debugLogging) {
				REX::INFO("[LootGlow] lockpick glow suppressed because value tier is active: ref={:016X}, refForm={:08X}, baseForm={:08X}, activeTier={}, name={}",
					a_entry.ref,
					a_entry.refFormID,
					a_entry.baseFormID,
					TierName(a_entry.appliedTier),
					a_entry.name[0] ? a_entry.name : "<unnamed>");
			}
			return true;
		}

		auto* shader = ResolveLockpickShader();
		if (!shader) {
			++g_counters.applyFailures;
			return false;
		}

		void* processLists = GetProcessLists();
		if (!processLists) {
			++g_counters.applyFailures;
			REX::INFO("LootGlow lockpick apply failed: ProcessLists singleton was null for ref={:016X}", a_entry.ref);
			return false;
		}

		const auto stackCount = g_settings.lockpickStackCount;
		std::uint32_t stackSuccesses = 0;
		a_entry.lockpickGlow = GlowStackState{};

		++g_counters.applyAttempts;
		for (std::uint32_t stackIndex = 0; stackIndex < stackCount; ++stackIndex) {
			void* effect = ConstructMagicShaderHitEffect(a_ref, shader, -1.0f);
			if (!effect || !InitMagicShaderHitEffect(effect) || !EmplaceFrontMagicEffectListPO3(processLists, effect)) {
				REX::INFO("LootGlow lockpick stack {}/{} failed for ref={:016X}", stackIndex + 1, stackCount, a_entry.ref);
				continue;
			}
			++stackSuccesses;
		}

		if (stackSuccesses == 0) {
			++g_counters.applyFailures;
			return false;
		}

		a_entry.lockpickGlow.applied = true;
		a_entry.lockpickGlow.activeStacks = stackSuccesses;
		a_entry.appliedLockpickGlow = true;
		a_entry.lastApplyMs = NowMs();
		++g_counters.applySuccesses;

		if (g_settings.debugLogging) {
			REX::INFO("[LootGlow] lockpick glow applied: ref={:016X}, refForm={:08X}, baseForm={:08X}, shader={:08X}, stacks={}/{}, name={}",
				a_entry.ref,
				a_entry.refFormID,
				a_entry.baseFormID,
				g_settings.lockpickShaderFormID,
				stackSuccesses,
				stackCount,
				a_entry.name[0] ? a_entry.name : "<unnamed>");
		}
		return true;
	}

	bool RemoveAllOwnedVisuals(RE::TESObjectREFR* a_ref, TrackedRef& a_entry, const char* a_reason)
	{
		bool ok = true;
		ok = RemoveAppliedValueGlow(a_ref, a_entry, a_reason) && ok;
		ok = RemoveAppliedLockpickGlow(a_ref, a_entry, a_reason) && ok;
		return ok;
	}

	bool RebuildVisualPlan(RE::TESObjectREFR* a_ref, TrackedRef& a_entry, DesiredVisualPlan a_desiredPlan, const char* a_reason)
	{
		++g_counters.rebuildAttempts;
		if (!RemoveAllOwnedVisuals(a_ref, a_entry, a_reason)) {
			return false;
		}

		bool ok = true;
		if (a_desiredPlan.valueTier != LootTier::None) {
			ok = ApplyValueGlow(a_ref, a_entry, a_desiredPlan.valueTier) && ok;
		}
		if (a_desiredPlan.lockpickGlow && a_desiredPlan.valueTier == LootTier::None) {
			ok = ApplyLockpickGlow(a_ref, a_entry) && ok;
		} else if (a_desiredPlan.lockpickGlow && g_settings.debugLogging) {
			REX::INFO("[LootGlow] mixed value+lockpick visual plan suppressed before apply: ref={:016X}, tier={}, name={}",
				a_entry.ref,
				TierName(a_desiredPlan.valueTier),
				a_entry.name[0] ? a_entry.name : "<unnamed>");
		}
		return ok;
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

namespace LootGlow::TieredLoot
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
			auto* containerChanges = reinterpret_cast<RE::ExtraContainerChanges*>(a_ref->extra.GetExtraData(RE::EXTRA_DATA_TYPE::ContainerChanges));
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
		if (potion->data.costOverride > 0) {
			return { true, static_cast<std::uint32_t>(potion->data.costOverride) };
		}
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
		return typed ? dynamic_cast<RE::TESEnchantableForm*>(typed) : nullptr;
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
			return magicItem ? magicItem->GetCost(nullptr) : 0.0F;
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
		fullValue += static_cast<std::uint32_t>((0.4F * (rawCost + amountOfEnchantment)) + 0.5F);
#endif
		return fullValue;
	}

	struct ContainerValueSummary
	{
		std::int32_t goldTotalCount{ 0 };
		std::int32_t lockpickTotalCount{ 0 };
		std::uint64_t knownFullValueTotal{ 0 };
		std::uint32_t highestFullValueEach{ 0 };
		std::uint32_t knownItems{ 0 };
		std::uint32_t unknownItems{ 0 };

		std::uint64_t KnownValueTotal() const
		{
			return knownFullValueTotal + static_cast<std::uint64_t>(goldTotalCount > 0 ? goldTotalCount : 0);
		}

		std::uint32_t HighestValueCandidate() const
		{
			const auto goldCandidate = goldTotalCount > 0 ? static_cast<std::uint32_t>(goldTotalCount) : 0u;
			return highestFullValueEach > goldCandidate ? highestFullValueEach : goldCandidate;
		}
	};

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
		return a_changeCount > 0 ? a_changeCount : 0;
	}

	void AddItemToValueSummary(ContainerValueSummary& a_summary, RE::TESForm* a_form, std::int32_t a_count)
	{
		if (!a_form || a_count <= 0) {
			return;
		}
		if (IsExactLockpickForm(a_form)) {
			a_summary.lockpickTotalCount += a_count;
			return;
		}
		if (IsExactGoldForm(a_form)) {
			a_summary.goldTotalCount += a_count;
			return;
		}

		const auto fullValueEach = EstimateFullGoldValue(a_form);
		if (fullValueEach > 0) {
			++a_summary.knownItems;
			a_summary.knownFullValueTotal += static_cast<std::uint64_t>(fullValueEach) * static_cast<std::uint64_t>(a_count);
			if (fullValueEach > a_summary.highestFullValueEach) {
				a_summary.highestFullValueEach = fullValueEach;
			}
		} else {
			++a_summary.unknownItems;
		}
	}

	ContainerValueSummary ScanContainerValue(RE::TESObjectREFR* a_ref, RE::TESContainer* a_container)
	{
		ContainerValueSummary summary{};
		auto* changes = GetInventoryChanges(a_ref);

		for (auto it = a_container->objectList.begin(); it != a_container->objectList.end(); ++it) {
			auto* obj = reinterpret_cast<RE::ContainerObject*>(*it);
			if (!obj || !obj->type) {
				continue;
			}

			auto* change = FindItemChange(changes, obj->type);
			const auto baseCount = NormalizeBaseContainerCount(obj->count);
			const auto changeCount = change ? change->count : 0;
			const auto totalCount = EffectiveInventoryCount(baseCount, changeCount, change != nullptr);
			AddItemToValueSummary(summary, obj->type, totalCount);
		}

		if (changes && changes->list) {
			for (auto it = changes->list->begin(); it != changes->list->end(); ++it) {
				auto* change = *it;
				if (!change || !change->object || ContainerContains(a_container, change->object)) {
					continue;
				}
				AddItemToValueSummary(summary, change->object, EffectiveInventoryCountFromChangeOnly(change->count));
			}
		}

		return summary;
	}

	TrackedRef* EnsureTrackedContainer(RE::TESObjectREFR* a_ref)
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
		return ::TrackContainer(a_ref, objectContainer, name);
	}

	bool ScanAndApplyTier(RE::TESObjectREFR* a_ref, const char* a_source)
	{
		if (!a_ref) {
			return false;
		}

		auto* container = GetContainer(a_ref);
		if (!container) {
			return false;
		}

		++g_counters.scanCalls;
		auto* entry = EnsureTrackedContainer(a_ref);
		if (!entry) {
			return false;
		}

		const auto summary = ScanContainerValue(a_ref, container);
		const auto knownValueTotal = summary.KnownValueTotal();
		const auto highestValueCandidate = summary.HighestValueCandidate();
		const auto desiredTier = ::SelectTier(knownValueTotal, highestValueCandidate);
		const bool lockpickDetected = g_settings.lockpickMode && summary.lockpickTotalCount > 0;

		// v0.4.0G rule:
		// Lockpick is intentionally standalone-only. If a value tier qualifies,
		// the value tier owns the visual presentation and lockpick is suppressed.
		const bool lockpickSuppressedByValueTier = lockpickDetected && desiredTier != LootTier::None;
		const bool desiredLockpickGlow = lockpickDetected && !lockpickSuppressedByValueTier;
		const DesiredVisualPlan desiredPlan{ desiredTier, desiredLockpickGlow };
		switch (desiredTier) {
		case LootTier::Insane:
			++g_counters.tierInsane;
			break;
		case LootTier::High:
			++g_counters.tierHigh;
			break;
		case LootTier::Medium:
			++g_counters.tierMedium;
			break;
		case LootTier::Low:
			++g_counters.tierLow;
			break;
		default:
			++g_counters.tierNone;
			break;
		}
		if (desiredLockpickGlow) {
			++g_counters.lockpickDesired;
		}

		const bool classificationChanged =
			!entry->hasScanned ||
			entry->lastDesiredTier != desiredTier ||
			entry->lastDesiredLockpickGlow != desiredLockpickGlow ||
			entry->lastKnownValueTotal != knownValueTotal ||
			entry->lastHighestValueCandidate != highestValueCandidate ||
			entry->lastGoldTotalCount != summary.goldTotalCount ||
			entry->lastLockpickTotalCount != summary.lockpickTotalCount;

		if (classificationChanged && g_settings.debugLogging) {
			REX::INFO("[LootGlow] {} scan: ref={:08X}/{:08X}, desired={} lockpick={} suppressed={}, value={}, highest={}, gold={}, picks={}, name={}",
				a_source ? a_source : "scan",
				GetFormID(a_ref),
				entry->baseFormID,
				TierName(desiredTier),
				desiredLockpickGlow,
				lockpickSuppressedByValueTier,
				knownValueTotal,
				highestValueCandidate,
				summary.goldTotalCount,
				summary.lockpickTotalCount,
				entry->name[0] ? entry->name : "<unnamed>");
		}

		entry->hasScanned = true;
		entry->lastDesiredTier = desiredTier;
		entry->lastDesiredLockpickGlow = desiredLockpickGlow;
		entry->lastKnownValueTotal = knownValueTotal;
		entry->lastHighestValueCandidate = highestValueCandidate;
		entry->lastGoldTotalCount = summary.goldTotalCount;
		entry->lastLockpickTotalCount = summary.lockpickTotalCount;

		const bool valueStackMismatch =
			desiredTier != LootTier::None &&
			(!entry->valueGlow.applied ||
				entry->valueGlow.activeStacks < ::TierStackCount(desiredTier) ||
				(desiredTier == LootTier::Insane && g_settings.insaneTierSecondaryEnabled && g_settings.insaneTierSecondaryShaderFormID != 0 && (!entry->insaneSecondaryGlow.applied || entry->insaneSecondaryGlow.activeStacks < g_settings.insaneTierSecondaryStackCount)));
		const bool lockpickStackMismatch =
			desiredLockpickGlow &&
			(!entry->lockpickGlow.applied || entry->lockpickGlow.activeStacks < g_settings.lockpickStackCount);

		const auto now = NowMs();
		const bool lockpickRefreshDue =
			desiredLockpickGlow &&
			desiredTier == LootTier::None &&
			entry->appliedLockpickGlow &&
			entry->lockpickGlow.applied &&
			entry->lastApplyMs != 0 &&
			now - entry->lastApplyMs >= kLockpickRefreshIntervalMs;

		const bool loadRefreshDue =
			g_settings.visualRefreshMode &&
			a_source &&
			std::strcmp(a_source, "loadgraphics") == 0 &&
			desiredTier != LootTier::None &&
			entry->appliedTier == desiredTier &&
			entry->valueGlow.applied &&
			(entry->lastLoadRefreshMs == 0 || now - entry->lastLoadRefreshMs >= kLoadVisualRefreshCooldownMs);

		const DesiredVisualPlan appliedPlan{ entry->appliedTier, entry->appliedLockpickGlow };
		if (::SamePlan(desiredPlan, appliedPlan) && !valueStackMismatch && !lockpickStackMismatch && !lockpickRefreshDue && !loadRefreshDue) {
			++g_counters.skippedNoChange;
			return desiredTier != LootTier::None || desiredLockpickGlow;
		}

		const char* rebuildReason = "visual plan changed";
		if (lockpickRefreshDue) {
			rebuildReason = "lockpick refresh";
		} else if (loadRefreshDue) {
			rebuildReason = "load visual refresh";
			entry->lastLoadRefreshMs = now;
			++g_counters.visualRefreshes;
		}
		return ::RebuildVisualPlan(a_ref, *entry, desiredPlan, rebuildReason);
	}

}

struct Hook_SetInfoForRef_TieredLoot
{
	static bool SetInfoForRef(RE::TESObjectREFR* a_ref, bool a_arg2, bool a_arg3)
	{
		const bool result = SetInfoForRefHook(a_ref, a_arg2, a_arg3);
		LootGlow::TieredLoot::ScanAndApplyTier(a_ref, "hover-update");
		return result;
	}

	static inline REL::THook SetInfoForRefHook{
		"LootGlow_SetInfoForRef_TieredLoot",
		REL::ID(406425),
		0x63,
		SetInfoForRef
	};
};

struct Hook_LoadGraphics_TieredLoot
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

		if (TrackContainer(a_ref, container, name)) {
			LootGlow::TieredLoot::ScanAndApplyTier(a_ref, "loadgraphics");
		}

		MaybeLogStats("loadgraphics");
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

	REX::INFO("LootGlow v0.4.0 initialized");
	REX::INFO("Tier settings: aggregateMode={}, Low(enabled={}, threshold={}, shader={:08X}, stacks={}), Medium(enabled={}, threshold={}, shader={:08X}, stacks={}), High(enabled={}, threshold={}, shader={:08X}, stacks={}), Insane(enabled={}, threshold={}, primaryShader={:08X}, primaryStacks={}, secondaryEnabled={}, secondaryShader={:08X}, secondaryStacks={}), Lockpick(enabled={}, form={:08X}, shader={:08X}, stacks={})",
		g_settings.valueAggregateMode,
		g_settings.lowTierEnabled,
		g_settings.lowTierThreshold,
		g_settings.lowTierShaderFormID,
		g_settings.lowTierStackCount,
		g_settings.mediumTierEnabled,
		g_settings.mediumTierThreshold,
		g_settings.mediumTierShaderFormID,
		g_settings.mediumTierStackCount,
		g_settings.highTierEnabled,
		g_settings.highTierThreshold,
		g_settings.highTierShaderFormID,
		g_settings.highTierStackCount,
		g_settings.insaneTierEnabled,
		g_settings.insaneTierThreshold,
		g_settings.insaneTierShaderFormID,
		g_settings.insaneTierStackCount,
		g_settings.insaneTierSecondaryEnabled,
		g_settings.insaneTierSecondaryShaderFormID,
		g_settings.insaneTierSecondaryStackCount,
		g_settings.lockpickMode,
		g_settings.lockpickFormID,
		g_settings.lockpickShaderFormID,
		g_settings.lockpickStackCount);
	REX::INFO("Visual refresh settings: refreshMode={} (0=off, 1=load/graphics defensive refresh)",
		g_settings.visualRefreshMode);
	REX::INFO("v0.4.0 behavior: four exclusive value tiers suppress lockpick glow; Insane can apply a configurable secondary accent shader; VisualRefreshMode=1 refreshes value-tier visuals on LoadGraphics, no periodic value blink");
	if (g_settings.debugLogging) {
		MaybeLogStats("startup", true);
	}
	return true;
}

# LootGlow

LootGlow is an OBSE64 plugin for **The Elder Scrolls IV: Oblivion Remastered** that highlights loot containers based on what is inside them.

Simply put, valuable, useful, or unique containers should be easier to notice at a glance without opening every chest, coffin, barrel, or sack.

## Features

- Highlights containers based on estimated loot value.
- Supports four value tiers:
  - **Low**
  - **Medium**
  - **High**
  - **Insane**
- Highlights containers containing known unique/artifact items.
- Unique/artifact items take priority over normal value tiers and lockpick glow.
- The **Insane** and **Unique** visuals use built-in accent shaders for stronger effects.
- Highlights lockpick-only containers when no unique item or value tier is active.
- Value tiers suppress lockpick-only glow, so a valuable container with lockpicks still uses the value-tier visual.
- Supports total-value or highest-item-value detection.
- Includes a defensive visual refresh option for containers whose graphics load or reload after the initial scan.

## Requirements

- **Oblivion Remastered**
- **OBSE64**
- **Address Library**

## Installation

Install with your preferred mod manager, or install manually by placing the files in:

```text
Data/OBSE/Plugins/
```

The folder should contain:

```text
LootGlow.dll
LootGlow.ini
```

## Configuration

LootGlow works out of the box with the included configuration.

The default INI intentionally exposes only the most useful settings. Shader FormIDs, shader stack counts, and other advanced tuning options are handled internally so normal users do not need to configure them.

```ini
[LootGlow]

; Add item values together when deciding a container's loot tier.
; Example: five 20-gold items count as 100 total value.
; 1 = enabled, 0 = disabled
AggregateMode=1

; Defensive visual refresh when a container's graphics load or reload.
; Recommended: 1
; 1 = enabled, 0 = disabled
VisualRefreshMode=1

; Containers with known unique/artifact items use a special Unique glow.
; Unique items take priority over normal value tiers and lockpicks.
; 1 = enabled, 0 = disabled
UniqueItemMode=1

; Minimum total value needed for each glow tier.
LowTierThreshold=25
MediumTierThreshold=100
HighTierThreshold=250
InsaneTierThreshold=500

; Containers with lockpicks use a Lockpick glow only when they do not
; already qualify for Unique or value-tier glow.
; 1 = enabled, 0 = disabled
LockpickMode=1
```

## Glow Priority

LootGlow chooses one visual category per container.

Priority order:

1. **Unique** — known unique/artifact item present.
2. **Insane** — value meets or exceeds the Insane threshold.
3. **High** — value meets or exceeds the High threshold.
4. **Medium** — value meets or exceeds the Medium threshold.
5. **Low** — value meets or exceeds the Low threshold.
6. **Lockpick** — lockpicks are present and no higher-priority glow applies.
7. **None** — no qualifying contents found.

This means a container with a unique item will always use the Unique glow, even if it also contains expensive loot or lockpicks.

## Unique Items

LootGlow includes a built-in list of known unique/artifact item FormIDs.

Unique detection is intentionally separate from monetary value detection. Some unique items have unusual, misleading, or low gold values, but are still important to the player. LootGlow treats those as special loot and highlights them with the Unique visual when `UniqueItemMode=1`.

Examples of unique/artifact items include Daedric artifacts, quest rewards, special weapons, special armor, and other hand-placed unique items.

## Value Tiers

LootGlow chooses the highest matching value tier.

With the default settings:

- Containers worth **25+ gold** use the Low tier.
- Containers worth **100+ gold** use the Medium tier.
- Containers worth **250+ gold** use the High tier.
- Containers worth **500+ gold** use the Insane tier.

Unique items take priority over value tiers.

## AggregateMode

```ini
AggregateMode=1
```

Uses the estimated total value of the container.

```ini
AggregateMode=0
```

Uses the highest estimated value of a single item or stack in the container.

## LockpickMode

```ini
LockpickMode=1
```

Enables lockpick-only glow.

Lockpick-only glow applies only when the container does not already qualify for Unique or value-tier glow.

## VisualRefreshMode

```ini
VisualRefreshMode=1
```

Enables a defensive refresh when container graphics load or reload. This helps restore visuals in cases where the game loads a container after LootGlow has already scanned it.

This is not a background refresh loop. LootGlow does not use an engine tick hook, background thread, async refresh loop, or periodic shader refresh timer.

## Compatibility Notes

LootGlow only adds visual shader effects to detected containers. It does not change leveled lists, container contents, item values, or loot generation.

LootGlow hooks the container graphics/loading path and tracks loaded container references. When a container is loaded, LootGlow checks its inventory for qualifying loot.

If another mod changes container contents, LootGlow should evaluate the container based on what the game reports at runtime.

However, compatibility with other mods that deeply alter the same container/menu or shader-effect paths is not guaranteed.

## Advanced Settings

The default INI intentionally exposes only the most useful settings.

LootGlow also supports additional advanced options for shader tuning and troubleshooting, but they are hidden from the default INI to keep installation simple. Most users should not need to change them.

Advanced users may manually add supported shader FormID, stack count, secondary shader, or debug logging keys if needed for testing or troubleshooting.

## Building From Source

LootGlow is built as an OBSE64/CommonLibOB64 plugin.

The development build environment used for release builds included:

- Windows
- Visual Studio C++ toolchain
- xmake
- CommonLibOB64

Typical build commands from the project root:

```powershell
xmake f -c
xmake
```

Build artifacts are not intended to be committed directly to this repository. Release DLLs should be distributed through GitHub Releases and/or a mod hosting site.

## Repository Layout

Suggested source repository layout:

```text
LootGlow/
├─ README.md
├─ LICENSE
├─ LootGlow.ini
├─ xmake.lua
├─ src/
│  └─ main.cpp
└─ docs/
   └─ THIRD_PARTY_NOTICES.md
```

## Credits and Acknowledgements

LootGlow was made possible through study of existing open-source Oblivion/Skyrim modding work and through extensive runtime testing.

Special thanks to:

### powerof3 — creator of Unread Books Glow

Unread Books Glow was an important reference during LootGlow development, especially for understanding the `MagicShaderHitEffect`-based glow application pattern.

LootGlow does not redistribute Unread Books Glow assets or convert Unread Books Glow to another game. Credit is given to powerof3 as the creator of Unread Books Glow.

### Wolfmark — creator of Container and Inventory Menu Fix

Container and Inventory Menu Fix was an important reference during LootGlow development, especially for understanding Oblivion Remastered container inventory access, `InventoryChanges` handling, and the target/mini-menu information path.

Credit is given to Wolfmark as the creator of Container and Inventory Menu Fix.

### CommonLibOB64

LootGlow is built using CommonLibOB64. CommonLibOB64 is licensed under the GNU General Public License v3.0.

### OBSE64

LootGlow is an OBSE64 plugin. OBSE64 is required separately and is not redistributed with LootGlow.

## Licensing

LootGlow source code is released under the GNU General Public License v3.0.

See `LICENSE` for the full license text.

See `docs/THIRD_PARTY_NOTICES.md` for third-party credits, permissions notes, and licensing notes.

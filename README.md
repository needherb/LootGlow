# LootGlow

LootGlow is an OBSE64 plugin for **The Elder Scrolls IV: Oblivion Remastered** that highlights loot containers based on what is inside them.

The goal is simple: valuable containers should be easier to notice at a glance without opening every chest, coffin, barrel, or sack.

## Features

- Highlights containers based on estimated loot value.
- Supports four value tiers:
  - **Low**
  - **Medium**
  - **High**
  - **Insane**
- The **Insane** tier uses a configurable accent shader by default for a stronger visual effect.
- Highlights lockpick-only containers when no value tier is active.
- Value tiers suppress lockpick-only glow, so a valuable container with lockpicks still uses the value-tier visual.
- Supports total-value or highest-item-value detection.
- Includes a defensive visual refresh option for containers whose graphics load or reload after the initial scan.
- No engine tick refresh, background thread, or periodic shader refresh loop.

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

```ini
[LootGlow]

LowTierThreshold=25
LowTierShaderFormID=000C7939

MediumTierThreshold=100
MediumTierShaderFormID=000C793E

HighTierThreshold=250
HighTierShaderFormID=0008B95F

InsaneTierThreshold=500
InsaneTierShaderFormID=000C793F

LockpickMode=1
LockpickShaderFormID=0014A0A2

; Set to 0 to disable the Insane tier accent shader.
InsaneTierSecondaryMode=1
InsaneTierSecondaryShaderFormID=0018B576

; 1 = use total estimated container value
; 0 = use highest single item/stack value
AggregateMode=1

; 0 = disabled
; 1 = defensive refresh when container graphics load/reload
VisualRefreshMode=1
```

### Value tiers

LootGlow chooses the highest matching tier.

For example, with the default settings:

- Containers worth **25+ gold** use the Low tier.
- Containers worth **100+ gold** use the Medium tier.
- Containers worth **250+ gold** use the High tier.
- Containers worth **500+ gold** use the Insane tier.

The Insane tier can also apply a secondary accent shader. This is enabled by default and can be disabled with:

```ini
InsaneTierSecondaryMode=0
```

### AggregateMode

```ini
AggregateMode=1
```

Uses the estimated total value of the container.

```ini
AggregateMode=0
```

Uses the highest estimated value of a single item or stack in the container.

### LockpickMode

```ini
LockpickMode=1
```

Enables lockpick-only glow.

If a container qualifies for any value tier, the value tier wins and lockpick-only glow is suppressed.

### VisualRefreshMode

```ini
VisualRefreshMode=1
```

Enables a defensive refresh when container graphics load or reload. This helps restore visuals in cases where the game loads a container after LootGlow has already scanned it.


## Compatibility Notes

LootGlow only adds visual shader effects to detected containers. It does not change leveled lists, container contents, item values, or loot generation.

LootGlow hooks the container graphics/loading path and tracks loaded container references. When a container is loaded, LootGlow checks its inventory for qualifying loot.

If another mod changes container contents, LootGlow should evaluate the container based on what the game reports at runtime.

However, compatibility with other mods that deeply alter the same container/menu or shader-effect paths is not guaranteed.

## Advanced Settings

The default INI intentionally exposes only the most useful settings.

LootGlow also supports additional advanced options for shader tuning and troubleshooting, but they are hidden from the default INI to keep installation simple. Most users should not need to change them.

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

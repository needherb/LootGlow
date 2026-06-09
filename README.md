# LootGlow

**LootGlow** is an OBSE64/CommonLibOB64 plugin for **Oblivion Remastered** that highlights loaded containers with worthwhile loot.

LootGlow can highlight containers that contain:

- enough gold to meet your configured gold threshold
- valuable non-gold loot, including high-value unenchanted items and enchanted/generated items

Gold and high-value loot use separate glow effects, so a container can show one glow or both at the same time. Loose gold in the world is intentionally ignored; LootGlow is designed for containers.

## Features

- Automatically scans loaded containers.
- Highlights containers that contain enough gold.
- Highlights containers that contain valuable non-gold loot.
- Supports enchanted/generated item value estimates.
- Uses separate glow effects for gold and high-value loot.
- Removes the appropriate glow after qualifying loot is removed.
- Ignores loose gold by design.
- Uses a simple INI file with only the main user-facing settings.
- Uses quiet logging by default.

## Requirements

- Oblivion Remastered
- OBSE64
- A mod manager or manual installation into the game's OBSE plugin folder

OBSE64 is required and must be installed separately. LootGlow does not redistribute OBSE64.

## Installation

Install the release package so that `LootGlow.dll` and `LootGlow.ini` end up in the OBSE Plugins folder used by your Oblivion Remastered installation.

Typical manual install path:

```text
OblivionRemastered\Binaries\Win64\OBSE\Plugins\
```

The folder should contain:

```text
LootGlow.dll
LootGlow.ini
```

For example:

```text
OblivionRemastered\Binaries\Win64\OBSE\Plugins\LootGlow.dll
OblivionRemastered\Binaries\Win64\OBSE\Plugins\LootGlow.ini
```

If using a mod manager, package or install the mod so that the same files are placed under the game's `OBSE\Plugins` folder.

## Configuration

LootGlow uses `LootGlow.ini` if present. If the INI is missing, built-in defaults are used.

Default configuration:

```ini
[LootGlow]

; Containers glow when they contain this much gold or more.
GoldCountThreshold=100

; Set to 0 to disable glow for valuable non-gold loot.
HighValueMode=1

; Containers glow when they contain loot worth this much or more.
HighValueThreshold=250
```

### Settings

#### `GoldCountThreshold`

The minimum amount of gold a container must contain before the gold glow is applied.

Default:

```ini
GoldCountThreshold=100
```

Examples:

```ini
; Any gold-containing container glows.
GoldCountThreshold=1

; Only containers with 100 or more gold glow.
GoldCountThreshold=100

; Only containers with 500 or more gold glow.
GoldCountThreshold=500
```

#### `HighValueMode`

Enables or disables the high-value loot glow.

Default:

```ini
HighValueMode=1
```

Set to `0` if you only want the gold glow:

```ini
HighValueMode=0
```

#### `HighValueThreshold`

The minimum estimated value required before a container receives the high-value loot glow.

Default:

```ini
HighValueThreshold=250
```

LootGlow estimates high-value loot using item value information from the game. Enchanted/generated items receive an additional enchantment-based value estimate so that valuable enchanted loot is not treated like a cheap base item.

Examples:

```ini
; More sensitive high-value glow.
HighValueThreshold=100

; Default behavior.
HighValueThreshold=250

; Only very valuable loot containers glow.
HighValueThreshold=1000
```

## Advanced Settings

The default INI intentionally exposes only the most useful settings.

LootGlow also supports additional advanced options for shader tuning and troubleshooting, but they are hidden from the default INI to keep installation simple. Most users should not need to change them.

## How It Works

LootGlow hooks the container graphics/loading path and tracks loaded container references. When a container is loaded, LootGlow checks its inventory for qualifying loot.

If the container contains enough gold, LootGlow applies the gold glow. If the container contains valuable non-gold loot, LootGlow applies the high-value glow. These glow states are tracked separately, so removing one type of loot should remove only the matching glow.

LootGlow also hooks the target/mini-menu information path as a correction path. This allows the plugin to notice when a highlighted container's contents change after it has already been scanned. If qualifying loot is removed, the matching glow is finished and allowed to dissipate.

## Limitations

- Containers are scanned when their graphics load.
- Inventory changes are corrected when the container is targeted/opened through the game's normal interaction path.
- Loose gold is intentionally ignored; LootGlow is designed to highlight containers, not individual gold piles.
- High-value detection is an estimate intended to match the practical in-game value of loot closely enough for container highlighting.
- Compatibility with other mods that deeply alter the same container/menu or shader-effect paths is not guaranteed.

## Logging

LootGlow writes a small startup log entry showing the loaded version and main configuration.

By default, routine container detection is not logged. This keeps logs small during normal gameplay.

Advanced logging settings are supported for troubleshooting, but they are not included in the default INI.

## Known Good Defaults

The `v0.2.0` baseline is designed around these settings:

```ini
[LootGlow]

GoldCountThreshold=100
HighValueMode=1
HighValueThreshold=250
```

These defaults highlight containers with meaningful gold or valuable loot while ignoring very small gold amounts and low-value clutter.

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

## Version History

### v0.2.0

- Adds high-value loot detection for containers.
- Adds a separate high-value loot glow.
- Supports enchanted/generated item value estimates.
- Adds configurable gold threshold with `GoldCountThreshold`.
- Simplifies the default INI to the main user-facing settings.
- Keeps gold and high-value glow states independent.
- Reduces default log verbosity.
- Preserves automatic scanning, delayed rescans, and glow removal after looting.

### v0.1.0

- Initial public baseline.
- Automatically highlights loaded containers that contain gold.
- Removes glow after gold is looted.
- Adds configurable gold form ID, shader form ID, stack count, and debug logging.
- Uses quiet logging by default.

## Troubleshooting

### The mod does not appear to load

- Confirm OBSE64 is installed and working.
- Confirm `LootGlow.dll` is in the correct `OBSE\Plugins` folder.
- Check the plugin log for:

```text
LootGlow v0.2.0.0
```

### No containers are glowing

- Confirm the container actually has enough gold or high-value loot.
- Lower `GoldCountThreshold` if you want smaller gold amounts to glow.
- Lower `HighValueThreshold` if you want more valuable-loot containers to glow.
- Confirm `HighValueMode=1` if you want high-value non-gold loot highlighting.

### Too many containers are glowing

- Raise `GoldCountThreshold` to require more gold.
- Raise `HighValueThreshold` to require more valuable loot.
- Set `HighValueMode=0` if you only want gold-based highlighting.

### I only want gold containers to glow

Set:

```ini
HighValueMode=0
```

### I want any gold amount to glow

Set:

```ini
GoldCountThreshold=1
```

## Uninstall

Remove these files from the OBSE Plugins folder:

```text
LootGlow.dll
LootGlow.ini
```

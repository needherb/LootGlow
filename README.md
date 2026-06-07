# LootGlow

**LootGlow** is an OBSE64/CommonLibOB64 plugin for **Oblivion Remastered** that automatically highlights loaded containers that contain gold.

When a loaded container contains gold, LootGlow applies a configurable glow shader to that container. When the gold is looted, the glow is removed. Non-gold containers are ignored.

This first release focuses on gold-containing containers only. Loose gold in the world is intentionally ignored.

## Features

- Automatically scans loaded containers.
- Highlights containers that contain gold.
- Removes the glow after gold is looted.
- Ignores non-gold containers.
- Ignores loose gold by design.
- Uses a configurable effect shader.
- Supports a small INI file for basic tuning.
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
GoldFormID=0000000F
ShaderFormID=000C793F
StackCount=8
DebugLogging=0
```

### Settings

#### `GoldFormID`

The form ID treated as gold.

Default:

```ini
GoldFormID=0000000F
```

#### `ShaderFormID`

The TESEffectShader form ID used for the container glow.

Default:

```ini
ShaderFormID=000C793F
```

#### `StackCount`

The number of shader instances applied per highlighted container.

Default:

```ini
StackCount=8
```

Lower values reduce intensity. Higher values increase intensity, though there may be diminishing returns.

#### `DebugLogging`

Enables extra troubleshooting logs.

Default:

```ini
DebugLogging=0
```

Recommended for normal use:

```ini
DebugLogging=0
```

For troubleshooting:

```ini
DebugLogging=1
```

## How It Works

LootGlow hooks the container graphics/loading path and tracks loaded container references. When a container is loaded, LootGlow checks its inventory for the configured gold form ID. If the container has a positive gold count, the plugin applies a `MagicShaderHitEffect`-based glow using the configured shader.

LootGlow also hooks the target/mini-menu information path as a correction path. This allows the plugin to notice when a highlighted container's contents change after it has already been scanned. If the gold is removed, the glow is finished and allowed to dissipate.

## Limitations

- Containers are scanned when their graphics load.
- Inventory changes are corrected when the container is targeted/opened through the game's normal interaction path.
- Loose gold is intentionally ignored; LootGlow is designed to highlight containers, not individual gold piles.
- The default visual style depends on shader form ID `000C793F`.
- This release only targets gold-containing containers.
- Compatibility with other mods that deeply alter the same container/menu or shader-effect paths is not guaranteed.

## Logging

LootGlow writes a small startup log entry showing the loaded version and configuration.

By default, routine container detection is not logged. This keeps logs small during normal gameplay.

Set `DebugLogging=1` in `LootGlow.ini` only when troubleshooting.

## Known Good Defaults

The `v0.1.0` baseline was tested with:

```ini
[LootGlow]
GoldFormID=0000000F
ShaderFormID=000C793F
StackCount=8
DebugLogging=0
```

These defaults highlight gold-containing containers with a strong red/pink smoky glow.

## Building From Source

LootGlow is built as an OBSE64/CommonLibOB64 plugin.

The development build environment used for the initial release included:

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
LootGlow v0.1.0 initialized
```

### The glow is too strong or too faint

- Try changing `StackCount` in `LootGlow.ini`.
- Lower values reduce intensity.
- Higher values increase intensity, though there may be diminishing returns.

### The wrong object is glowing

- Enable `DebugLogging=1`.
- Reproduce the issue.
- Check the log for the configured `GoldFormID` and `ShaderFormID`.
- Confirm the INI is being loaded from the expected location.

## Uninstall

Remove these files from the OBSE Plugins folder:

```text
LootGlow.dll
LootGlow.ini
```

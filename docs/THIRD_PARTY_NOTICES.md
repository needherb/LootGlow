# Third-Party Notices

This document records third-party projects, tools, and reference materials that were important to the development of LootGlow.

LootGlow is an independent OBSE64/CommonLibOB64 plugin for Oblivion Remastered. Unless explicitly stated otherwise, LootGlow does not redistribute third-party assets, binaries, or source files from the projects listed below.

## CommonLibOB64

LootGlow is built using CommonLibOB64.

CommonLibOB64 is licensed under the GNU General Public License v3.0. Because LootGlow is built using CommonLibOB64, LootGlow source code is released under GPL-3.0-compatible terms.

See the repository `LICENSE` file for the full GPL-3.0 license text.

## OBSE64

LootGlow is an OBSE64 plugin.

OBSE64 is required separately and is not redistributed with LootGlow.

Based on the OBSE64 licensing information reviewed during development:

- `obse64/PluginAPI.h` is distributed under a permissive MIT-style license.
- The remainder of OBSE64 is not generally licensed as permissive open-source software.

LootGlow does not bundle OBSE64. Users must install OBSE64 separately.

## powerof3 — Unread Books Glow

Unread Books Glow, created by powerof3, was an important reference during LootGlow development.

Specifically, it helped inform the approach for applying persistent glow effects through the game's `MagicShaderHitEffect` path, including initializing the effect and inserting it into the game's magic effect processing list.

LootGlow does not redistribute Unread Books Glow assets. LootGlow does not convert Unread Books Glow to another game. LootGlow credits Unread Books Glow as an important reference project studied during development.

Credit is given to powerof3 as the creator of Unread Books Glow.

Permission notes reviewed during development included:

- Upload permission granted with credit.
- Modification permission granted with credit.
- Asset use permission granted with credit.
- Conversion to other games not permitted.
- Use in sold mods/files not permitted.
- Donation Points permitted.

LootGlow is not a paid mod and does not include assets from Unread Books Glow.

## Wolfmark — Container and Inventory Menu Fix

Container and Inventory Menu Fix, created by Wolfmark, was an important reference during LootGlow development.

Specifically, it helped inform the approach for reading Oblivion Remastered container inventory state, including container inventory access patterns, `InventoryChanges` handling, and the target/mini-menu information path used as a correction path when container contents change.

LootGlow does not redistribute Container and Inventory Menu Fix assets. LootGlow credits Container and Inventory Menu Fix as an important reference project studied during development.

Credit is given to Wolfmark as the creator of Container and Inventory Menu Fix.

Permission notes reviewed during development included:

- Upload permission granted with credit.
- Modification permission granted.
- Conversion permission granted with credit.
- Asset use permission granted.
- Use in sold mods/files not permitted.
- Donation Points permitted.

LootGlow is not a paid mod and does not include assets from Container and Inventory Menu Fix.

## No Third-Party Asset Redistribution

LootGlow's release package is intended to contain only:

- `LootGlow.dll`
- `LootGlow.ini`
- Documentation files such as `README.txt` or `README.md`

LootGlow does not bundle OBSE64, CommonLibOB64, Unread Books Glow, Container and Inventory Menu Fix, or assets from those projects.

## Source Code License

LootGlow source code is released under the GNU General Public License v3.0.

See `LICENSE` for the full license text.

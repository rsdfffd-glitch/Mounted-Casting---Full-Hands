# Mounted Casting - Full Hands (experimental bridge)

Goal: while mounted, use the normal attack controls for spells with no extra key, MCM, Papyrus, powers, horse systems, or UI.

Expected mapping:
- Primary attack / LMB: spell equipped in the right hand.
- Secondary attack / RMB: spell equipped in the left hand.
- Hold both: vanilla dual-cast path, provided the two equipped spells and perks allow it.
- On foot: the DLL does nothing.

The plugin only writes the animation graph booleans `bWantCastRight` and `bWantCastLeft` while the player is mounted. The included behavior patch already contains the mounted magic graph, BeginCastLeft/Right events, and dual-magic states.

## Important
This source has not been field-tested in Skyrim yet. It is intentionally a minimal experimental implementation. Test on a disposable save first. If left-hand or dual casting fails, the next revision should bridge the magic-caster state as well rather than adding unrelated features.

## Requirements
- Skyrim SE/AE 1.6.1170 target
- SKSE64
- Address Library for SKSE Plugins (through CommonLib runtime relocation)
- Pandora (run after enabling the mod)

## Credits / source basis
- Mounted behavior assets: Animated Mounted Casting by Zartar / Knight Life. Original Nexus mod permissions allow modification/reuse with credit.
- Input-event research/reference: DualCastHotkey by Shikis01 (MIT).
- Build/runtime library: CommonLibSSE-NG.

This package does not redistribute DualCastHotkey.dll.

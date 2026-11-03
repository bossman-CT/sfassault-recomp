# Status and roadmap

**Current state: mission 1 loads and plays, at 60 fps.** Milestone 0
is done apart from the frame-rate measurements. The Windows build compiles 7,650 recompiled
functions from `main.dol` and 21 of the 23 modules (`m7` and `m009` wait on #22). The game
initializes the SDK, links its modules with `OSLink`, takes keyboard input past its first
dialog, shows the title screen, creates a save on the emulated memory card, opens User
Settings and runs the mission 1 briefing and mission, played with a DS4 through DS4Windows.
Finishing mission 1 is next.

## Reference disc

| | |
|---|---|
| Disc ID | `GF7E01` (USA), revision 0 |
| Disc SHA-1 | `065341b8575adea00fd0a75e389f135d32840423` (1,459,978,240 bytes) |
| `main.dol` SHA-1 | `ae7201e7acd5a29acc807d7668a856392fd37ff9` |
| Code modules | 23 `.rel` files, linked at run time with `OSLink` |
| Symbol maps on the disc | None: 962 files, the only text file a 68-byte sound `.ini` |
| Redump | Matches disc 1857: size, CRC-32 07166d23, MD5 and SHA-1 |

Each milestone has a matching GitHub milestone, and each item has an issue. Pick an
issue, say you're working on it, and open a pull request when you're done (see
[CONTRIBUTING.md](../CONTRIBUTING.md)).

## Milestone 0: Analysis

Everything the recompiler needs to know about the game's executable.

- [x] Record the reference disc: `GF7E01`, its revision and the SHA-1 of a clean dump
- [x] Extract `main.dol` and record its SHA-1 in `CMakeLists.txt`
- [x] Generate `analysis/config.yml` and `analysis/symbols.txt` with decomp-toolkit
- [x] Symbols for the 23 relocatable modules, and `OSLink`/`OSUnlink` named for the recompiler
- [x] Identify the SDK functions for `recomp/hle.txt` and `recomp/special_calls.txt`
- [x] Find the scheduler's idle loop for `recomp/idle.txt`
- [x] Check whether the disc ships any symbol or map files that would name functions
- [ ] Confirm the game's native frame rate, per mode (menus, flight, on foot, multiplayer)

## Milestone 1: Boots

- [x] Configure and build on Windows with clang (LLVM-MinGW)
- [ ] Reach the Nintendo and Namco logos
- [x] Reach the title screen
- [ ] Reach the main menu
- [x] Create a save on the emulated memory card
- [ ] Load that save after a restart
- [ ] Audio plays: music and sound effects
- [x] Keyboard input works through SDL
- [x] Controller input works through SDL (DS4 via DS4Windows)

## Milestone 2: First mission

- [x] Mission 1's briefing and mission load and play
- [ ] Finish mission 1, start to end, including cutscenes
- [ ] No crashes or hangs over a full run
- [ ] Compare against Dolphin and record any visual differences

## Milestone 3: Full game

- [ ] Every story mission completable
- [ ] Versus multiplayer, including 4-player split screen
- [ ] Unlockables work: medals, versus content and bonus extras
- [ ] Builds and runs on Linux and macOS

## Milestone 4: Enhancement menu and core options

The goal is a port as feature-rich as Ship of Harkinian: everything below is a toggle
in an in-game menu, off by default where it changes the original experience. The menu
comes first because every later feature plugs into it. A menu framework that isn't
specific to this game should go upstream to gcn-recomp so other ports can use it.

- [ ] In-game menu bar (Dear ImGui), opened with a hotkey or controller combo, with
      settings saved to a config file
- [ ] Graphics: internal resolution, anti-aliasing, VSync, fullscreen and window modes
- [ ] Widescreen: projection, HUD placement and culling
- [ ] High and unlocked frame rates, from the game's per-frame steps (`recomp/steps.txt`)
- [ ] Upscaling: FSR first (vendor-neutral), DLSS after it if motion vectors are feasible
- [ ] Controls: per-controller remapping, keyboard and mouse, mouse and gyro aiming,
      rumble, several controllers for versus
- [ ] Audio: separate music, effects and voice volume

## Milestone 5: Enhancements, cheats and mods

- [ ] Quality of life: skip logos and intros, skippable cutscenes, faster menus
- [ ] Gameplay options: difficulty tweaks, invert-axis choices per vehicle
- [ ] Cheats: infinite health, ammo and bombs, unlock everything
- [ ] Cosmetics: Arwing, Landmaster and character color editors
- [ ] Save editor in the menu
- [ ] Texture packs: load HD replacement textures, including existing Dolphin packs
- [ ] Mod loader for audio and other asset replacements
- [ ] Speedrun tools: in-game timer and mission splits
- [ ] Developer tools: FPS and frame-time overlay, object viewer, free camera

## Milestone 6: Other versions

- [ ] PAL (`GF7P01`)
- [ ] Japan (`GF7J01`)

## Known problems

- Modules `m7` and `m009` aren't built, because they share module ID 11 with `m8` (#22).
- System-font text has almost no space between words (#25).
- Characters' mouths don't move during briefing transmissions (#26).

# Status and roadmap

**Current state: mission 1 can be finished, and mission 2 plays, at 60 fps.** Milestone 0
is done apart from the frame-rate measurements. The Windows build compiles 7,650 recompiled
functions from `main.dol` and 21 of the 23 modules (`m7` and `m009` wait on #22). The game
initializes the SDK, links its modules with `OSLink`, saves to the emulated memory card, and
plays mission 1 from the briefing through the boss (fixed in #42: the FPU's non-IEEE mode)
into mission 2, played with a DS4 through DS4Windows. An in-game menu (F1), widescreen and
players 2–4 are in. Later missions and versus mode are next to test.

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
- [ ] Confirm the game's native frame rate, per mode (menus, flight, on foot, multiplayer):
      menus and flight 60, on foot and cutscenes 30 (`docs/dev/frame-rate.md`); multiplayer
      not yet

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
- [x] Finish mission 1, start to end, including cutscenes (played through the boss into mission 2 on the #42 fix)
- [ ] No crashes or hangs over a full run
- [ ] Compare against Dolphin and record any visual differences

## Milestone 3: Full game

- [ ] Every story mission completable
- [ ] Versus multiplayer, including 4-player split screen
- [ ] Unlockables work: medals, versus content and bonus extras
- [ ] Builds and runs on Linux and macOS
- [x] Build from your own disc with one script: `build.bat` checks the tools and the disc,
      then builds (#34)
- [ ] A release ZIP whose launcher fetches the tools itself, so players install nothing by
      hand (#34)

## Milestone 4: Enhancement menu and core options

The goal is a port as feature-rich as Ship of Harkinian: everything below is a toggle
in an in-game menu, off by default where it changes the original experience. The menu
comes first because every later feature plugs into it. A menu framework that isn't
specific to this game should go upstream to gcn-recomp so other ports can use it.

- [x] In-game menu (Dear ImGui), opened with F1, Esc, a controller's Back or L + R + Start,
      with settings saved to `settings.txt` (see `docs/dev/menu.md`)
- [ ] Graphics: internal resolution, anti-aliasing, VSync, fullscreen and window modes
      (internal resolution, fractional or matched to the window, and fullscreen are in the
      menu)
- [x] Widescreen: projection, HUD placement and culling (`--aspect=16:9`, or Graphics >
      Aspect ratio in the menu; see `docs/dev/widescreen.md`)
- [ ] High and unlocked frame rates, from the game's per-frame steps (`recomp/steps.txt`)
- [ ] Upscaling: FSR first (vendor-neutral), DLSS after it if motion vectors are feasible
- [ ] Controls: per-controller remapping, keyboard and mouse, mouse and gyro aiming,
      rumble, several controllers for versus
- [ ] Audio: separate music, effects and voice volume
- [x] Audio quality: a clean 32-to-48 kHz step, the DSP's own voice filters, and an optional
      Enhanced resampling (Audio > Quality; see `docs/dev/audio.md`)

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
- [ ] Online versus, delay-based lockstep (#36). Phase 1 found that two instances fed the same
      inputs stay identical, at any internal resolution and aspect ratio
      ([netplay.md](dev/netplay.md)). Phases 2–4 (localhost, internet, menu) are still to do

## Milestone 6: Other versions

- [ ] PAL (`GF7P01`)
- [ ] Japan (`GF7J01`)

## Known problems

- Modules `m7` and `m009` aren't built, because they share module ID 11 with `m8` (#22).
- System-font text has almost no space between words (#25).
- On Fortuna, enemies and obstacles fade in late, and the central tower appears late (#28).

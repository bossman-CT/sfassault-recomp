# Status and roadmap

**Current state: nothing runs yet.** The project is set up, and milestone 0 is next.

Each milestone has a matching GitHub milestone, and each item has an issue. Pick an
issue, say you're working on it, and open a pull request when you're done (see
[CONTRIBUTING.md](../CONTRIBUTING.md)).

## Milestone 0: Analysis

Everything the recompiler needs to know about the game's executable.

- [ ] Record the reference disc: `GF7E01`, its revision and the SHA-1 of a clean dump
- [ ] Extract `main.dol` and record its SHA-1 in `CMakeLists.txt`
- [ ] Generate `analysis/config.yml` and `analysis/symbols.txt` with decomp-toolkit
- [ ] Identify the SDK functions for `recomp/hle.txt` and `recomp/special_calls.txt`
- [ ] Find the scheduler's idle loop for `recomp/idle.txt`
- [x] Check whether the disc ships any symbol or map files that would name functions:
      it doesn't (`tools/list_disc_files.py --unusual`)
- [ ] Confirm the game's native frame rate, per mode (menus, flight, on foot, multiplayer)

## Milestone 1: Boots

- [ ] Configure and build on Windows with clang
- [ ] Reach the Nintendo and Namco logos
- [ ] Reach the title screen and main menu
- [ ] Load and save a profile on the emulated memory card
- [ ] Audio plays: music and sound effects
- [ ] Controller input works through SDL

## Milestone 2: First mission

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

None yet, because nothing runs.

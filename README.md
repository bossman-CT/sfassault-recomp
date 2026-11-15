# sfassault-recomp

A native PC port of **Star Fox Assault** (GameCube, 2005), made by statically recompiling
the game's PowerPC code to C with [gcn-recomp](https://github.com/ethannicholas/gcn-recomp).

> [!IMPORTANT]
> **This repository contains no game code or data.** You need your own copy of the game.
> The build reads your disc image and produces the recompiled code on your machine; none
> of it is ever committed or distributed.

## Status

**In progress: mission 1 is playable on Windows.** The game boots, saves to its memory
card, and plays mission 1 at 60 fps, with sound, talking portraits, controller support and
readable menus. There's no packaged release yet: you build it yourself from your own disc
(below). See [docs/status.md](docs/status.md) for the roadmap and known problems, and the
[issues](../../issues) for what's being worked on.

**Works today:**
- Native Windows build, no emulator
- Any internal resolution (`--scale=N`) and fullscreen (F11 or Alt+Enter)
- Keyboard and game controllers through SDL

**Being worked on:** widescreen (#11), 120 fps (#12), an in-game options menu (#10), and a
launcher so you don't need to build from source (#34).

## Planned features

The aim is a port as feature-rich as Ship of Harkinian, with everything adjustable from
an in-game menu:

- Native Windows, Linux and macOS builds, with no emulator
- Any internal resolution, widescreen, and high or unlocked frame rates
- Upscaling (FSR, then DLSS where possible)
- Modern controls: remapping, keyboard and mouse, mouse and gyro aiming
- Quality-of-life options, cheats, cosmetics and a save editor
- HD texture packs and mods
- Speedrun and developer tools
- PAL and Japanese versions after the USA version

## Supported versions

| Region | Disc ID | Status |
|---|---|---|
| USA | `GF7E01` | In progress |
| Europe | `GF7P01` | Planned |
| Japan | `GF7J01` | Planned |

## Getting your game files

1. Dump **your own** disc with a Wii or a GameCube running homebrew, using
   [CleanRip](https://wiki.dolphin-emu.org/index.php?title=Ripping_Games). Don't
   download images: that's piracy, and those dumps are often modified.
2. If your dump is `.rvz`, `.gcz` or `.wia`, convert it to `.iso` in Dolphin
   (right-click the game, **Convert File...**, format **ISO**).
3. Put it in [`rom/`](rom/README.md), for example `rom/game.iso`.

## Building

Requirements: CMake 3.20+, Ninja, Python 3, Git, and clang with C++20 support. SDL2 is
built automatically on Windows the first time. On Windows, use
[LLVM-MinGW](https://github.com/mstorsjo/llvm-mingw), which brings its own Windows
libraries (plain LLVM also needs Visual Studio's Build Tools):

```powershell
winget install Kitware.CMake Ninja-build.Ninja MartinStorsjo.LLVM-MinGW.UCRT Python.Python.3.13
```

Known to build with CMake 4.4.4, Ninja 1.13.2, LLVM-MinGW 20261020 (clang 23.1.4, UCRT)
and Python 3.13.15.

```sh
git clone --recursive https://github.com/bossman-CT/sfassault-recomp.git
cd sfassault-recomp
```

Then, on Windows:

```powershell
.\build.ps1
```

On Linux or macOS:

```sh
./build.sh
```

The game is `build/sfassault.exe` (`build/sfassault` elsewhere). It finds the disc image
it was built from, so it needs no arguments. `--scale=N` sets the internal resolution
multiplier, and `--fullscreen` starts in fullscreen. While playing, **F11** or **Alt+Enter**
toggles fullscreen and **Esc** quits.

## Testing and feedback

Tried it? Tell us how it went:

- **Bugs:** open an [issue](../../issues/new/choose) with the bug report form.
- **Questions, ideas and impressions:** post in [Discussions](../../discussions).

Every run records your controller input in `saves/inputs/<date-time>/`, and a recording
replays the run exactly, which is the fastest way for us to reproduce a bug. You can attach
a run's `inputs.txt` to a bug report. **Don't attach `memcard_a.raw`**: memory card data
includes game assets, such as the save icon.

## Contributing

Pull requests are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md) first: the most
important rule is that **no game data, disc images, extracted files or generated code**
can ever go into an issue or pull request.

Issues labeled [`good first issue`](../../issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)
are a good place to start.

## Credits

- [gcn-recomp](https://github.com/ethannicholas/gcn-recomp) by Ethan Nicholas: the
  recompiler and GameCube runtime this port is built on (MIT)
- [decomp-toolkit](https://github.com/encounter/decomp-toolkit) and
  [dtk-template](https://github.com/encounter/dtk-template) by encounter, for analyzing
  the executable
- [gabefletch/StarFoxAssault](https://github.com/gabefletch/StarFoxAssault): the
  `main.dol` symbols and splits in `analysis/` (CC0)
- [xSNAIRZx/star-fox-assault-recomp](https://github.com/xSNAIRZx/star-fox-assault-recomp):
  the relocatable-module symbols in `analysis/rels/` (MIT, see
  [analysis/rels/LICENSE](analysis/rels/LICENSE))

## Legal

This is an independent fan project, not affiliated with or endorsed by Nintendo or
Bandai Namco. *Star Fox* is a trademark of Nintendo. The project's own code is under
the [MIT license](LICENSE), which grants no rights in the game itself.

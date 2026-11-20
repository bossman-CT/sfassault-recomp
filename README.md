# sfassault-recomp

A native PC port of **Star Fox Assault** (GameCube, 2005), made by statically recompiling
the game's PowerPC code to C with [gcn-recomp](https://github.com/ethannicholas/gcn-recomp).

> [!IMPORTANT]
> **This repository contains no game code or data.** You need your own copy of the game.
> The build reads your disc image and produces the recompiled code on your machine; none
> of it is ever committed or distributed. This is an independent fan project, not
> affiliated with or endorsed by Nintendo or Namco; see [Legal](#legal).

## Quick start (Windows)

The built game can't be shared, because it's made from the disc's code. So everyone builds
it on their own PC from their own disc. That takes one script. The first build took a
minute on an 8-core Ryzen 7 9800X3D; expect several minutes on a slower PC, plus a few more
the very first time while it builds SDL2 (once per PC).

1. **Install the tools** (once), in PowerShell or Terminal:

   ```powershell
   winget install Git.Git Kitware.CMake Ninja-build.Ninja MartinStorsjo.LLVM-MinGW.UCRT Python.Python.3.13
   ```

2. **Get the source.** Open a new PowerShell window where you want the game (for example
   `cd C:\Games`) and run the line below. It makes a `sfassault-recomp` folder there: the
   project folder. Don't use GitHub's ZIP download, which leaves out the recompiler (a
   submodule).

   ```powershell
   git clone --recursive https://github.com/bossman-CT/sfassault-recomp.git
   ```

3. **Add your disc.** Dump **your own** *Star Fox Assault* (USA, `GF7E01`) and put the
   `.iso` in the `rom\` folder ([how](#getting-your-game-files)). The build checks that it's
   the supported version: revision 0, SHA-1 `065341b8575adea00fd0a75e389f135d32840423`.

4. **Build.** Double-click **`build.bat`** in the project folder (or run
   `.\tools\build.ps1`). It checks the tools and the disc first and says exactly what's
   missing, then builds. After an update (`git pull --recurse-submodules`), run it again;
   it only rebuilds what changed.

5. **Play.** Start `build\sfassault.exe` with the project folder as the working folder:

   ```powershell
   cd C:\Games\sfassault-recomp
   .\build\sfassault.exe
   ```

   Double-clicking `sfassault.exe` inside `build\` also starts it, but keeps its saves and
   settings in `build\` instead. A Steam shortcut (below) or a Windows shortcut with
   **Start in** set to the project folder avoids that.

   **F1** or **Esc** (or a controller's Back/View button) opens the menu: graphics,
   controls, and quitting. **F11** toggles fullscreen.

**Where things live**, all in the project folder: your memory card (the game's saves) in
`saves\memcard_a.raw`, what you change in the menu in `settings.txt`, your controls in
`controls.txt` if you make one ([Controls](#controls)), and a recording of every run's
input in `saves\inputs\`.

**Add it to Steam:** in Steam, **Games > Add a Non-Steam Game to My Library > Browse**,
pick `build\sfassault.exe`, and add it. Then right-click it, **Properties**, and set
**Start In** to the project folder (for example `"C:\Games\sfassault-recomp\"`, quotes
included). Steam fills in the `build` folder, where the game wouldn't find its saves and
settings.

## Status

**In progress: mission 1 is playable on Windows.** The game boots, saves to its memory
card, and plays mission 1 at 60 fps, with sound, talking portraits, controller support and
readable menus. There's no packaged release yet: you build it yourself from your own disc
(below). See [docs/status.md](docs/status.md) for the roadmap and known problems, and the
[issues](../../issues) for what's being worked on.

**Works today:**
- Native Windows build, no emulator
- Any internal resolution (`--scale=N`, fractions such as `2.25` too), or the window's own
  (`--scale=window`, so a 1440p or 4K screen is drawn natively), and fullscreen (F11 or Alt+Enter)
- Keyboard and up to four game controllers through SDL, one per port, with remapping, a
  PlayStation-style face-button layout and gyro aiming ([Controls](#controls))

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

Playing on Windows? The [Quick start](#quick-start-windows)'s `build.bat` does all of
this. The details, for other platforms and for development:

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
multiplier (`2.25` works too), `--scale=window` draws at the window's resolution and follows
it, `--aspect=16:9` turns on widescreen, and `--fullscreen` starts in fullscreen. The menu's
Graphics page has the same settings.
While playing, **F11** or **Alt+Enter**
toggles fullscreen and **Esc** quits.

## Controls

The keyboard and the first controller play on port 1. Each controller you connect after it
gets the next port, up to four, and keeps its port until it's unplugged. The game prints
which controller went where when it starts.

To change the controls, copy [`controls.example.txt`](controls.example.txt) to
`controls.txt` next to where you run the game and uncomment what you want. Everything is
per port, or for one kind of controller:

- **Face buttons:** `layout = modern` makes the bottom button confirm and the right one go
  back (Cross = A, Circle = B). The default, `positional`, keeps the GameCube's arrangement.
- **Remapping:** any GameCube control from any button, key or joystick: `A = gamepad circle`.
- **Gyro aiming** on a DS4, DualSense or Switch Pro controller: `gyro = stick`, with
  sensitivity, dead zone, smoothing, invert and an optional hold-to-aim button.
- **A second player on the keyboard:** `[port 2]` with `connected = always` and some keys.

`sfassault.exe --list-controls` shows each controller's name and what each button is
called. With DS4Windows running, the game can see your DS4 twice (the DS4 and the Xbox pad
DS4Windows makes), on two ports. Close DS4Windows, or have it hide the DS4. With DS4Windows
closed, the game reads the DS4 directly, gyro included.

## Testing and feedback

Tried it? Tell us how it went:

- **Bugs:** open an [issue](../../issues/new/choose) with the bug report form.
- **Questions, ideas and impressions:** post in [Discussions](../../discussions).

**Never attach game files to an issue.** That includes recordings and routes
(`saves/inputs/`, `saves/routes/`), the memory card (`memcard_a.raw`) and frame dumps: they
hold game data, such as the save icon and images of the game. Screenshots and short clips
are fine. The bug report form asks when in your run the bug happened instead.

## Contributing

Pull requests are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md) first: the most
important rule is that **no game data, disc images, extracted files or generated code**
can ever go into an issue or pull request.

Issues labeled [`good first issue`](../../issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)
are a good place to start, and [Where help is wanted](CONTRIBUTING.md#where-help-is-wanted)
lists the bigger pieces we'd most like help with.

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

This is an independent fan project. It is **not affiliated with, endorsed by or sponsored
by Nintendo, Namco or Bandai Namco**.

- **You need your own, legally obtained copy of the game**, dumped from a disc you own.
  This project doesn't provide the game, and doesn't link to anywhere that does.
- **This repository contains no game data:** no code, assets, disc images or extracted
  files, only original code and documentation, with addresses and names that describe the
  executable. The game's code is recompiled on your machine, from your disc.
- **Don't share your build.** `build/` holds the game's recompiled code, and `saves/` holds
  your memory card and recordings, so neither may be uploaded or passed on. The project
  publishes no builds of the game.
- *Star Fox* and *Star Fox Assault* are trademarks of Nintendo. They're used here only to
  name the game this project ports. All other trademarks belong to their owners.

The project's own code is under the [MIT license](LICENSE), which grants no rights in the
game itself.

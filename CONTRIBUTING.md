# Contributing

Thanks for helping. This file covers the rules, how the project fits together, how to test
a change, and what a good pull request looks like.

## The one rule that can't bend: no game data

This project can be hosted only because it contains no part of the game. **Never** put
any of the following in a commit, issue, pull request, comment or attachment:

- Disc images, or links to anywhere that offers them
- Files extracted from the disc: `main.dol`, `.rel` files, textures, models, audio, movies
- Generated code: anything from `build/`, including the recompiled C
- Disassembly or decompiled code copied out of Ghidra, IDA, dtk or objdump
- Code from leaked Nintendo SDKs or other leaked material
- Anything from your `saves/` folder: recordings, routes, memory cards, regression
  baselines. They hold your memory card, which includes game assets, and images of the game
- Frame dumps, texture dumps, or the game's logos and artwork
- A build of the game, or instructions for sharing one: a build contains the game's code

Addresses, sizes, function names and short descriptions of what the game does are fine.
In issues and pull requests, screenshots and short clips of the running game that show a
bug are fine too, but don't commit them to the repository.

`tools/check_no_game_data.py` catches the common cases:

- disc images, and the game's own file formats
- memory-card and save files
- frame dumps, images, audio and video
- recompiled code, and pasted disassembly or decompiler output
- anything from `rom/`, `saves/` or a build directory
- large binaries

GitHub runs it on every pull request, on the result and on each commit in it, since a file
added in one commit and deleted in the next would still be in the history. Run it on your
own commits too, once per clone:

```sh
git config core.hooksPath .githooks
```

Never bypass it with `--no-verify`. It can't catch everything, so reviewers reject anything
that looks like game data. If it flags a file of yours that really is original (an icon you
drew, say), say so in the pull request: the file can be added to its `ALLOWED_PATHS`.

## How the project works

The game's executable (`main.dol`) and its relocatable modules (`.rel` files) are PowerPC
code. At build time, [gcn-recomp](https://github.com/bossman-CT/gcn-recomp) reads them from
your disc image, translates every function to C, and compiles that against its runtime,
which stands in for the GameCube's hardware: graphics, audio, controllers, memory card and
disc. Nothing from the disc is ever stored in this repository; the translated code only
exists in your `build/` folder.

| Folder | What's in it |
|---|---|
| `analysis/` | Symbols and splits for `main.dol` and the modules: where each function starts and ends |
| `recomp/` | The tables gcn-recomp reads for this game: functions it replaces, the idle loop, names, instruction patches, per-frame steps. See [recomp/README.md](recomp/README.md) |
| `src/` | Game-specific code in C++: frame rate, widescreen, controls, player modes. Patches in `recomp/patches.txt` call into it |
| `gcn-recomp/` | The recompiler and runtime, as a submodule |
| `docs/status.md` | The roadmap; the issues mirror it |
| `docs/dev/` | Design notes for each feature |
| `tools/` | Scripts: routes, regression checks, analysis |

### This game, or gcn-recomp?

The `gcn-recomp/` submodule is our fork,
[bossman-CT/gcn-recomp](https://github.com/bossman-CT/gcn-recomp), of Ethan Nicholas's
[gcn-recomp](https://github.com/ethannicholas/gcn-recomp), which other GameCube ports share.

- **A fix that isn't specific to Star Fox Assault**, such as a GX rendering bug, an audio
  problem or a missing hardware feature, goes in the fork. The fork must stay free of
  anything game-specific: no addresses, names or behavior of this game. Open the pull
  request against the fork's `main`, then a second one here that moves the submodule to it.
  The maintainers offer fork changes to the original project.
- **A fix that only makes sense for this game** goes here, in `recomp/` or `src/`. Every
  patch in `recomp/patches.txt` needs a comment saying what it fixes.

Report bugs here either way, with the `upstream` label if they're in the runtime.

## Building

Follow the [Quick start](README.md#quick-start) in the README, with your own disc image
in `rom/`. It lists what to install and the build commands.

## Picking up work

1. Find an issue. [`good first issue`](../../issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)
   and [`help wanted`](../../issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22)
   are good starting points, and [docs/status.md](docs/status.md) shows how they fit
   together.
2. Comment on the issue to say you're taking it, so two people don't do the same work.
3. If there's no issue for what you want to do, open one first for anything bigger than
   a small fix.

## Where help is wanted

- **Online multiplayer** ([#36](../../issues/36)): peer-to-peer versus, where only
  controller inputs cross the network. The design is in
  [docs/dev/netplay.md](docs/dev/netplay.md). This is the big one.
- **Upscaling** ([#13](../../issues/13)): FSR 1 first, then temporal upscaling.
- **HD texture packs** ([#14](../../issues/14)): load Dolphin-format packs that players
  supply themselves.
- **Versus bots** ([#50](../../issues/50)): computer opponents, at four difficulties.
- **Fortuna's slow fade-in** ([#28](../../issues/28)): enemies and obstacles appear late.
  A rendering investigation, with candidates listed in the issue.
- **System-font spacing** ([#25](../../issues/25)): words run together in some menus.
- **Native frame rates** ([#7](../../issues/7)): measure the original game in Dolphin and
  post a table. No code needed.

## Testing

GitHub's `check` only looks for game data. It can't build or run the game, because that
needs a disc, so **you test on your own machine and paste the result in your pull
request**.

You rarely need to play through the game to test a change. Every run is recorded, and a
recording replays exactly, so a *route* (a saved run) brings you back to any point. Routes
are made from your own play and stay in `saves/routes/`:

```powershell
.\tools\route.ps1 save fortuna-plaza     # save the newest run as a route
.\tools\route.ps1 play fortuna-plaza     # play on from where it ends
```

`tools\regress.ps1` replays every route and compares frames with a baseline:

1. Save routes that pass through what your change affects.
2. Build `main` and run `.\tools\regress.ps1 -Update` to record the baselines.
3. Build your branch and run `.\tools\regress.ps1`.
4. Paste the table it prints into the pull request. Each route is `SAME`, `CHANGED` or
   `STOPPED` (the replay ended early, usually a crash). Explain every `CHANGED`: a visual
   fix should change the frames it meant to, and nothing else.

Don't attach the frames, baselines or side-by-sides it makes: they're images of the game.
Describe what changed instead. [docs/dev/testing.md](docs/dev/testing.md) has more:
comparing two builds frame by frame, and checking that two runs stay identical.

For changes to docs or tooling only, say "not tested in game".

## Branches and pull requests

1. **One issue per branch.** Fork the repository and branch from `main`. Name the branch
   after the issue: its milestone in [docs/status.md](docs/status.md) (`m4` for
   Milestone 4) and a short name, such as `m4/widescreen-pause`.
2. **One change per pull request**, to `main`. Small pull requests get reviewed faster.
3. **Link the issue** (`Fixes #55`), and explain what changed and why.
4. **Say how you tested it**, with your `regress.ps1` result, which mission or menu, and
   your OS and GPU.
5. **Tick the roadmap item** in [docs/status.md](docs/status.md) in the same pull request,
   if it finishes one.

Keep your branch up to date by rebasing on `main`.

## Reporting bugs

Use the **Bug report** form. It asks for the commit you built, your `settings.txt`, steps
to reproduce, and when in the run the bug happened. Logs and screenshots help a lot.
Recordings, memory cards and frame dumps stay on your machine.

## AI-assisted contributions

Using AI tools is fine. You're still responsible for the change: understand it, test it
in the game, and be ready to answer questions about it in review.

## License

By contributing, you agree that your contribution is released under the project's
[MIT license](LICENSE).

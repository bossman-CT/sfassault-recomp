# Prior art

What other people have already built for Star Fox Assault and for gcn-recomp, what we can
reuse, and what to watch for. Researched on 2026-11-02 from public GitHub data only: every file
was read through the GitHub API at the default branch. Nothing was cloned, downloaded or run,
and xSNAIRZx's prebuilt `.exe` files were not opened. Findings about their code come from
comparing their files with gcn-recomp's by git blob hash and reading the differences.

This document holds names, addresses, sizes and descriptions only. No game code, no
disassembly, nothing from the disc.

## In short

- **Two other projects cover our disc (`GF7E01`).** [xSNAIRZx/star-fox-assault-recomp](https://github.com/xSNAIRZx/star-fox-assault-recomp)
  (MIT) boots to the first mission on Windows. [gabefletch/StarFoxAssault](https://github.com/gabefletch/StarFoxAssault)
  (CC0) is a decompilation that never got past the first dtk analysis.
- **The game is not a single executable.** Besides `main.dol` (5,375 functions) it links 23
  relocatable modules (2,315 functions) at run time. Upstream gcn-recomp stops at build time
  when two modules share a module ID, and xSNAIRZx patched that, and the matching run-time
  side, in a private copy of gcn-recomp's `runtime/`. Their changes only make sense if some
  of the 23 share an ID. We have not confirmed that: the module headers on our disc would
  settle it. If it is true, we need the same fix, ideally upstream.
- **Their symbols are not new work.** xSNAIRZx's `symbols.txt` is byte-identical to
  gabefletch's. Only 306 of the 5,375 `main.dol` functions have a name. The 23 per-module
  symbol files are xSNAIRZx's own.
- **Their two known bugs have no documented cause.** The disappearing first boss is not
  explained anywhere. The current-thread-pointer crash may be related to REL address reuse;
  see [Known bugs](#known-bugs).
- **Widescreen and resolution scaling exist, but only in their fork.** Upstream has no
  flat-screen widescreen. Their approach is described in
  [Resolution scaling and widescreen](#resolution-scaling-and-widescreen). Culling is
  untouched, and nothing exists for frame rate.
- **Prime and Blue Storm show what finished looks like:** a short CMake file, a handful of
  small tables, game code in `src/` or `runtime/`, and `docs/dev/` notes. Blue Storm is the
  template for frame rate.

| Project | Covers | License | State on 2026-11-02 |
|---|---|---|---|
| [xSNAIRZx/star-fox-assault-recomp](https://github.com/xSNAIRZx/star-fox-assault-recomp) | `GF7E01` | MIT for project code; GitHub reports `NOASSERTION`; see [Licenses](#licenses) | Created 2026-11-01, two commits, one prerelease, no issues. By its own report, first-mission flight works. |
| [gabefletch/StarFoxAssault](https://github.com/gabefletch/StarFoxAssault) | `GF7E01` | CC0-1.0 | Created 2026-10-07, last push 2026-10-11, no releases. |
| [ethannicholas/gcn-recomp](https://github.com/ethannicholas/gcn-recomp) | Any game | MIT | Active. Our submodule pins `9e93f1b`, which was still its head when checked. Three pull requests from another contributor are open. |
| [ethannicholas/prime-recomp](https://github.com/ethannicholas/prime-recomp) | Metroid Prime | MIT | Complete, beaten start to finish. |
| [ethannicholas/bluestorm-recomp](https://github.com/ethannicholas/bluestorm-recomp) | Wave Race: Blue Storm | MIT | Released. Runs at 30 or 60. |

`prime2-recomp` is listed in gcn-recomp's README, but its GitHub address returns 404 (private
or not yet published), so it is not covered here.

## xSNAIRZx/star-fox-assault-recomp

### What it is

An experimental Windows x64 port, version `0.2.0-rc.2`. The repository was created on
2026-11-01 and has two commits made 9 minutes apart ("Publish audited experimental v0.2.0-rc.2
source and host tools" and "Record project publishing preferences"), so there is no
development history to read. There is one prerelease: a 5 MB ZIP of source and host tools.
Its `CREDITS.md` notes AI-assisted development of the launcher and integration.

How it builds: a PowerShell launcher takes the user's `GF7E01` ISO or CISO, downloads pinned
LLVM-MinGW, CMake, Ninja and Python (SHA-256 pinned in `tools/portable-toolchain.json`) plus
SDL 2.30.9 source, runs CMake with `-DGAME_ISO`, and produces `assault.exe`. They report about
150 seconds and 101 Ninja units from an empty cache. They compile with clang from LLVM-MinGW
and link statically, which is the same direction as our #8.

What they say works: first-mission flight, readable menus, profile saving. Not verified:
mission completion, later missions, full audio, physical controllers. Widescreen is
"experimental".

| Path | Contents |
|---|---|
| `runtime/` | A vendored, modified copy of gcn-recomp (not a submodule). See [Changes to gcn-recomp](#changes-to-gcn-recomps-runtime). |
| `decomp/config/GF7E01/` | dtk configs and symbols for the DOL and 23 modules |
| `recomp/` | Four tables: `hle.txt`, `idle.txt`, `names.txt`, `special_calls.txt` |
| `CMakeLists.txt` | One `gcn_add_game()` call plus REL wiring |
| `tools/` | PowerShell build, toolchain and packaging scripts; `LauncherBootstrap.cs`; `native/assault-image-tool.exe` |
| `Launch-StarFoxAssault.ps1` | The launcher UI (a 108 KB script) |
| `assets/` | Launcher artwork, and two replacement GameCube system fonts |

### `decomp/config/GF7E01/`

- **`symbols.txt`**: 18,590 entries. 5,375 functions (about 2.04 MB of code, spanning
  `0x80003100` to `0x80200AC0`), 13,126 data objects and 89 labels. Only **306 functions have
  names**: the Metrowerks runtime start-up, MetroTRK, the `PPC*` processor-register
  helpers, `memcpy`, `memset` and `main`. The other 5,069 are `fn_<address>`, and 9,362 of the
  13,126 objects are `lbl_<address>`. This is dtk's first automatic pass; no SDK function such
  as `OSLink` is named. It is **byte-identical to gabefletch's file** (the same git blob).
- **`main-symbols.txt`**: the same list. It differs only in which entries carry an `align:`
  attribute (2,214 against 4,002).
- **`splits.txt`, `main-splits.txt`**: the section table (`.init` through `.sbss2`) and
  nothing else. There are no per-source-file splits.
- **`config.yml`, `main-only.yml`**: minimal dtk configs (object, symbols, splits,
  `mw_comment_version: 11`, which means CodeWarrior for GameCube 2.7+). `config.example.yml`
  is the dtk template's documentation. `build.sha1` is empty.
- **`assault-analysis.yml`**: the full dtk config with all 23 `modules:` entries. It records
  which modules import from which: `m2`, `m3` and `m5` link to `m4`; `mb02` and `mb10a` link
  to `m8`. Nothing else in the repository references this file.
- **`<module>/symbols.txt` and `<module>/splits.txt`** for 23 modules: `m0`, `m009`, `m1`
  to `m10`, `mb01a`, `mb01b`, `mb02`, `mb05`, `mb10a`, `mbg0`
  to `mbg2` and `md0` to `md2`. Together 2,315 functions, almost all `fn_<section>_<offset>`.
  The largest are `m4` (771), `m8` (464) and `m9` (293). Split files list sections only.
  `mbg0`, `mbg1` and `mbg2` have the same function count and near-identical sizes, as do
  `md0` to `md2`, so they look like variants of one module. That would explain the repeated
  module IDs below, but it is an inference from sizes, not something they state.

The DOL SHA-1 they use in `CMakeLists.txt`, `ae7201e7acd5a29acc807d7668a856392fd37ff9`,
matches the `hash:` in gabefletch's `config.yml`. Two files agreeing is weaker than two
independent dumps, so #2 still records ours from the disc.

### `recomp/`

| File | Contents | Compared with what gcn-recomp needs |
|---|---|---|
| `hle.txt` | `OSLoadContext`, `OSReport`, `OSResetSystem`, `OSSaveContext` | Missing `OSPanic`, which gcn-recomp lists as required. A panic would spin instead of printing. |
| `special_calls.txt` | `OSSaveContext` at `0x8012F7E4` | Complete. |
| `names.txt` | The same one entry | Names only `OSSaveContext`. `OSLink` and `OSUnlink` stay anonymous, which is presumably why they wrote the run-time fallback described below. |
| `idle.txt` | Empty | No idle-loop skip, so the guest spins through the scheduler's wait. #5 has an answer for this. |
| `patches.txt` | Absent | They needed no instruction patches. |
| `steps.txt` | Absent | No frame-rate work. `FPS 60` is passed for the benchmark only. |

The tables agree with #16 on `OSSaveContext` at `0x8012F7E4`. By their account the game
reaches the first mission with no instruction patches, so what they fixed, they fixed in the
runtime.

### Changes to gcn-recomp's runtime

Their `runtime/` is gcn-recomp with 97 files unchanged, **24 modified and 22 added** (10 test
files, 2 for a host tool, 9 sources and 1 doc). Four of upstream's repository-metadata
files (`.claude/settings.json`, `.gitattributes`, `.githooks/pre-commit`, `.gitignore`) are not
carried over. This assumes they forked at or just before `9e93f1b`, the commit our submodule
pins. Every difference read was an addition for this game's needs, so that looks right.

Most changes are not specific to Star Fox Assault. Under gcn-recomp's own rule ("nothing
game-specific belongs here") they are candidates for upstream; see
[Upstream candidates](#upstream-candidates).

| Area | Files | What changed |
|---|---|---|
| **Relocatable modules, build** | `recomp/rel.py` | A module ID may repeat. A module's import of its own ID resolves to its own file. An import from another module must pick exactly one variant by section and addend, or the build stops with a message naming the candidates. Upstream exits on any repeated ID. |
| **Relocatable modules, run time** | `runtime/rels.cpp`, `cpu.cpp`, `recomp.h`, `recomp/recomp.py`, `docs/rels.md` | A linked module is identified by ID, then by the name in the SDK's module string table, then by section sizes. A mapped code range is trusted only while the module header still matches the compiled variant. A new link retires any older range it overlaps. `rels_bind()` is a fallback: on a call to an unmapped address it walks the SDK module list at `0x800030C8`, finds the module whose code contains the address and registers it, for games whose `OSLink` has no name. The `OSUnlink` hook now runs the original first, so a module's epilog callbacks still resolve. |
| **System fonts** | `runtime/hw/exi.cpp`, `assets/fonts/` | The EXI IPL device serves two replacement fonts (Western and Japanese) at the ROM offsets the SDK's font loader reads. Upstream logs those reads as unsupported and returns zeros. `GCN_FONT_DIR` overrides the location. Their boot test loads both fonts and shows readable text on the health-and-safety screen. |
| **Text rendering** | `gx/shadergen.cpp`, `render_gl.cpp`, `xf.cpp`, `docs/graphics.md`, new `gx/texcoord.h` | The texture-coordinate scale now comes from the `SU_SSIZE`/`SU_TSIZE` registers of the coordinate slot (size minus one in the low 16 bits), not from the dimensions of the bound texture. The game's glyph atlas submits pixel-space coordinates with a scale of one texel; the old behavior sent them far outside the atlas and removed the glyphs. Indirect stages use the same scale. |
| **Display** | `gx/render_gl.cpp/.h`, `main.cpp`, new `gx/display_options.h`, `gx/render_math.h` | Fractional internal scale, aspect handling, FXAA and sharpening on the final blit, F8 and Alt+Enter, `--aspect`, `--scale`, `--aa`, `--filter`, `--vsync`, `--fullscreen`, `--volume`, per-monitor DPI awareness. See below. |
| **Audio** | `audio_sdl.cpp`, new `audio_gain.h` | A master volume applied to the mixed output. |
| **Disc** | `hw/di.cpp`, new `disc_image.*`, `disc_mods.*` | One reader for ISO and CISO with validation, and an overlay that replaces files read from the disc by same-path, **same-size** files under `GCN_MODS_DIR` (`docs/disc-mods.md`). `.dol`, `.rel` and `sys/` files are refused because that code is compiled in. |
| **Windows build** | `CMakeLists.txt`, new `assault.manifest` | An application manifest that makes the process use UTF-8 paths. A stricter validity check for the shared SDL build cache. Passing more toolchain variables through to that build. MinGW resource-compiler support. |
| **Portability** | `runtime/gx/xf.cpp`, many includes | `__builtin_ctz`/`__builtin_popcount` replaced by `<bit>` functions, and missing `<cstdlib>`/`<cstdio>`/`<cmath>` includes added in about ten files. These look like what stopped gcn-recomp compiling with LLVM-MinGW. Useful to read before #8. |
| **Diagnostics** | `threads.cpp`, `mmu.cpp`, `recomp.h`, `texture.cpp`, CMake options | `GCN_THREAD_WATCH` (a build option) reports stores that can overlap the OS current-thread word at `0x800000E4`, with the storing address and link register. The MMU's "no page table" fatal now prints the address, write flag, link register and current thread, and `GCN_RAM_DUMP=<path>` saves all guest RAM there. `GCN_TLUTDUMP=<address>` prints one texture's palette. |
| **Tests** | `runtime/tests/` | Unit tests for module rebinding, disc images, disc overlays, display math, texcoord scale, audio gain, UTF-8 paths and the thread-watch filter. |

### Known bugs

Their `README.md`, `VALIDATION.md`, `RELEASE-NOTES.md` and `CREDITS.md` all list two
problems as unresolved. None of them gives a cause, and there are no issues filed.

1. **The first boss disappears.** Described as a "first-boss visibility report". Nothing in
   the repository explains it, and no change in the runtime diff targets it. We cannot say
   whether it happens at 4:3 as well as in widescreen, or whether it is a rendering,
   culling or game-logic problem. Compare against Dolphin at the same point (the M2 item) before
   assuming it is ours or theirs.
2. **A crash on an invalid current-thread pointer.** Also unexplained in prose. The runtime
   changes show what they chased:
   - an opt-in build that watches writes near the OS current-thread word at `0x800000E4` and
     prints the old and new values;
   - the MMU fatal for an access with no page table, where a wild pointer read through a
     garbage thread value would end, now prints that word and can dump RAM;
   - a unit test, `rel_rebinding_test.cpp`, whose comment says the test's callback address is
     "the crash callback address" from an older module layout: a callback into code at an
     address that a different module variant had since taken over.

   Reading of the evidence (not their statement): a stale REL address mapping sent a callback into
   the wrong function, which then corrupted thread state. They added the stale-mapping checks
   in `rc.2`, and `VALIDATION.md` still lists the crash as open. If this is right, #9 will hit
   it too, and the REL fixes are the first thing to port.

### Resolution scaling and widescreen

Both live in `runtime/gx/render_gl.cpp` with the math in two small headers that have unit
tests.

**Resolution.** The GX embedded frame buffer is 640 by 528. The internal scale multiplies that,
and is now a float, so `1.5` works. All sizes and edges go through one rounding helper so that
adjacent scissor rectangles and EFB copies stay seamless at fractional scales. "Match window"
(`--scale=0`) picks the smallest half step that covers the output viewport, clamped to 1 to 8,
and re-evaluates as the window changes; half steps avoid reallocating render targets for
every pixel of a window drag. The finished frame is blitted to the window in a viewport
letterboxed to 4:3, 16:9 or stretched, with optional FXAA and a "sharp" or "smooth"
filter. Higher resolution sharpens geometry edges but does not replace textures.

**Widescreen.** Four mechanisms, all in the draw path:

- A perspective draw has its projection's x terms multiplied by `0.75` (4:3 divided by 16:9).
  The frame is rendered into the same 640 by 528 buffer and shown 16:9, so the picture
  gets wider rather than stretched.
- An orthographic draw is scaled the same way unless it covers the whole screen, so a HUD
  piece keeps its proportions and a full-screen fade or backdrop still fills the frame. From
  the code, it appears HUD pieces are not re-anchored to the screen edges; they stay near the
  center.
- Scissor rectangles of scaled orthographic draws are scaled about the viewport center by the
  same factor, and the z-freeze plane is fitted with it.
- Each frame is classified. If it contains no on-screen perspective draw (a purely 2D screen,
  for example), it is presented 4:3 inside the wide window; if it does, 16:9. Draws into
  off-screen copies are left alone. The VR paths bypass all of it.

What it does not do: the game still culls to its 4:3 camera, so objects at the wide edges
can pop in. They say as much ("game-specific culling limits"). Prime's patches show the way
to fix that (hooking the game's frustum set-up), and it is the main piece of work left for
#11.

### Licenses

| Part | License | Notes |
|---|---|---|
| Project code and docs (launcher, scripts, runtime additions, tools, tests) | MIT, "Copyright (c) 2026 Star Fox Assault Recomp contributors" | The top `LICENSE` has a paragraph before the MIT text saying the license covers "original software source code and integration code" and grants nothing in the game, artwork, fonts or third-party parts. GitHub's API reports `NOASSERTION`, probably because of that paragraph. |
| `runtime/` (gcn-recomp) | MIT, "Copyright (c) 2026 Ethan Nicholas" in `runtime/LICENSE` | The license file is byte-identical to upstream's. Their additions inside `runtime/` have no separate notice; treat them as covered by the top-level MIT. |
| `decomp/config/GF7E01/symbols.txt`, `splits.txt` | CC0-1.0 (from gabefletch) | The same file as gabefletch's, which is CC0. `CREDITS.md` names only dtk-template. |
| Per-module symbols and `assault-analysis.yml` | Unclear; probably the top-level MIT | Metadata, not "source code", so the top `LICENSE` does not obviously cover it. The README says only that "analysis metadata uses the CC0 decomp-toolkit template", which is about the dtk workflow, not this data. Assume MIT with credit, as the open M0 pull request (#16) does, and ask the author to confirm if it matters. |
| `recomp/*.txt` | Same as above | Four short address lists. |
| `assets/fonts/font_western.bin`, `font_japanese.bin` | Apache-2.0 | Byte-identical to the replacement fonts in Dolphin's `Data/Sys/GC` (same git blobs, same notice: generated with Dolphin's gc-font-tool, based on Droid Sans, copyright Google). Not Nintendo's IPL fonts. |
| `assets/*.png` (launcher artwork) | No grant | Generated images. The top `LICENSE` excludes artwork. Do not reuse. |
| SDL 2.30.9 | zlib | Fetched at build time. |
| glad loader (in `runtime/`) | `(WTFPL OR CC0-1.0) AND Apache-2.0` | Same as upstream. |
| Downloaded toolchains | Their own | LLVM-MinGW, CMake, Ninja, Python, pinned in `portable-toolchain.json`. |

### Prebuilt executables

The repository holds two compiled files. The release ZIP, which was not opened, is described
as containing the launcher and the inspector as well:

- **`Star Fox Assault Recomp.exe`** (17,920 bytes) is a bootstrap. Its source,
  `tools/LauncherBootstrap.cs`, is in the repository: it finds `Launch-StarFoxAssault.ps1`
  beside itself and starts Windows PowerShell on it with `-ExecutionPolicy Bypass`.
  `tools/Build-Launcher.ps1` builds it with the .NET Framework's `csc.exe`, and
  `tools/Build-LauncherIcon.ps1` draws its icon. The listing in `SHA256SUMS.txt` includes the
  executable and these sources.
- **`tools/native/assault-image-tool.exe`** (233 KB) is the read-only image inspector. Its
  source is `runtime/tools/native/main.cpp` with its `CMakeLists.txt`, and they use the
  shared disc reader.

So matching source exists in the repository for both. **This review could not confirm the
binaries are built from it:** that would take compiling it, which was out of scope, and
nothing in the repository claims a reproducible build. For the source files themselves, the
hashes in `SHA256SUMS.txt` match the repository for the five checked (the bootstrap, its
build script, the launcher script and the image tool's two source files). The game executable is
not distributed: it is built locally from the user's disc image. We do not run or ship either
file, and our project ships no binaries.

### What not to take from it

- The launcher, artwork and installer scripts: a different distribution model from ours.
- The copy-the-runtime approach. Vendoring gcn-recomp ties them to a snapshot; we should send
  generic fixes upstream and keep the submodule (our `CLAUDE.md` already says so).

## gabefletch/StarFoxAssault

A decompilation scaffold made from [encounter/dtk-template](https://github.com/encounter/dtk-template),
CC0-1.0 for the whole repository. It was created on 2026-10-07, with its last push on
2026-10-11, no stars, no releases and no source files.

| Item | State |
|---|---|
| `config/GF7E01/config.yml` | `object: sys/main.dol`, `hash:` set to the DOL SHA-1 above, `mw_comment_version: 11`. No `modules:` list: the commit history shows the template's module was removed, so **the 23 modules are not analyzed at all**. |
| `config/GF7E01/symbols.txt` | 18,590 entries from dtk, committed on 2026-10-07. 5,375 functions, 306 of them named (the runtime, MetroTRK and similar). The same file xSNAIRZx ships. |
| `config/GF7E01/splits.txt` | The section table plus **one** translation unit, `Runtime.PPCEABI.H/__init_cpp_exceptions.cpp` (`.text`, `.ctors$10`, `.dtors$10`, `.dtors$15`, `.sdata`), added by a Copilot-authored pull request on 2026-10-11 from symbol boundaries. Effectively no splits. |
| `configure.py` | The template with the version list set to `GF7E01`: dtk `v1.8.3`, objdiff `v3.6.1`. No source files are registered and there is no `src/`. |
| CI | `.github/workflows/build.yml` runs in a private container image, so nobody else can reproduce it. |

Two cautions. The history includes commits that add and then remove a game image
(`.rvz`, tracked with Git LFS). The repository is only 396 KB, so there is no disc image in
its git objects, but read it at the head through the API as we do here and do not clone it or
fetch its LFS objects. And there is nothing to learn from it beyond the symbol file: no
names, no splits, no module data.

## Finished gcn-recomp projects

[gcn-recomp](https://github.com/ethannicholas/gcn-recomp) calls itself the common layer
under per-game projects, and each project "supplies its own game's layout and steering files
and nothing else". Both finished projects follow that, and both are MIT, copyright Ethan
Nicholas.

| | prime-recomp | bluestorm-recomp | Ours today |
|---|---|---|---|
| Game | Metroid Prime, 60 fps | Wave Race: Blue Storm, 30 fps (60 optional) | Star Fox Assault |
| `CMakeLists.txt` | One `gcn_add_game()` with `SHADER_KEYS`, `SOURCES`, Android and visionOS ids | The same, `FPS 30`, five `SOURCES` | `gcn_add_game()` with the hash still empty; PR #16 fills it in and adds `RELS` and `REL_SYMBOLS` |
| `analysis/` | `config.yml` (3 lines), `symbols.txt` (2.7 MB, every function named from a decompilation), `splits.txt` (142 KB) | The same, 573 KB of symbols | Empty until PR #16 |
| `recomp/hle.txt` | All five required functions | Four (no `OSResetSystem`) | Empty |
| `recomp/special_calls.txt` | `OSSaveContext` | The same | Empty |
| `recomp/idle.txt` | Two entries: the scheduler's `SelectThread` spin, and the game's own frame wait | Absent | Empty |
| `recomp/names.txt` | Empty ("the symbol file names every function") | Six hand-named SDK functions, including `OSSaveContext`, `OSPanic` and the scheduler's thread chooser, because the symbols lack SDK names | Empty |
| `recomp/patches.txt` | About 30 patch lines, each commented, for the VR and gamepad builds | 17 patch lines, each commented: heap tracing and 60 fps pacing | Empty |
| `recomp/steps*.txt` | None | `steps.txt` (9 KB), `steps_manual.txt` (14 KB), `steps_skip.txt` (5 KB) | None |
| Game code | `src/gx_prime.cpp`, `heap_check.cpp`, `vr_prime.cpp` | `runtime/pace.cpp`, `first_person.cpp`, `motion.cpp`, `start_rig.cpp`, `vr_waverace.cpp` | None |
| `docs/dev/` | `audio`, `diagnostics`, `graphics`, `status`, `vr` | Adds `performance`, `telemetry` | None before this file |
| Other | `.recomp.json` (a [recomp.fyi](https://recomp.fyi) listing), `routes/`, recorded inputs | `tools/rate_sites.py`, `compare_runs.py` | |

What the table shows that matters to us:

- **A patch is a line with an address, an optional call into the game's own code, and a C
  statement that replaces one instruction.** The call goes to a function declared inline,
  because the generated code only sees `recomp.h`. Every patch has a comment saying why, and
  Prime's name the source file and section in `src/` that explains it. That is the shape for
  #11 and #12.
- **Hooks are installed from a static initializer** in a file listed under `SOURCES`.
  Prime's `gx_prime.cpp` is a few lines that set one flag in the GX front end under a long
  comment saying what the game does and why the flag is safe.
- **Tables stay small.** Prime's `idle.txt` has two entries. The second, the game's own
  frame wait, is worth looking for in Star Fox Assault for #5, beyond the SDK scheduler.
- **Blue Storm is the frame-rate template.** Its 60 fps work has three parts:
  1. Patches at the game's frame-end wait and clock (`pace.cpp` holds the logic).
  2. A list of per-frame steps in `steps.txt`, found by running a debug build with
     `GCN_STORE_HIST` and filtering with `tools/rate_sites.py`, plus hand-read exceptions
     in `steps_manual.txt` and sites left alone in `steps_skip.txt`. The method is in
     gcn-recomp's `docs/diagnostics.md` ("Finding a game's per-frame steps") and the
     story in `docs/dev/performance.md`, "60 frames per second".
  3. Gating any simulation that cannot be scaled (water, spray, physics passes) to every
     other frame.
  It also shows that frame-rate work is a long tail: `performance.md` has thirteen dated
  sections written over two days.
- **Neither project passes `RELS`.** Prime has one module (an emulator it cannot run) and
  says the recompiler translates only its DOL. Star Fox Assault has 2,315 functions in
  modules against 5,375 in the DOL, so for module handling gcn-recomp's `docs/rels.md` and
  xSNAIRZx's changes are the references, not these projects.
- **Neither project, nor gcn-recomp, covers system fonts or flat-screen widescreen.** Their
  docs do not mention either, so xSNAIRZx's fork is the only reference for both.
- **Upstream conventions** for anything we send back: `CLAUDE.md` forbids game names, disc
  IDs and guest addresses in code, enforced by `tools/check_generic.py`; environment
  variables and CMake options use `GCN_`; findings go in `docs/`. Outside pull requests are
  accepted in practice: there are three open from another contributor.

## Upstream candidates

Our rules say anything not specific to this game goes to gcn-recomp, not into our tree. These
changes in xSNAIRZx's fork are generic and would help us, so they are worth an issue or pull
request upstream, written for the mechanism and not for this game:

1. Repeated REL module IDs at build time and run time (`rel.py`, `rels.cpp`). First choice,
   if our modules turn out to share IDs, since it stops the build. The new `rels_bind()` is
   only needed if `OSLink` stays unnamed; PR #16 names `OSLink` and `OSUnlink` in
   `names.txt`, which is lighter.
2. IPL system fonts through EXI.
3. The `SU_SSIZE`/`SU_TSIZE` texture-coordinate scale.
4. Fractional internal scale, the aspect-aware present, FXAA and sharpening.
5. The MinGW/clang compile fixes and the SDL cache check, for #8.
6. The thread-watch build option and the richer MMU fatal.
7. Possibly the disc overlay, if we want mods in M5.

Until upstream takes them, we have no way to use them without a patched submodule, which our
rules do not allow. Credit xSNAIRZx for any of it we send. Their code is MIT, so this is
permitted with the notice kept, and contact before copying is courteous.

Suggested next steps, in order: read the module ID from each of the 23 REL headers on our
disc (read-only) to learn whether any repeat; if so, open an upstream issue for the
repeated-ID case, linking this document and crediting xSNAIRZx, before #8 meets it as a
build error; and keep xSNAIRZx's `rel_rebinding_test.cpp` in mind as the test to adapt.

## What we can reuse

Credit wording is for the README's Credits section unless noted. "Fact" means addresses,
names or hashes, which need no license but are polite to credit.

| What | From | License | Credit wording | Helps |
|---|---|---|---|---|
| `main.dol` symbols and splits | gabefletch/StarFoxAssault | CC0-1.0 | "Symbols for `main.dol` from [gabefletch/StarFoxAssault](https://github.com/gabefletch/StarFoxAssault) (CC0), generated with decomp-toolkit." | #3 |
| Per-module symbols and splits, and the dtk module `links` | xSNAIRZx/star-fox-assault-recomp | MIT (probably; see Licenses) | "Per-module symbols from [xSNAIRZx/star-fox-assault-recomp](https://github.com/xSNAIRZx/star-fox-assault-recomp) (MIT). Copyright (c) 2026 Star Fox Assault Recomp contributors." Keep their `LICENSE` beside the files. | #3 |
| `RELS` and `REL_SYMBOLS` wiring in CMake | xSNAIRZx | MIT | In a code comment: "Module list after xSNAIRZx/star-fox-assault-recomp." | #3, #8 |
| `OSSaveContext` address as a cross-check; the five-function `hle.txt` to complete theirs | xSNAIRZx; gcn-recomp README | Fact | None needed. | #4 |
| DOL SHA-1 as a cross-check | gabefletch, xSNAIRZx | Fact | None needed. | #2 |
| An idle-loop pattern: SDK scheduler plus the game's frame wait | prime-recomp `recomp/idle.txt` | MIT | None for the idea. | #5 |
| Duplicate-module-ID support, stale-mapping checks, module-list fallback | xSNAIRZx `runtime/` | MIT (code); MIT (gcn-recomp base) | If sent upstream: "Based on a patch from xSNAIRZx/star-fox-assault-recomp (MIT)", in the commit message and the file header, with their notice kept. | #8, #9 |
| System-font serving through EXI, and the replacement fonts | xSNAIRZx (fonts: Dolphin) | MIT code; Apache-2.0 fonts | Code as above. Fonts: "GameCube system font replacements from Dolphin, based on Droid Sans, copyright Google, Apache-2.0", with `font-licenses.txt` kept. Check Dolphin's copies directly rather than via xSNAIRZx. | #9 |
| Text fix (`SU_SSIZE`/`SU_TSIZE` coordinate scale) | xSNAIRZx | MIT | As above. | #9 |
| Windows/clang compile fixes and SDL cache validation | xSNAIRZx | MIT | As above. | #8, #9 |
| `GCN_THREAD_WATCH`, richer MMU fatal, `GCN_RAM_DUMP` | xSNAIRZx | MIT | As above. | #9 |
| Aspect and viewport math, widescreen projection scale, z-freeze plane, fractional scale, FXAA and sharpening | xSNAIRZx `gx/` | MIT | "Widescreen and internal-resolution approach after xSNAIRZx/star-fox-assault-recomp (MIT)." | #11, #10 |
| Culling for widescreen | prime-recomp `patches.txt` (frustum hook pattern) | MIT | None for the idea. | #11 |
| 30/60 fps pattern: pacing patches, `steps.txt`, `rate_sites.py`, `compare_runs.py` | bluestorm-recomp | MIT, Copyright Ethan Nicholas | "Frame-rate method and tooling from [bluestorm-recomp](https://github.com/ethannicholas/bluestorm-recomp) by Ethan Nicholas (MIT)." Keep the notice on copied scripts. | #12 |
| Disc overlay (`GCN_MODS_DIR`, same-size files) | xSNAIRZx | MIT | As above. | M5 mod loader (no issue yet) |
| Project layout: `docs/dev/`, `.recomp.json`, `SOURCES` hooks | prime-recomp, bluestorm-recomp | MIT | None for layout. | #10, #11, #12 |
| Launcher, build scripts, artwork, prebuilt `.exe` files | xSNAIRZx | MIT code; no grant for artwork | Do not use. | none |
| Their two open bugs (first boss, current-thread crash) | xSNAIRZx `VALIDATION.md` | Observation | If we hit either, link their notes in the issue. | #9 |

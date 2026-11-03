# The enhancement menu: design

Design for issue #10, the in-game menu every milestone 4 and 5 feature plugs into. Nothing
here is built yet, and the game doesn't boot, so this is a plan to agree on before any code.

It is based on gcn-recomp at `9e93f1b` (the commit this repository pins) and on Ship of
Harkinian's menu, which was read for its ideas only: none of its code is used or copied.

## Summary

- **gcn-recomp gets the generic overlay**: Dear ImGui and its SDL2 and OpenGL 3 backends,
  input capture, pausing, the settings registry and its file, the menu shell with the
  registration API, and the pages for what the runtime itself does (window, resolution,
  audio mix, controller bindings, statistics). The gcn-recomp ports share this frontend,
  so they would all get the menu.
- **This project gets only Star Fox Assault's features**: each is a file in `src/` that
  registers its own settings and menu entries through the API. There is no menu code here.
- If upstream wants less than that, the fallback is a handful of frontend hooks upstream,
  with the ImGui menu living in this project's `src/` instead (see [Fallback](#fallback-if-upstream-declines)).
- Settings live in `settings.txt` beside `saves/` and `controls.txt`, one `key = value` per
  line, like gcn-recomp's other text files.
- **F1** or **Esc** opens the menu. On a controller it's **Back** (View, Select, Share or −),
  which no GameCube control is mapped to, or holding **L + R + Start** on a controller that
  has no Back button. The game pauses while the menu is open.

## What gcn-recomp's frontend looks like today

Everything below is in `runtime/main.cpp` unless it says otherwise.

```
main loop, on the main thread (which is also the render thread):
  pump_events()        SDL_PollEvent: input_map_event, controller hot-plug, quit, Esc quits
  update_pad()         merge_desktop_pad (keyboard and first gamepad, polled with
                       SDL_GetKeyboardState and SDL_GameControllerGet*), input_map_apply
                       (controls.txt), input_script_apply, then pad_set_state(0, ...)
  take_batch(4)        the guest thread's next frame; on a timeout, loop again (nothing drawn)
  render_execute(b)    draws the frame and blits it to framebuffer 0, letterboxed to 4:3
  glBindFramebuffer(GL_FRAMEBUFFER, 0)
  record::frame()      --record captures the window here
  SDL_GL_SwapWindow()
```

What matters for an overlay:

- **Events.** `pump_events()` is the only event loop. Esc quits the game.
- **Pad input is polled, not event-driven.** `merge_desktop_pad()` reads the keyboard and the
  gamepad state directly, so swallowing SDL events isn't enough to keep input from the
  game. The gate has to be on the `PadState` that `update_pad()` builds. Only port 1 is fed
  today.
- **Drawing.** The finished frame is in framebuffer 0 after `render_execute()` returns true.
  An overlay draws there, before `SDL_GL_SwapWindow()`. Drawing after `record::frame()`
  keeps it out of `--record` movies.
- **No frame, no present.** When `take_batch()` times out the loop draws nothing, so an
  overlay over a paused or stalled game needs the last frame shown again. The renderer has
  no entry point for that yet. `runtime/gx/render_gl.cpp` has a comment about keeping the
  last presented rect for refreshes, but no function that does it.
- **No pause.** Not taking batches doesn't stop the guest: `submit_batch()` in
  `runtime/gx/xf.cpp` waits two seconds, then drops the oldest frame and carries on.
- **A safe point on the guest thread.** `gx::set_guest_frame_hook()` (`runtime/gx/render.h`)
  runs at the end of every submitted frame, when the guest's state is consistent. That is
  where anything that writes guest memory (cheats, the save editor) belongs. It holds one
  hook, which is the game project's.
- **Config files.** There's no settings system. Options are command-line flags (`--scale`,
  `--no-music` and others), `GCN_*` environment variables, `controls.txt` (bindings, in the
  working directory) and `vr.txt` (headset tuning, `runtime/vr_config.*`). Machine state
  goes in `saves/`: `memcard_a.raw`, `shaders.bin`, `inputs/`.
- **Replays must stay exact.** `runtime/input_log.h` records every pad poll and every pose
  the guest reads, so a run replays exactly on the virtual clock. Anything the guest reads
  that the user can change has to be recorded too, or a replay drifts.
- **Game hooks.** A game passes `SOURCES` to `gcn_add_game()`, and they install hooks from
  static initializers (`vr::GameHooks`, the eye hook, the guest frame hook). Registries use
  function-local statics because those initializers may run first. The same sources are
  compiled into the headless benchmark, which has no SDL or GL.
- **Nothing game-specific upstream.** gcn-recomp's `CLAUDE.md` forbids game names, disc IDs
  and per-game constants outside its README, and `tools/check_generic.py` enforces it. The
  upstream half of this design has to pass that check.

## What Ship of Harkinian does

These are the ideas worth borrowing from Ship of Harkinian and libultraship. Its code isn't
copied; the ideas are general.

- **Two layers.** libultraship is the generic layer: window, ImGui setup, console variables
  and the JSON config. Ship of Harkinian only adds menus. That's the same split as
  gcn-recomp and this project.
- **Settings are named variables with defaults** (console variables, or CVars), grouped by
  name prefix (`gEnhancements.*`, `gCheats.*`, `gSettings.*`) and stored in one JSON file.
  A change marks the file for saving, and it's written once at the next frame. Version
  updaters migrate old configs.
- **Widgets are data, not drawing code.** Each entry is a record (label, variable, type,
  options, tooltip, callbacks, the conditions that disable or hide it) in a tree of top menu,
  sidebar page, section and column. Search, tooltips and "disabled because..." messages
  all fall out of that for free.
- **Opening it.** F1 toggles the menu bar, Esc the full menu, and the gamepad's Back button
  does it too when "controller navigation" is on. Gamepad navigation is opt-in.
- **Settings for the menu itself.** Theme, background opacity, scale, controller
  navigation, and remembering the last page viewed in each menu.
- **The shape of the tree.** Settings (General, Audio, Graphics, Controls), Enhancements
  (Quality of Life, Skips & Speed-ups, Graphics, Items, Fixes, Difficulty, ...),
  Randomizer, Network and Dev Tools (Stats, Console, Save Editor, Collision Viewer, Actor
  Viewer, Value Viewer, ...).

What we do differently: here the game pauses while the menu is open, by default, so
gamepad navigation can always be on. The config is a text file in gcn-recomp's style
rather than JSON, and only changed values are written. And settings the guest reads are
latched once per frame and logged, so replays stay exact.

## Architecture

### Upstream in gcn-recomp

All of it would be behind a CMake option, `GCN_OVERLAY`, on by default for the desktop
executable. It is split so that a game's `SOURCES`, which also go into the benchmark, can
register settings without SDL or GL:

| Piece | Where | Links into |
|---|---|---|
| Settings registry, values, `settings.txt` | `runtime/settings.*` (new) | every target |
| Menu registry (pages, windows, buttons) | `runtime/overlay_registry.*` (new) | every target |
| ImGui core library | fetched like the OpenXR headers, pinned to a tagged release | every target (the benchmark never calls it) |
| ImGui backends, input routing, drawing | `runtime/overlay.cpp` (new) | the desktop executable |
| Generic pages: Game, Graphics, Controls, Audio, Developer, Help | `runtime/overlay_pages.cpp` (new) | the desktop executable |
| Pause gate, re-present, clock rebase | `gx/fifo.cpp`, `gx/render_gl.*`, `cpu.cpp`, `audio_sdl.cpp` | as today |

Where each part hooks into `main.cpp`:

1. **Setup.** After `gl_load_with()` succeeds and before `gx::render_init()`: create the
   ImGui context and initialize the SDL2 and OpenGL 3 backends on the existing window and
   context (GLSL `#version 330 core`, or `300 es` under `GCN_GL_ES`). Load `settings.txt`
   before this, since it decides the window mode and internal scale. ImGui's own layout
   file goes to `saves/imgui.ini`.
2. **Events.** At the top of `pump_events()`'s loop, `overlay::handle_event(e)` passes the
   event to ImGui and returns true if the overlay consumed it: a toggle key, or keyboard and
   mouse input while the menu is open. Consumed events skip the rest of the loop, except
   that `input_map_event()` always sees device hot-plugs. With the overlay built, Esc opens
   the menu instead of quitting. Quit is in the menu, and so is a setting that restores the
   old behavior (`menu.escape_quits`).
3. **Pad.** In `update_pad()`, between `merge_desktop_pad()` and `input_script_apply()`:
   `overlay::filter_pad(p)` watches for the hold combo, and while the menu has input it
   replaces `p` with a connected pad at rest. Rumble stops. Scripted input still applies,
   so tests can drive the game with the menu open.
4. **Draw.** When a frame is presented, after `record::frame()` and before
   `SDL_GL_SwapWindow()`: `overlay::draw(dw, dh)`. That draws the menu bar and pages,
   floating tool windows and HUD overlays (an FPS counter, a timer). It has to leave GL
   state as it found it: the renderer shadows some state (cull mode, logic op), and ImGui's
   OpenGL 3 backend saves and restores what it touches, but that needs checking against the
   shadow.
5. **No new frame.** When `take_batch()` times out and the overlay is visible, call
   `gx::render_present_again()` (new: blits the last presented image at the current window
   size), draw the overlay, and swap. That keeps the menu responsive while the game is
   paused or loading.

New runtime pieces it needs:

- **A pause gate** on the guest thread, at the frame boundary beside the guest frame hook
  call in `gx/fifo.cpp`. While paused the guest blocks there, with its state consistent.
  Because guest time is virtual, blocking is invisible to the game and to a replay.
- **A clock rebase on resume.** Otherwise `clock_pace()` finds real time far ahead and
  catches up, either by running flat out or with a logged catch-up jump.
- **Audio pause** (`SDL_PauseAudioDevice`) while the guest is paused, instead of a
  starved ring and an underrun count.
- **`render_present_again()`**, described above.
- **Setting records in the input log.** See [Threads and replays](#threads-and-replays).

### In this project

Only Star Fox Assault's features, each in its own file under `src/`, passed as `SOURCES`.
A feature registers its settings and menu entries and consults them where the game is
patched (`recomp/patches.txt` can call into `SOURCES`), or does its work in a guest-frame
callback. For example:

- `src/qol/skip_logos.cpp`, `src/qol/cutscenes.cpp`
- `src/cheats/player.cpp` (health, ammo, bombs), `src/cheats/unlock.cpp`
- `src/cosmetics/colors.cpp`
- `src/widescreen.cpp`: the game's projection, HUD placement and culling (#11), on top of
  the runtime's aspect setting
- `src/framerate.cpp`: which modes allow which rates (#12), on top of `recomp/steps.txt`
- `src/save_editor.cpp`, `src/speedrun.cpp`, `src/dev/object_viewer.cpp`
- `src/menu_layout.cpp`: the order of this game's pages (see below)

The game's own guest frame hook stays the game's: the overlay adds its pause gate and its
callback list next to the hook call, not through it.

### Fallback if upstream declines

If gcn-recomp would rather not carry ImGui or a settings system, the smallest generic change
that still unblocks us is a frontend hook struct, installed from `SOURCES` like
`vr::GameHooks`:

- `on_init(SDL_Window*, SDL_GLContext)`
- `on_event(const SDL_Event&) -> bool` (consumed)
- `filter_pad(PadState&)`
- `after_present(int w, int h)`, plus `render_present_again()` and a "keep presenting"
  answer for when no frame arrives
- the pause gate and clock rebase, which are needed either way

The menu, the registry and `settings.txt` would then live in this project's `src/` and
`CMakeLists.txt`, with the same API, so moving them upstream later wouldn't change any
feature file. The draft issue at the end asks the maintainer which they prefer.

## Registration API sketch

This is a sketch, not the final API. It's C++20, in gcn-recomp's style: snake_case,
namespaces, registries as function-local statics, and registration from static initializers.

### Settings (core: no SDL, no GL)

```cpp
// runtime/settings.h
namespace settings {

enum Flags : uint32_t {
    Restart      = 1 << 0,  // read once at startup; the menu shows "after a restart"
    AffectsGuest = 1 << 1,  // the guest reads it: latched each guest frame, logged for replays
    Cheat        = 1 << 2,  // marks the run as modified (speedrun timer, save warning)
    Developer    = 1 << 3,  // shown only with Developer > Show developer options
};

// One user-changeable value. Created from a static initializer and never destroyed, so a
// reference to it stays valid for the whole run.
template <typename T> class Setting {
public:
    T get() const;            // any thread; lock-free for bool, int, float and colors
    operator T() const { return get(); }
    void set(T value);        // main thread only: the menu, a hotkey, --set
    void reset();             // back to the default

    // Options, chained where the setting is defined.
    Setting& help(const char* text);                     // tooltip, and searched
    Setting& flags(uint32_t f);
    Setting& on_change(void (*fn)(T value));             // main thread, after a change
    Setting& enabled_if(bool (*fn)(), const char* why);  // otherwise greyed out, `why` shown
    Setting& visible_if(bool (*fn)());
    Setting& while_on(void (*fn)(uint32_t frame));       // bool only: guest thread, each frame
    Setting& hotkey(const char* default_key);            // rebindable in Game > Hotkeys
};

// `key` is the name in settings.txt and never changes once released ("cheats.infinite_health").
// `path` is where it appears: "Menu/Page/Section". `label` is what the menu says.
Setting<bool>&     toggle(const char* key, const char* path, const char* label, bool def = false);
Setting<int>&      slider(const char* key, const char* path, const char* label, int def, int lo, int hi);
Setting<float>&    slider(const char* key, const char* path, const char* label, float def, float lo, float hi);
Setting<int>&      choice(const char* key, const char* path, const char* label,
                          std::initializer_list<const char*> names, int def = 0);
Setting<uint32_t>& color(const char* key, const char* path, const char* label, uint32_t rgba);

// Guest-thread work at the end of each guest frame, where its state is consistent.
void on_guest_frame(void (*fn)(uint32_t frame));
// Once, on the guest thread, at the next frame boundary: a menu button that writes guest memory.
void run_on_guest(std::function<void()> fn);
// A key renamed between releases, so an old settings.txt still applies.
void renamed(const char* old_key, const char* new_key);
}
```

### Menu entries beyond settings (core registry, drawn by the frontend)

```cpp
// runtime/overlay_registry.h
namespace menu {
void button(const char* path, const char* label, void (*fn)(), const char* confirm = nullptr);
// A page the feature draws itself with ImGui, for what a list of settings can't express.
void page(const char* path, void (*draw)());
// A floating tool window the menu opens and closes; whether it's open is remembered.
void window(const char* path, const char* title, void (*draw)());
// Drawn over the game while the menu is closed: an FPS counter, a timer, an input display.
void hud(const char* key, const char* label, void (*draw)());
// Order of a menu's pages, or a page's sections. Anything unlisted follows, alphabetically.
void layout(const char* path, std::initializer_list<const char*> children);
bool is_open();
bool has_input();
}
```

Static initializers in different files run in no fixed order, so the menu never depends on
it. Top menus come in a fixed order (Game, Graphics, Controls, Audio, Enhancements, Cheats,
Developer, Help). A game can add its own before Developer. Pages and sections follow
`menu::layout()`, then alphabetical order. Items in a section keep the order they were
registered in within one file. A duplicate key is a fatal error at startup, and so is any
registration after the menu starts. Because the registry is frozen by then, the menu and
the guest can walk it without locks.

### What a feature looks like

A toggle that a patch consults:

```cpp
// src/qol/skip_logos.cpp: start at the title screen.
#include "settings.h"

static auto& skip_logos =
    settings::toggle("enhancements.skip_logos", "Enhancements/Quality of life/Boot",
                     "Skip the logos")
        .help("Go straight to the title screen.")
        .flags(settings::AffectsGuest);

// Called by the patch where the game starts its logo sequence (recomp/patches.txt).
extern "C" bool sfa_skip_logos() { return skip_logos; }
```

A cheat that does its work each frame:

```cpp
// src/cheats/player.cpp
static void refill_health(uint32_t) { /* write the player's health in guest RAM */ }

static auto& infinite_health =
    settings::toggle("cheats.infinite_health", "Cheats/Player", "Infinite health")
        .flags(settings::AffectsGuest | settings::Cheat)
        .while_on(refill_health);
```

Neither file touches the menu code, and adding a feature is adding a file to `SOURCES`.
Because every entry is data (key, path, label, help), the menu gets search, tooltips,
"reset to default", "changed from default" markers and `--list-settings` without any
feature doing anything.

### Threads and replays

- The registry is filled during static initialization and frozen when `main()` starts the
  overlay.
- The main thread is the only writer: the menu, hotkeys, `settings.txt` and `--set`.
  `on_change` callbacks run there.
- bool, int, float, choice and color values are atomics, so the guest thread, the audio
  callback and the renderer read them without locks. Strings take a lock.
- `AffectsGuest` settings are latched at the guest's frame boundary, so the guest sees one
  value for a whole frame. Each change is written to the input log as a `setting <key>
  <value>` line against the frame count, the same way `pose` lines work, and a replay
  applies them from the log. A replay then reproduces a run that toggled a cheat halfway
  through.
- Anything that writes guest memory runs on the guest thread, through `while_on`,
  `on_guest_frame` or `run_on_guest`, never from the menu directly.

## Config

**File:** `settings.txt` in the working directory, beside `saves/` and `controls.txt`.
`--settings=<path>` or `GCN_SETTINGS` points elsewhere. Settings are preferences, like
`controls.txt`, so they sit next to it rather than in `saves/`, which is state the machine
builds up.

**Format:** one `key = value` per line, `#` to the end of the line a comment, like
`controls.txt`:

```
# sfassault-recomp settings, written by the in-game menu (F1). Safe to edit while the game
# isn't running. Only settings changed from their defaults are listed; run with
# --list-settings for every one, its default and what it does.
version = 1

graphics.internal_scale = 3
graphics.window_mode = borderless
graphics.vsync = on
audio.music_volume = 0.6
enhancements.skip_logos = on
cosmetics.arwing_primary = #3060C0
```

- Values: `on`/`off` (also `true`/`false`, `1`/`0`), numbers, choices by name (so reordering
  a choice never changes what a file means), `#RRGGBB` or `#RRGGBBAA`, and strings in double
  quotes.
- Keys are lowercase and dotted. The first part names the menu by convention
  (`graphics.`, `audio.`, `controls.`, `enhancements.`, `cheats.`, `cosmetics.`, `dev.`,
  `menu.`). Keys are permanent once released; labels can change freely.
- **Only changed values are written.** A default improved later reaches everyone who didn't
  change it, and the file stays short enough to read.
- **Unknown keys are kept** and written back, so a setting from a newer build, or from a
  feature compiled out, survives a run of an older one.
- **Saving** happens when the menu closes, at exit, and two seconds after the last change
  while it's open (so a crash loses little), through a temporary file and
  `plat_replace_file()`. Edits made by hand while the game runs are overwritten, and the
  header says so.
- **Migrations:** `version` is bumped when a key's meaning changes. `settings::renamed()`
  covers renames.
- **Command line:** `--set key=value` (repeatable) overrides a setting for one run without
  saving it. The existing flags keep working as shorthands: `--scale=3` sets
  `graphics.internal_scale` for that run. `--list-settings` prints every setting, the way
  `--list-controls` prints every control.
- **Bindings stay in `controls.txt`.** The Controls page edits that file and changes only
  the lines it rewrites, so comments survive. `vr.txt` stays as it is.

## Opening the menu

| Input | Opens or closes the menu | Notes |
|---|---|---|
| F1 | toggles | as issue #10 says |
| Esc | opens; closes, or backs out of a page | today Esc quits; `menu.escape_quits` brings that back |
| Controller Back (View, Select, Share, −) | toggles | gcn-recomp's default layout maps no GameCube control to it, so the game never sees it. Guide/Home is left alone because Steam and the OS often take it |
| Hold L + R + Start for 1 second | opens | for controllers with no Back button (a GameCube controller on an adapter, a `controls.txt` device). The buttons and the hold time are settings, and a game can change the default |
| Window close | quits | as today |

- **While the menu is open** the game is paused (`menu.pause`, on by default). The keyboard,
  mouse and gamepad drive the menu: d-pad or stick to move, A to choose, B to go back,
  Back or Start to close. The game sees a pad at rest, which is why gamepad navigation can
  always be on. `menu.game_keeps_input` (off by default) lets the game keep the pad while
  the menu is open, for tuning graphics while playing; it turns off pausing and gamepad
  navigation.
- **The mouse cursor** shows while the menu is open and hides after two seconds without
  movement while it's closed. Once mouse aiming exists (milestone 4, controls), opening the
  menu releases relative mouse mode and closing it takes it back.
- **First runs** show a hint for a few seconds: "F1 or Back: menu". It stops once the menu
  has been opened (`menu.show_hint`).
- **In versus** any port's controller can open the menu, and the controller that opened it
  drives it.

## The menu tree

The top bar is Game, Graphics, Controls, Audio, Enhancements, Cheats, Developer and Help.
That's issue #10's six menus plus Game and Help, which hold what the runtime itself owns:
quit, reset, the menu's own settings and the about box.

The **Layer** column says who builds an entry: **up** for gcn-recomp (generic) and
**here** for this project (Star Fox Assault). The **Roadmap** column gives the milestone
(and issue) from `docs/status.md`. Items marked "candidate" aren't on the roadmap yet.
Everything that changes the original game is off by default. Graphics and audio defaults
match what the runtime does today (2x internal scale, 4:3, VSync off).

### Game

| Page / section | Items | Layer | Roadmap |
|---|---|---|---|
| (menu items) | Resume; Reset the console (a fresh process, as `plat_relaunch` does now); Quit | up | M4 #10 |
| Menu | Pause while open; let the game keep input; Esc quits; controller combo and hold time; menu scale; opacity; first-run hint | up | M4 #10 |
| Hotkeys | A rebindable key for every setting with a `hotkey()` | up | M4 #10 |
| Presets | Original (everything off), Recommended: one click sets a group of settings | up (mechanism), here (the groups) | candidate |

### Graphics

| Page / section | Items | Layer | Roadmap |
|---|---|---|---|
| Display | Window mode (windowed, borderless, fullscreen); display; VSync; frame cap | up | M4 |
| Rendering | Internal resolution (`render_set_internal_scale`, 1x to 8x); anti-aliasing; shader cache status | up | M4 |
| Aspect ratio | Output aspect (4:3, 16:9, 16:10, 21:9, window), replacing the fixed 4:3 letterbox | up | M4 #11 |
| Aspect ratio | Widescreen projection; HUD placement (centered 4:3, stretched, at the edges); culling margin | here | M4 #11 |
| Frame rate | Target rate (native, 60, 120, 144, unlimited) through `recomp/steps.txt` and `step_set_scale` | up (mechanism), here (which modes allow what) | M4 #12 |
| Upscaling | FSR, then DLSS where motion vectors allow; sharpness | up | M4 #13 |
| Statistics overlay | FPS, frame time (shortcut to the same toggles in Developer) | up | M5 |

### Controls

| Page / section | Items | Layer | Roadmap |
|---|---|---|---|
| Ports | Which device is on ports 1 to 4 (keyboard, a gamepad, none). The runtime only feeds port 1 today | up | M4, M3 (versus) |
| Bindings | Per port: each GameCube control to a key, button or axis, with "press something" capture, saved to `controls.txt` | up | M4 |
| Sticks and triggers | Dead zones, sensitivity, ranges (`stick_x_min` and the rest), trigger thresholds | up | M4 |
| Rumble | On or off, strength | up | M4 |
| Mouse and gyro | Mouse to stick or C-stick, sensitivity, smoothing; gyro (SDL's controller sensors) the same | up (mechanism) | M4 |
| Aiming | Mouse and gyro aiming per mode (Arwing, Landmaster, on foot); invert Y per vehicle | here | M4, M5 (gameplay) |
| Input display | Show the pad on screen | up | candidate |

### Audio

| Page / section | Items | Layer | Roadmap |
|---|---|---|---|
| Volume | Master; music (the streamed DTK track, which the runtime mixes separately); effects (the DSP mix) | up | M4 |
| Volume | Voice, and music if the game plays it through the DSP rather than a stream: needs the game's own mixer | here | M4 |
| Output | Device; mute when the window loses focus | up | M4 |

### Enhancements

| Page / section | Items | Layer | Roadmap |
|---|---|---|---|
| Quality of life | Skip the Nintendo and Namco logos; skip the intro; make every cutscene skippable; faster menus and transitions | here | M5 |
| Gameplay | Difficulty tweaks (damage taken, and others once the game is understood) | here | M5 |
| Cosmetics | Arwing, Landmaster and character color editors; reset | here | M5 |
| Mods | Texture packs, including Dolphin-format packs: list, enable, order | up | M5 #14 |
| Mods | Asset replacements (audio and other disc files), from a folder that overlays the disc | up | M5 |
| Speedrun | In-game timer, mission splits, a "modified run" marker when cheats are on, LiveSplit connection (candidate) | here | M5 |

### Cheats

| Page / section | Items | Layer | Roadmap |
|---|---|---|---|
| Player | Infinite health; infinite ammo; infinite bombs | here | M5 |
| Unlocks | Unlock everything (missions, medals, versus content, bonus extras), with a warning before it touches a save | here | M5 |

Every entry here carries the `Cheat` flag, which turns on the speedrun page's "modified run"
marker.

### Developer

| Page / section | Items | Layer | Roadmap |
|---|---|---|---|
| Statistics | FPS and frame-time graph; audio buffer (`audio_ring_stats`); shader builds; batch queue | up | M5 |
| Tools | Free camera | here (maybe up, through the eye path `--eye` uses) | M5 |
| Tools | Object viewer | here | M5 |
| Tools | Save editor (a page of its own) | here | M5 |
| Tools | Mission select | here | candidate |
| Diagnostics | Frame dumps; start or stop an input log; log categories (`--log=`); ImGui's metrics window | up | candidate |
| (toggle) | Show developer options (entries flagged `Developer`) | up | M4 #10 |

### Help

| Page / section | Items | Layer | Roadmap |
|---|---|---|---|
| About | Version, this project's commit, gcn-recomp's commit, GPU and GL version | up | M4 #10 |
| Files | Open the folder with `settings.txt`, `controls.txt` and `saves/` | up | M4 #10 |

## Doing it in stages

1. **This design** and the upstream conversation (the draft below). Nothing else can start
   until gcn-recomp's maintainer answers.
2. **The overlay core**, upstream or in the fallback: ImGui setup, F1/Esc/Back, input
   capture, pausing and re-presenting, the registry, `settings.txt`, the Game and Help menus,
   and Developer > Statistics. Issue #10 is done when the menu opens over the game and a
   sample toggle (the FPS counter) survives a restart.
3. **The runtime's generic pages**: Graphics (window mode, VSync, internal resolution,
   aspect), Audio (master, music, effects), Controls (bindings, rumble).
4. **Star Fox Assault's features**, each its own roadmap issue, once the game boots
   (milestone 1) and a mission plays (milestone 2).

## Open questions

1. **Upstream or not.** Will gcn-recomp take the full overlay, only the hooks, or neither?
   The answer decides where `settings.*` and the menu live. The feature files don't change
   either way.
2. **Pausing.** Is the end of the guest's frame always a safe place to block? DVD reads and
   audio DMA might be in flight there. And how should a catch-up jump the clock had already
   scheduled be handled across a pause? Needs testing once the game boots.
3. **The controller combo.** Does holding L + R + Start do anything in Star Fox Assault, in
   flight, in the Landmaster, on foot or in versus? Start probably opens the game's own
   pause screen underneath, which is harmless but should be checked in milestone 1.
4. **Where files live.** The working directory keeps installs portable and matches
   `controls.txt` and `saves/` today. An installed build (Program Files, a Linux package, a
   macOS app bundle) would want a per-user folder (`SDL_GetPrefPath`). That's an upstream
   decision covering saves too; keeping every path relative to one base folder makes it a
   single change.
5. **Bindings.** Should the Controls page keep editing `controls.txt`, or should bindings
   move into `settings.txt`? Editing in place keeps one source of truth and the file's
   comments, but rewriting lines is more work than writing a fresh file.
6. **Replays.** The `setting` lines in the input log change its format, and that needs a
   version bump upstream. Is latching once per frame acceptable for every setting the guest
   reads, or do some need to change mid-frame?
7. **Recordings.** Should `--record` and frame dumps include HUD overlays such as a speedrun
   timer? Leaving the menu out is clear; a timer might be wanted.
8. **Headsets.** `--vr`, Android and visionOS have no menu in this design. `settings.txt`
   still applies there. Drawing ImGui onto the theater panel is possible later.
9. **Fonts and scale.** ImGui's built-in font is tiny on a 4K screen. Ship a redistributable
   TTF (an OFL font, say) or rely on scaling? Pick one before the first release.
10. **Music and voice.** Whether Star Fox Assault streams its music (DTK) or plays it, and
    the voices, through the DSP decides how much of the Audio page is generic. That's
    milestone 1's audio work.
11. **Cosmetics.** Recoloring the Arwing and the characters means a renderer-level color
    override keyed by texture hash (generic, and close to what texture packs need) or
    writing the game's own material colors (project). Which one depends on how the game
    draws them.
12. **Save editor safety.** The game keeps a copy of the save in RAM. Edit that copy on the
    guest thread and let the game save it, or edit the memory card file while the game
    isn't touching it? The first is safer, but needs the game's save layout.
13. **Save states.** They aren't on the roadmap, and the runtime's host-thread design makes
    them hard. Out of scope, unless upstream plans them.

## Draft: a proposal to gcn-recomp

Not posted. This is a draft for the user to review, change and post to
ethannicholas/gcn-recomp if and when they decide to; link it from issue #10 once it exists.

````markdown
**Title:** Proposal: a shared in-game overlay menu and settings file

Hi! I'm working on a gcn-recomp port of another GameCube game (sfassault-recomp), and it
will need an in-game options menu, as I expect the other ports will. Today every option in
the runtime is a command-line flag, a `GCN_*` variable or a text file, which is fine for
development but hard for players. Rather than build a menu that only one game can use, I'd
like to ask whether you'd take a generic one here. I'm offering to write it, to your
standards, if you're open to it.

**What it would be** (all behind a `GCN_OVERLAY` option, on by default for the desktop
executable):

- Dear ImGui (MIT), fetched and pinned the way the OpenXR headers are, with its SDL2 and
  OpenGL 3 backends set up in `main.cpp` on the existing window and context.
- Input capture: F1 or Esc, or the controller's Back button (which the default layout
  leaves unmapped), toggles the menu. While it's open the game gets a pad at rest (the gate
  is on the `PadState` in `update_pad()`, since the keyboard and gamepad are polled) and
  the menu is navigable by gamepad. With the overlay built, Esc would open the menu
  instead of quitting, with a setting to restore the old behavior.
- Pausing: a gate where the guest thread ends its frame (beside the guest frame hook in
  `fifo.cpp`), a clock rebase on resume so `clock_pace()` doesn't catch up, the audio
  device paused, and a `render_present_again()` so the overlay can redraw over the last
  frame while no new one arrives.
- A settings registry in the core (no SDL or GL, so games' `SOURCES` still link into the
  benchmark): typed settings with a key, a menu path, a label, help text and flags,
  registered from static initializers like `vr::GameHooks`. Saved to a `settings.txt` of
  `key = value` lines (only values changed from the defaults), with `--set key=value` and
  `--list-settings`.
- Settings the guest reads would be latched once per guest frame and recorded in the input
  log, like `pose` lines, so replays stay exact.
- Generic pages for what the runtime itself does: window mode, VSync, internal scale,
  output aspect, master/music/effects volume, controller bindings (editing `controls.txt`),
  rumble, and an FPS/frame-time and audio-buffer statistics page.

Nothing in it would name a game: games add their own entries from `SOURCES`, the way they
already add hooks, and the code would pass `tools/check_generic.py`. (I'd also add this
game's terms to `tools/game_terms.txt`.)

**A smaller alternative**, if you'd rather not carry ImGui or a settings system: just a
frontend hook struct (`on_init`, `on_event` returning whether it consumed the event,
`filter_pad`, `after_present`), plus the pause gate, the clock rebase and
`render_present_again()`, which are needed either way. Each port would then bring its own
menu.

**Questions for you:**

1. Full overlay, hooks only, or neither?
2. Is the end of the guest frame the right place for a pause gate, or is there somewhere
   safer?
3. Settings next to `controls.txt` in the working directory, or a per-user folder
   (`SDL_GetPrefPath`) for saves and settings together?
4. Is a new `setting` record in the input log acceptable, with a version bump?

I'd send it as a series of small PRs (pause and re-present first, then the registry, then
the overlay, then the pages), if that suits you.
````

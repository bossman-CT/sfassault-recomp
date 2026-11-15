# 120 fps (issue #12)

Status: **at the 120 setting, mission 1 plays at 120 fps at the game's own speed, from boot
to the flight.** It is off by default, and with it off the game is unchanged (the
regression check reports SAME). One patch does the work: it tells the game's time step that
it runs at 120 frames a second. The front end's menus, loading screen and briefing keep the
original rate, since they count frames the patch doesn't reach. This page records what was
found, the evidence, and what is left.

| Scene | Original | At 120 |
|---|---|---|
| Play (mission 1's flight) | 60 | 120 |
| Cutscenes in missions | 30 | 60 |
| Boot logos | 30 | 60 |
| Title and menus (`m2.rel`, module 6) | 60 | 60 |
| Loading screen and briefing (`m5.rel`, module 9) | 30 | 30 |

## Turning it on

In the menu: **Graphics > Display > Frame rate: 120** (`graphics.frame_rate` in
`settings.txt`, or `--set=graphics.frame_rate=120` for one run). It is 60 by default. It can
change while the game runs. The change takes effect at the next frame boundary and is
logged in the input log like any setting the guest sees, so a replay switches where its
recording did and stays exact.

At 120, `src/frame_rate.cpp` does three things, except while the front end's menus or
briefing are loaded: it makes the VI retrace twice a field (`vi_set_rate(2)`), makes the CPU
twice as fast (`clock_set_cpu_scale(2)`), and sets the step scale to 0.5. It decides at each
frame boundary from the guest's own module list (the SDK's at `0x800030C8`), so a replay
switches at the same frames. These environment variables are for testing:

- `SFASSAULT_FPS=120` forces the 120 setting, and replays by virtual time
  (`input_replay_by_time`), so a route recorded at 60 lands its inputs at the same moments.
- `SFASSAULT_FPS_AT=<seconds>`, with it, holds off until that much virtual time has passed.
- `SFASSAULT_FRAMELOG=<file>` writes each presented frame's number and virtual time.

## How the game paces itself

The main loop's frame ends in `fn_801759C0` (three variants sit beside it, up to
`0x80175F90`). It calls `VISetNextFrameBuffer`, `VIFlush` and `VIWaitForRetrace`, then
waits until a set number of retraces has passed since the last frame. That number is a
`u32` at `r13-0x6780`: 1 in play, 2 in some cutscenes. It is read only by these pacers.
VI functions here are unnamed. By SDK order and shape: `VIWaitForRetrace` = `0x80140570`,
`VIGetRetraceCount` = `0x801417EC`, `__VIRetraceHandler` = `0x8013FB18`.

- `GCN_CPU_SCALE=2` alone changes nothing: still 60 fps. Play takes about 12% of a field
  per frame, so it was never CPU-bound.
- With the VI at twice the rate (gcn-recomp `GCN_VI_RATE`, added for this), play runs at
  120 fps and the 30 fps cutscenes at 60.

## The game's time step

The game keeps its frames per second in `sda-30252`. It is 60 at boot (`8000E13C`), and
other code sets it, to 30 for instance (`8003AFEC`), presumably where a scene runs at
half rate. Every frame, the main loop (`8000E4D0`) calls `fn_8003AEB0`, which derives
from it:

| Variable | Value | Used as |
|---|---|---|
| `sda-30236` | `0x10000/fps` | 1/fps s in 16.16; `main` adds it to the game clock at `sda-30176` every frame, and a dozen timers add it too |
| `sda-30244` | `0x3C00/fps` | |
| `sda-30240` | `c/fps` (float) | read at 384 sites |
| `sda-30232` | `c'/fps` (float) | a frame multiplier, read at 509 sites |
| `sda-30248` | fps (float) | |

The patch at `8003AEC4` (`recomp/patches.txt`) makes `fn_8003AEB0` read fps divided by
the step scale. That gives 120 at 120 fps (60 where the game asks for 30) and exactly the
game's value at 60, so everything derived from it follows.

## Evidence

Method: replay the `fortuna` route at 60 and at 120 fps, both by virtual time. Dump the
frames each run shows at the same moments, using `SFASSAULT_FRAMELOG` and then gcn-recomp's
`GCN_DUMP_FRAMES`, and compare them side by side. Give each run its own copy of the route:
a replay writes to a scratch copy of the memory card inside the route's folder, and two
replays of one folder at once race on it. One such 60 fps run reached the title 20 s late.

- **Without the patch** (VI rate only, switching at 100 s): everything at double speed.
  The 120 run reaches Fortuna's surface by 230 s, while the 60 run is still in space until
  310 s; then it hits game over.
- **From boot, as shipped** (`SFASSAULT_FPS=120`), every 4–6 s from 20 to 152 s:
  - **Title, menus, briefing and loading screen:** identical frames at the same moments.
    The rate switches where the 60 run links and unlinks those modules (60 at 12 s, 120
    at 41 s, 30 at 46 s, 120 at 95 s).
  - **The flight:** the radio lines on a fixed schedule appear at the same moments at both
    rates ("All ships! Avoid…" at 110 s, "Uh-oh! I'm in trouble" at 122 s, "Andross's ships
    are being destroyed…" at 134 s, "Krystal and Fox, hit the right…" at 146 s, "All
    right! Hit 'em hard…" at 152 s). The camera, the planet and the fleets match. Lines
    triggered by events in the fight run at most a line apart in between.
  - **The hit counter** climbs more slowly at 120: 11 by 152 s against 98. The same inputs
    don't hit the same enemies, so something in the player's aiming or firing may still
    step per frame, or it may just be the drift of an open-loop replay. Not yet told apart.
  - **After about 170 s** (seen with the switch at 100 s) the 120 run's player dies where
    the 60 run's didn't, and the runs part.
- **Before the patch**, a per-site list of 239 steps from the flight was built (below).
  With it, the script still ran twice as fast and the hit counter showed broken digits.
  On top of the patch it garbles the counter again, so it isn't used.

Frame dumps stay under `build/` and the scratch folders; they are game imagery, not for
the repository.

## Not covered yet

- **Menus and briefing at 120.** They run at the original rate instead. At 120 the menus'
  cursor repeat counted frames:
  - From boot, a replay at 120 parted at 36 s on Select Player. The cursor went one entry
    further, to "Guest" instead of "New User", and from there into Name Entry.
  - The 22 frame-driven integer sites over that stretch, scaled as steps, didn't fix it,
    and they broke rendering.
  - At twice their rate, the loading screen and briefing ran 6 s ahead by the briefing's
    first lines.
- **Other front-end screens** (VS mode, Options, Bonus) are assumed to live in the same
  module; not checked.
- **The pause menu in a mission** runs at 120; its two entries have not been tried there.
- **Cutscenes in missions** run at 60 (twice their 30) with the time step told 60; their
  speed and voice sync are not checked. The flight-to-Fortuna cutscene at 300 s was not
  reached in step.
- **Not checked at 120:** the player's firing and aiming (see the hit counts), Fortuna's
  surface, the boss, other missions, and on foot and Landmaster.
- **Sound.** It is driven by time and should be unaffected, but no 120 fps run has been
  listened to (all runs were silent).

## Finding what still steps per frame

1. `python -I tools\find_steps.py build\gen` lists the static candidates (7,840): a load,
   an op `translate_step` can scale, and a store back to the same address.
2. On a `-DGCN_WATCH` build, run `GCN_STORE_HIST` over the same stretch of virtual time at
   60 (`GCN_REPLAY_BY_TIME=1`) and at 120 (`SFASSAULT_FPS=120`, `SFASSAULT_FPS_AT`). Take
   the frame windows from each run's `SFASSAULT_FRAMELOG`. A site that runs about twice
   as often at 120 is frame-driven; one that runs as often is driven by time (sound,
   timers) and must not be scaled.
3. Leave out sites in functions that read the time step variables above: the patch
   already covers them.
4. Try a list with `GCN_STEP_ONLY` / `GCN_STEP_SKIP` on a build whose `steps.txt` holds
   it. A broken site shows as a run whose frame log stops. Run a few subsets in parallel
   for about a minute each.

`steps.txt` keeps the sites known not to scale. After adding a table for the first time,
re-run cmake: ninja only learns of new table files at configure time.

## Follow-ups

- gcn-recomp: recording during a replay sets the RTC to the current time, not the
  replayed log's epoch (`main.cpp` starts the log after loading the replay, and
  `input_log_start` calls `rtc_set_base(now)`). An extended route then sees another date
  than its recording did.
- `--sample` on the `fortuna` route faulted (host null dereference) at about frame 23000,
  just after `m9.rel` was linked a second time. Plain replays don't fault.

## Future work

Truly unlocked rates (any refresh rate, through frame interpolation) are out of scope.
They would need the game's transforms for two frames and a blend between them. The time
step could also take other multiples (180, 240) with a VI rate to match, but nothing
here has tried that.

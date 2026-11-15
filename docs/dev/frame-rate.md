# 120 fps (issue #12)

Status: **mission 1's flight runs at 120 fps at the game's own speed, but the menus don't
yet.** It is off by default, and with it off the game is unchanged (the regression check
reports SAME). One patch does the work: it tells the game's time step that it runs at
120 frames a second. Menus still step per frame, so from boot a 120 fps replay goes
elsewhere in them. This page records what was found, the evidence, and what is left.

## Turning it on

```
$env:SFASSAULT_FPS = '120'; .\build\sfassault.exe
```

`src/frame_rate.cpp` reads it. At the first frame it does four things: it makes the VI
retrace twice a field (`vi_set_rate(2)`), makes the CPU twice as fast
(`clock_set_cpu_scale(2)`), sets the step scale to 0.5, and switches a replay to keying by
virtual time. Two settings exist for testing:

- `SFASSAULT_FPS_AT=<seconds>` switches only once that much virtual time has passed, so a
  replay gets through the menus first. 100 s is mission 1's loading screen on the
  `fortuna` route.
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

Method: replay the `fortuna` route at 60 and at 120 fps (switching at 100 s), both by
virtual time. Dump the frames each run shows at the same moments, using
`SFASSAULT_FRAMELOG` and then gcn-recomp's `GCN_DUMP_FRAMES`, and compare them side by
side.

- **Without the patch** (VI rate only): everything at double speed. The 120 run reaches
  Fortuna's surface by 230 s, while the 60 run is still in space until 310 s; then it
  hits game over.
- **With the patch**, 102–166 s, every 4 s:
  - The radio lines on a fixed schedule appear at the same moments at both rates
    ("Cornerian Army now commencing third volley…" at 130 s, "Andross's ships are being
    destroyed…" at 134 s, "Krystal and Fox, hit the right…" at 146 s, "All right! Hit 'em
    hard…" at 150 s).
  - The camera, the planet and the fleets match.
  - Lines triggered by events in the fight run a line apart for a while (114–126 s).
  - The hit counter climbs more slowly at 120: 12 by 142 s against 50. The same inputs
    don't hit the same enemies, so something in the player's aiming or firing may still
    step per frame, or it may just be the drift of an open-loop replay. Not yet told apart.
  - After about 170 s the 120 run's player dies where the 60 run's didn't, and the runs
    part.
- **Before the patch**, a per-site list of 239 steps from the flight was built (below).
  With it, the script still ran twice as fast and the hit counter showed broken digits.
  On top of the patch it garbles the counter again, so it isn't used.

Frame dumps stay under `build/` and the scratch folders; they are game imagery, not for
the repository.

## Not covered yet

- **Menus.** Cursor movement and repeat step per frame, so from boot a 120 fps replay of
  `fortuna` goes into Name Entry and User Settings instead of the briefing. Playing live,
  the cursor would move at twice the speed.
- **The loading screen and briefing before mission 1:** at 106 s the 120 run already shows
  MISSION START, while the 60 run is still on the loading screen.
- The player's firing and aiming (see the hit counts), Fortuna's surface, the boss, other
  missions, on foot and Landmaster, and cutscene and voice sync. None has been checked at
  120.
- Sound. It is driven by time and should be unaffected, but no 120 fps run has been
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

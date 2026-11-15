# 120 fps (issue #12)

Status: **not at the right speed yet.** The mechanism works, and it is off by default,
where the game is unchanged (the regression check reports SAME). At 120 fps, mission 1's
flight presents a steady 120 frames a second, and 239 per-frame steps are halved. But the
mission's script still runs at about twice its speed, so the game is not playable that
way yet. This page records what was found, what is covered, and how to carry on.

## Turning it on

```
$env:SFASSAULT_FPS = '120'; .\build\sfassault.exe
```

`src/frame_rate.cpp` reads it. At the first frame it does four things: it makes the VI
retrace twice a field (`vi_set_rate(2)`), makes the CPU twice as fast
(`clock_set_cpu_scale(2)`), sets the step scale to 0.5, and switches a replay to keying by
virtual time. Two settings exist for testing:

- `SFASSAULT_FPS_AT=<seconds>` switches only once that much virtual time has passed.
  Menus aren't covered, so a replay at 120 from boot takes a different path through
  them. Switching at 100 s, on mission 1's loading screen, reaches the flight in the same
  state at both rates.
- `SFASSAULT_FRAMELOG=<file>` writes each presented frame's number and virtual time.

## How the game paces itself

The main loop's frame ends in `fn_801759C0` (three variants sit beside it, up to
`0x80175F90`). It calls `VISetNextFrameBuffer`, `VIFlush` and `VIWaitForRetrace`, then
waits until a set number of retraces has passed since the last frame. That number is a
`u32` at `r13-0x6780`: 1 in play, 2 in some cutscenes (the 30 fps one in #7). It is
written at `0x80173E00` and `0x801745F4` and read only by these pacers. Nothing else in
the game scales by it, so there is no global time step to change. VI functions here are
unnamed. By SDK order and shape: `VIWaitForRetrace` = `0x80140570`,
`VIGetRetraceCount` = `0x801417EC`, `__VIRetraceHandler` = `0x8013FB18`.

- `GCN_CPU_SCALE=2` alone changes nothing: still 60 fps. Play takes about 12% of a field
  per frame, so it was never CPU-bound.
- With the VI at twice the rate (gcn-recomp `GCN_VI_RATE`, added for this), play runs at
  120 fps and the 30 fps cutscenes at 60. Every per-frame step then runs twice as often,
  so the one step scale of 0.5 suits both.

## Comparing 60 and 120 fps

The replay key was the problem. A "poll" is every guest read of the SI pad registers, so
at 120 fps the game polls twice as often. A route replayed by poll fed the inputs per
game frame, twice as fast in time: the 14-minute `fortuna` route ran out at 470 s.
gcn-recomp's input log v6 now records each poll's virtual time, and a replay by time
(`GCN_REPLAY_BY_TIME=1`, or `input_replay_by_time()`) lands every input at the same
instant at any frame rate. Older logs replay as before. To make a v6 copy of a route,
replay it once at 60 fps with logging on (`--input-log=<dir>`). Recording during a replay
gives the copy the epoch of that run (see Follow-ups). If the old log had an end marker,
add one by hand: `# end <poll> <frame> <tick>`.

Frames are then compared at the same moments:

1. Replay at both rates with `SFASSAULT_FRAMELOG`.
2. For each moment, take the first frame at or after it from each log.
3. Replay again with gcn-recomp's `GCN_DUMP_FRAMES=<list>` to dump exactly those frames.

At 120 fps the game is on retrace every frame, so after the switch the frame numbers are
the same from run to run.

Results on `fortuna` (switch at 100 s; every 20 s from 110 to 410 s):

- **No steps:** everything at double speed. The 120 run reaches Fortuna's surface by
  230 s, while the 60 run is still in space until 310 s; then it hits game over.
- **239 steps (this list):** the opening radio lines now line up (at 110 s both runs show
  the same line, where before the 120 run was already past it). But the script still
  advances about twice as fast: "Oikonny seems to have fled" comes at 212 s instead of
  310 s. The HUD hit counter shows broken digits, and after about 300 s the run falls
  back to the title screen. The player's own flight looks plausible; the timing of
  events doesn't.

## What is covered

`recomp/steps.txt` lists 179 float steps and 60 integer counters from mission 1's space
flight. Its header gives the selection; in short:

1. `tools/find_steps.py` lists 7,840 static candidates from `build/gen`: a load, an op
   `translate_step` can scale (`fadd(s)`, `fsub(s)`, `fmul(s)`, `fdiv(s)`, `fmadd(s)`,
   `addi ±1`, …), and a store back to the same address in one basic block.
2. `GCN_STORE_HIST` (on a `-DGCN_WATCH` build) over the same 70 s of flight, 115–185 s,
   at 60 and at 120 fps. 1,051 candidates ran on at least 30 frames in a row at 60. A
   frame-driven site runs about twice as often at 120 (ratio ≥ 1.6); a time-driven one
   (the sound library, timers) runs as often (ratio ≈ 1) and must not be scaled.
3. Excluded: the SDK (`0x8012D438–0x8015B000`, and from `0x801E8D48`), integer sites that
   run more than once a frame (loop counters, cursors), and the four integers listed at
   the end of `steps.txt`. Taken every other frame, those four hang the mission; they
   were found by bisecting with `GCN_STEP_SKIP`.

Not covered:

- **The mission's script timers.** The script still runs at twice its speed. The
  likeliest places, none in the list yet:
  - integer `add`/`subf` forms (excluded wholesale);
  - integer sites that run several times a frame (one per object or script thread);
  - the four that hang, which may count in a way that needs a different translation;
  - the 111 "other" floats with ratios between 0.75 and 1.6 (scene-dependent loops).

  The HUD's broken digits suggest a counter that is now off by a half step.
- Menus (cursor repeat), Fortuna's surface, other missions, on foot and Landmaster,
  cutscene and voice sync. None of these has been looked at.
- Sound. It is time-driven and should be unaffected, but no 120 fps run has been
  listened to (all runs were silent).

## Finding more

```
python -I tools\find_steps.py build\gen > candidates.txt
# a -DGCN_WATCH build in build-watch (cmake ... -DCMAKE_C_FLAGS=-DGCN_WATCH -DCMAKE_CXX_FLAGS=-DGCN_WATCH)
$env:GCN_REPLAY_BY_TIME='1'; $env:GCN_STORE_HIST='<a>-<b>'          # the 60 fps window
$env:SFASSAULT_FPS='120'; $env:SFASSAULT_FPS_AT='100'; $env:GCN_STORE_HIST='<c>-<d>'  # same virtual window at 120
```

Take the frame windows for one stretch of virtual time from each run's `SFASSAULT_FRAMELOG`.
To try a list without a rebuild, use `GCN_STEP_ONLY` / `GCN_STEP_SKIP` (files of
addresses) on a build whose `steps.txt` holds every candidate. A broken site shows as a
run whose frame log stops: it hangs, or its virtual time leaps to the route's end. Run a
few such subsets in parallel for about a minute each. To find a script timer, it may be
quicker to diff RAM (`GCN_RAM_DUMP_AT`) across a few frames of a radio line, and then
find the counter's writers with `GCN_WATCH_ADDR`.

After adding a table such as `steps.txt` for the first time, re-run cmake: ninja only
learns of a new table at configure time.

## Follow-ups

- gcn-recomp: recording during a replay sets the RTC to the current time, not the
  replayed log's epoch (`main.cpp` starts the log after loading the replay, and
  `input_log_start` calls `rtc_set_base(now)`). An extended route then sees another date
  than its recording did.
- `--sample` on the `fortuna` route faulted (host null dereference) at about frame 23000,
  just after `m9.rel` was linked a second time. Plain replays don't fault.

## Future work

Truly unlocked rates (any refresh rate, through frame interpolation) are out of scope.
They would need the game's transforms for two frames and a blend between them; the step
scale only covers integer multiples of the native rate.

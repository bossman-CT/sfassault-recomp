# 120 fps (issues #12, #59)

Status: **at the 120 setting, missions run at twice their original frame rate at the
game's own speed: flight at 120 (mission 1 checked), on foot at 60 (mission 2 checked).**
It is off by default, and with it off the game is unchanged (the regression check reports
SAME). Two sets of patches do the work. The game's frames-per-second variable holds twice
the game's value, so everything that converts time to frames by it follows. The game's
retrace callback, which paces it, counts a 120 Hz retrace as half a field. The front end's
menus, loading screen and briefing keep the original rate, since they count frames these
don't reach. This page records what was found, the evidence, and what is left.

| Scene | Original | At 120 |
|---|---|---|
| Flight (mission 1) | 60 | 120 |
| On foot (mission 2) | 30 | 60 |
| Cutscenes in missions | 30 | 60 |
| Boot logos | 30 | 60 |
| Title and menus (`m2.rel`, module 6) | 60 | 60 |
| Loading screen and briefing (`m5.rel`, module 9) | 30 | 30 |

The setting's help line in the menu gives these rates, since "60" and "120" read as frame
rates and the original's 30 on foot looks like a bug at "60". The option names stay "60"
and "120": a choice is saved by name in `settings.txt` and in input logs, and recordings
made at 120 must still replay.

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

The game's log (stderr) says when its frames per second or its pacers' retrace count
change: `[sfassault] game fps 30 (shown 60), 0 retraces a frame`.

## How the game paces itself

The main loop (`fn_8000DE88`) keeps a frame budget in `sda-30176`, in 16.16 seconds. Each
frame adds `0x10000/fps` to it (`sda-30236`, from the time step below). Before the frame
ends, the loop (`8000E540`) waits on `VIWaitForRetrace` until the budget is paid. The
game's retrace callback, `fn_8000DE20`, registered at boot (`8000E128`), pays it back: 1092
(`0x10000/60`, a 60 Hz field) every retrace, down to -1092. So a frame at fps 60 lasts
one field and at fps 30 two: on foot runs at 30 that way, steady, at 28% of a field of
work. The same callback counts retraces (`sda-30272`, read by a dozen sites), keeps a 16.16
clock (`sda-30260`, +1092 a retrace) and a once-a-second frame meter (`sda-30168`, 60
retraces).

The frame then ends in `fn_801759C0` (three variants sit beside it, up to `0x80175F90`).
It calls `VISetNextFrameBuffer`, `VIFlush` and `VIWaitForRetrace` once, then waits until a
set number of retraces has passed since the last frame. That number is a `u32` at
`r13-0x6780` (`0x803869A0`), read only by these pacers. In every run for #59 it stayed 0,
which waits for no more than the one retrace. VI functions here are unnamed. By SDK order
and shape: `VIWaitForRetrace` = `0x80140570`, `VIGetRetraceCount` = `0x801417EC`,
`__VIRetraceHandler` = `0x8013FB18`.

- `GCN_CPU_SCALE=2` alone changes nothing: still 60 fps. Play takes about 12% of a field
  per frame, so it was never CPU-bound.
- With the VI at twice the rate (gcn-recomp `GCN_VI_RATE`, added for this), the retrace
  comes twice a field. The callback then pays 1/60 s every 1/120 s, so any scene paced by
  the budget ran at 120 with its time step told 60: twice its speed (#59). At 120 the
  callback's patches (`8000DE20`–`8000DE50`, `recomp/patches.txt`) pay 546 a retrace and
  take its counts on every second retrace, so every retrace stands for half a field.

## The game's time step

The game keeps its frames per second in `sda-30252` (`0x80385AF4`). It is 60 at boot
(`8000E13C`), and 13 other stores set it as scenes change: 30 in the briefing (`m5.rel`),
the boot logos and as mission 2 loads (`m8.rel`, `818897C0`), 60 elsewhere. Five of
those are in `main.dol` through `r13`. The RELs reach it through `0x80380000 + 0x5AF4`,
some through a register that holds that address. Every frame, the main loop (`8000E4D0`)
calls `fn_8003AEB0`, which derives from it:

| Variable | Value | Used as |
|---|---|---|
| `sda-30236` | `0x10000/fps` | 1/fps s in 16.16; `main` adds it to the game clock at `sda-30176` every frame, and a dozen timers add it too |
| `sda-30244` | `0x3C00/fps` | |
| `sda-30240` | `c/fps` (float) | read at 384 sites |
| `sda-30232` | `c'/fps` (float) | a frame multiplier, read at 509 sites |
| `sda-30248` | fps (float) | |

Besides the time step, 84 sites read the variable itself, 40 in `main.dol` and 44 in the
RELs. Each turns a duration or a rate into frames (`t * 60 / fps` for a timer or
a turn rate, `fps * 10` frames for ten seconds, `0x10000 / fps`). A first patch for #12
made only `fn_8003AEB0` read fps divided by the step scale (`8003AEC4`), so at 120 these
readers still saw 60 or 30, and their timers ran twice as fast.

Now every store to the variable goes through `sfassault_fps_store` (`src/frame_rate.cpp`),
which stores the value divided by the step scale and remembers the game's own. A change of
rate rewrites the variable from it. The variable then holds 120 at 120 fps where the game
asks for 60, 60 where it asks for 30, and exactly the game's value at 60, so every reader
follows, in any module. At each frame boundary the variable is checked against what was
last stored: a store the patches miss would show there, and is logged and corrected. The
first run found four that way (stores through a register holding the address), which are
patched now.

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

### Mission 2 on foot (#59)

At 120, mission 2 on foot ran at twice its speed while its sound kept time. On foot runs
at 30 in the original, paced by the frame budget (above). At 120 the retrace callback paid
the budget back twice a field, so on foot ran at 120 frames a second with its time step
told 60. The fps variable's direct readers saw 30 besides.

Method: the user's recording of a session (boot to mission 2 at 120; then, in a pause, a
switch to 60 and on through the mission), replayed by virtual time three ways. At 60, with
its switch to 120 edited to 60 (`GCN_REPLAY_BY_TIME=1`). At 120 on `main`, which had only
the patch at `8003AEC4`. At 120 with these patches. Both 120 runs used `SFASSAULT_FPS=120`
and `SFASSAULT_FPS_AT=114`, where the recording switched. The frames each run showed at the
same moments were compared, with the frame dumps paired by `SFASSAULT_FRAMELOG`.

- **Frames a second on foot:** 30 at 60, 119 on `main` at 120, 59 with the fix.
- **On `main` at 120**, the mission clock on the first pause read 4 s where the 60 run's
  read 1 s.
- **With the fix:**
  - "Mission Start" and the "Frontier Base Battle" card appear at the same moments
    (232–234 s, after the Start press that skips the intro).
  - At 1870 s Peppy's "Fox, first get rid of the aparoids…" is on screen in both runs, with
    Fox in the same place.
  - At the next pause (1890 s) the mission clock reads 22 s in both.
  - The hit counts differ (4 against 2), as an open-loop replay drifts.
- **Loading is quicker at 120:** it is CPU work, and the CPU runs twice as fast. Mission 2's
  intro cutscene therefore starts a few seconds earlier, and the Start press that skips it
  lines the runs up again.

### Mission 1 again, with the #59 patches

The `fortuna` comparison above, repeated (`fortuna5`, the v6 route, by virtual time):

- **Front end:** the title, menus and briefing show identical frames at the same moments up
  to 92 s.
- **Flight:** the scripted radio lines appear at the same moments ("All ships! Avoid…" at
  110 s, "Krystal and Fox, hit the right…" at 146 s).
- **Hit counter:** it still trails at 120, 11 against 82 by 146 s.
- **Boot logos:** they now run at 60. On `main` they ran at 120, since they are paced by the
  budget too.
- **Loading screen (95–105 s):** its text types out behind the 60 run's, while its progress
  bar runs ahead. `main` shows the same, so it predates #59. The flight starts on time.

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
    first lines. The briefing runs at 30, paced by the frame budget, so that was likely
    the retrace callback fixed for #59. Worth trying again.
- **Other front-end screens** (VS mode, Options, Bonus) are assumed to live in the same
  module; not checked.
- **The pause menu in a mission** runs at 120; its two entries have not been tried there.
- **Cutscenes in missions** run at 60 (twice their 30) with the time step told 60; their
  speed and voice sync are not checked. The flight-to-Fortuna cutscene at 300 s was not
  reached in step.
- **Not checked at 120:** the player's firing and aiming (see the hit counts), Fortuna's
  surface, the boss, missions after 2, the Landmaster, and the Arwing in mission 2.
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

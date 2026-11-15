# Testing without playing through again

Every run of the game records its controller input, and gcn-recomp replays a recording
exactly: the same inputs land on the same frames, so the game does exactly what it did. That
makes three things cheap.

## Routes: come back to a point in the game

Save a run you want to return to as a named route:

```powershell
.\tools\route.ps1 save fortuna-plaza                      # the newest recording
.\tools\route.ps1 save fortuna-plaza -From saves\inputs\20261103-201727
.\tools\route.ps1 list
```

Play on from where a route ends:

```powershell
.\tools\route.ps1 play fortuna-plaza -Fullscreen
.\tools\route.ps1 play fortuna-plaza -Fullscreen -Scale 4   # at 2560x1920 internally
```

The game fast-forwards through the route (`--fast-forward`: unpaced while the replay lasts,
then real time), and the controller is yours when it runs out. Getting there takes a little
over a quarter of the route's length: fast-forward runs at about 3.5x real time, limited by
the game and the rendering, which still run in full so the replay stays exact. Playing on is recorded as a new run, so `route.ps1 save` it under a
new name to make a longer route. Saves made while playing on go to the route's own copy of
the memory card, not your real one.

## Regression check: did a change break anything?

```powershell
.\tools\regress.ps1               # every route
.\tools\regress.ps1 -Route fortuna-plaza
.\tools\regress.ps1 -Update       # the change was intended: make this the new baseline
```

Each route is replayed hidden and silent, as fast as the PC allows, and every 120th frame is compared
with the route's baseline. The first run of a route records the baseline. The result for
each route is `SAME`, `CHANGED` (with side-by-sides of the changed frames in
`build\regress\<route>-changes`), or `STOPPED` (the replay ended early, which is usually a
crash, and the game's error line is shown). The frame comparison needs Pillow
(`pip install pillow`).

A visual fix should show up as `CHANGED` in the places it was meant to, and nowhere else.

## Comparing two builds at one moment

For a closer look than the regression check, replay one stretch with frame dumps on each
build and compare them:

```powershell
$env:GCN_DUMP_RANGE = '32800-34400'
.\build\sfassault.exe --replay=saves\routes\fortuna --hidden --fast --no-input-log --scale=1 --dump-dir=out\a --dump-every=30
```

Then do the same with the other build or setting into `out\b`, and run
`python tools\frame_diff.py out\a out\b out\changes`. Settings that only change rendering,
such as `GCN_TEXCOORD_SCALE=texture`, can be compared on the same build.

## Determinism check: do two runs stay identical?

```powershell
.\tools\determinism.ps1 -Route fortuna -A '--scale=1' -B '--scale=4'
```

This replays a route in two instances at once, with their own arguments, and compares hashes
of their guest memory every 60 frames (`GCN_STATE_HASH`). It reports `SAME`, or the first frame
where they part and which megabytes of memory differ. Online play depends on this
([netplay.md](netplay.md)).

## What stays on your machine

Recordings, routes, baselines and frame dumps hold your inputs, memory card data and images
of the game. They live under `saves\` and `build\`, which git ignores, and must never be
committed or attached to issues. Describe what a frame shows, or attach a cropped
screenshot instead.

## Limits

- **No save states.** gcn-recomp runs the game's threads as host threads, so their state
  can't be saved and restored. Fast-forwarding a route is the substitute.
- **Replays follow the inputs, not the outcome.** A change to the game's own code (a patch
  in `recomp/patches.txt`) can make a replay go somewhere else. The runtime prints
  `the replay has parted from its recording` when it notices. Rendering changes don't
  affect replays. Audio changes usually don't, though the game does read some audio state
  (the talking portraits follow a bus's level).

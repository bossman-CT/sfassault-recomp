# Draw distance

Graphics > Rendering > Draw distance (`graphics.draw_distance`, issues #76 and #28). A slider
from Original (1x) through 2x to 8x, then Unlimited. The default is Original (see below for why).

## What the game does

The game draws a model only within a range of the camera, and fades it out as it nears the
edge of that range:

- each model's draw callback (in the draw pass, under `fn_8003CA20`) asks `fn_80068C7C` how far
  the model is, as a share of its range: the distance from the camera divided by the range, or
  -1 when it lies outside the range or the view;
- `fn_80070B80` turns that share into a strength: full up to 0.8, falling to nothing at 1, and
  nothing beyond or off-screen;
- the callback draws the model only if the strength is above 0, with the strength as its alpha.
  For the fighters that is `fn_80055918`.

A model's range is its own factor (1 for the fighters) times a scene-wide distance in
`0x80385DD4` (sda-29516), plus an offset in `0x80385DD8` (sda-29512) for factors above 1.
`fn_80071054` works the distance out every frame for each view, from the camera's base distance,
its field of view and the number of players, and `fn_80072930` sets it for the view being drawn.
`fn_80071874` and `fn_80071E58` set it up.

In the mission 3 space battle the distance runs between about 100 and 300 units (median 130 over
the Star Wolf fight in the recorded playthrough), while Star Wolf fights 200 to 400 units out.
At frame 27587 of that playthrough the distance is 103: Leon's and Falco's fighters, 230 and
390 units away, get a share past 1. They aren't drawn at all, though their name labels are, and
they pop in only when they come within about 80 units. Panther's fighter is drawn earlier in the
same mission because it is close. This is the original game's design, not a fault of the port:
the renderer receives every draw the game makes, and the distance comes from the game's own state
alone (no hardware readback feeds it).

The same fade applies to every model the game draws this way (enemies, scenery, asteroids), in
every mission. That is the late fade-in on Fortuna (#28).

## What the setting does

Every store the game makes to the two variables (eight sites, all in `recomp/patches.txt`) goes
through `sfassault_draw_range` (`src/draw_distance.cpp`), which multiplies the value by the
setting. At Original it stores the game's own value, bit for bit. Unlimited multiplies by 1000,
which puts every range past the far plane (50,000 units), so nothing is cut or faded by distance.

Only the draw pass reads the two variables: everything that calls the range test in a frame of
the space battle is a draw callback. But drawing more is more work for the guest's CPU, and the
game's timing follows its CPU time on the virtual clock. Replays of the recorded playthrough at
Original and at 4x, with `GCN_STATE_HASH` every 600 frames, agree at each of those frames on the
pad polls and the virtual time (to a tick) through missions 1 to 3, the Star Wolf fight included, with the same HUD
at frame 27587. Between frames 62400 and 67200 (the snow mission, on foot, where 4x draws far more
snow and trees) the 4x run's virtual time moves on by 337,837 ticks, about one 120 Hz retrace: a frame
ran past its budget. From there the two runs play differently: at frame 66098 the Original run is
in the hangar cutscene and the 4x run is still on foot.

So the setting is one the guest sees (`AffectsGuest`): latched at a frame boundary and recorded in
the input log, so a replay draws at the distance it was recorded with and stays exact. Its default
is Original, because a log from before the setting existed replays at the default, and every route
and recording so far was made at the original distance. At a higher value a heavy scene can still
cost the game a frame, as it would on the console; making the virtual CPU faster there is a
possible follow-up.

## Known problem: values above Original can crash

The game builds each frame's draws in a display-list buffer: 3 MB, split into two halves of
1.5 MB that alternate frames, set up once at boot (`fn_8000DE88` calls `fn_80174078` with
0x300000, which calls `fn_801750C4`). Each half holds the frame's display lists from the bottom
and the nodes of its four draw queues from the top (`fn_801753A0`, `fn_80175404`), and
`fn_801751BC` walks the queues at the end of the frame. At the original distance a frame never
fills it. At 4x the `fortuna` route (`SFASSAULT_DRAW_DISTANCE=4`) stops at frame 16680 with an
MMU fault in `fn_801751BC`: a queue node pointer reads as garbage, so the lists and the nodes
have run into each other.

Until that is fixed, only Original is safe. Possible fixes: give the buffer more room (a larger
size at boot, if the game's heap has it), or keep a frame's draws from passing the space left
(`fn_801752A4` returns it, and some callers already check it).

## Testing

- `tools/regress.ps1` (the default, Original) matches the baselines recorded before the setting
  existed, and at Original frame 27587 of the playthrough is byte for byte the frame from before.
- To see a value in a replay, record with it: a replay takes the value from its log, not from
  `--set`.

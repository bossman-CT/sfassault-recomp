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

The game reads the two variables at 23 places, all in model draw code: `fn_80068ABC`,
`fn_80068C7C`, `fn_8006A510`, `fn_8006BA14`, `fn_8006E2A8`, `fn_8006EB8C`, `fn_8006F414`,
`fn_8007017C`, `fn_80070BD0`, `fn_80070C78` and `fn_80088B00`. Each of those either runs under the
draw-callback list (`fn_8003CA20`) in a sampled frame of the space battle, or calls only model and
GX helpers. Every read goes through `sfassault_draw_range` (`recomp/patches.txt`,
`src/draw_distance.cpp`). It returns the value times the setting, or the value itself at
Original, and also when the frame's display-list buffer is running low (below). The game's own
stores are untouched, so the variables always hold its own value. At Original the reads return
it bit for bit. Unlimited multiplies by 1000, which puts every range past the far plane (50,000
units), so nothing is cut or faded by distance.

## The display-list buffer

The game builds each frame's draws in a display-list buffer: 3 MB, split into two halves of
about 1.5 MB that alternate frames, set up once at boot (`fn_8000DE88` calls `fn_80174078` with
0x300000, which calls `fn_801750C4`). Each half holds the frame's display lists from the bottom
and the nodes of its four draw queues from the top (`fn_801753A0`, `fn_80175404`), and
`fn_801751BC` walks the queues at the end of the frame. Nothing stops the two from meeting. At 4x,
with the multiplier on the stores and no guard, the `fortuna` route ran them into each other and
stopped at frame 16680 with an MMU fault in `fn_801751BC` (a queue node read as garbage).

The guard: a read returns the multiplied range only while the frame's half still has half its
room free (`fn_801752A4` works out the free room the same way). At Original the heaviest frames
of the `fortuna` route leave at least 82% free (`SFASSAULT_DRAW_DISTANCE_LOG=1` prints the least
free room every 600 frames). So the extra models only ever use room the game's own models don't
need, and models within the original distance always draw. At Unlimited the route's least free
room was 56%, and it ran past frame 16680 without a fault. That run then parted from its
recording (below), so it is not yet checked to the route's end.

## Timing: values above Original change the game's timing

Drawing more is more work for the guest's CPU, and the game's timing follows its CPU time on the
virtual clock. Every backward branch the recompiled code takes adds to a count (`IRQ_CHECK`,
`g_vcount`), which is the guest's time, and interrupts are polled from it. More models drawn is
more branches, so the frame ends later in guest time. Usually the frame still fits its budget and
nothing changes. In a heavy scene it doesn't: a frame runs past its retrace, and from there the
game runs differently.

- The recorded playthrough at Original and at 4x (multiplier on the stores, `GCN_STATE_HASH`
  every 600 frames): the same pad polls and virtual time (to a tick) at each of those frames
  through missions 1 to 3, the Star Wolf fight included, with the same HUD at frame 27587. Between
  frames 62400 and 67200 (the snow mission, on foot, where 4x draws far more snow and trees) the
  4x run moves on by 337,837 ticks, about one 120 Hz retrace. By frame 66098 the Original run is in
  the hangar cutscene and the 4x run is still on foot.
- The `fortuna` route at Unlimited (multiplier on the reads, guarded): the replay parts from its
  recording at frame 16725, where a recorded clock jump falls at a different poll.

Moving the multiplier from the stores to the reads doesn't change this, as expected: every reader
was already in draw code. So the setting is one the guest sees (`AffectsGuest`): latched at a
frame boundary and recorded in the input log, so a replay draws at the distance it was recorded
with and stays exact. Its default is Original, because a log from before the setting existed
replays at the default, and every route and recording so far was made at the original distance.
Every value above Original is affected.

Making the extra drawing free in guest time would need a gcn-recomp mechanism: a game hook that
stops `IRQ_CHECK` charging `g_vcount` (and polling) while it is held. Here it would be held from
the point a model's range test passes only because of the multiplier until its draw callback
returns. The poll must neither fire inside that window nor be skipped across it. One way is to
save `g_vcount` on entry and restore it on exit, with `g_vlimit` raised meanwhile so no poll
falls inside. The game side needs a hook at each draw callback's exit, or a "charge-free until
the stack unwinds past here" form using r1.

## Testing

- `tools/regress.ps1` (the default, Original) matches the baselines recorded before the setting
  existed (`fortuna`, `title-short`), and at Original frame 27587 of the playthrough is byte for
  byte the frame from before.
- To see a value in a replay, record with it: a replay takes the value from its log, not from
  `--set`. `SFASSAULT_DRAW_DISTANCE=<1 to 9>` forces a value for a test, after which the replay
  may part from its recording (above).

# The sun's lens flare and EFB depth peeks (issue #85)

The game decides whether the sun is hidden with a CPU read of the EFB's depth, `GXPeekZ`
(`fn_80158130`, its only caller `fn_800A1E78`). gcn-recomp answered every depth read with 0, so
the flare never appeared. The runtime now answers with the depth the GPU drew
(gcn-recomp `docs/graphics.md`, "Depth peeks").

## What the game does

For each of up to four viewports it reads nine depths in a 3x3 grid, 10 pixels apart, around the
sun's screen position, after the frame's draws (the markers lie after the last draw of the batch):

- a sample of exactly `0xFFFFFF` is the cleared depth: nothing was drawn there, not even the
  backdrop. The test stops and keeps the previous result;
- otherwise each sample above `0xFFFD00` counts one and each above `0xFFE000` counts one more,
  and the viewport's result is that count scaled to 0..1.

The backdrop is drawn at the far plane and reads `0xFFFFFC` here, so a clear view of the sun
counts all nine samples twice; geometry in front of a sample counts nothing. The result drives the
flare's brightness, and the peeks only occur while the sun is near the screen.

## What changes

- The flare shows when the sun is clear and fades out as geometry covers it. In the `fortuna`
  route it appears around frame 17285; in `mission1-boss` at 20072, 21174 and 31185 to 31231,
  where the boss ship slides over it.
- The value is three frames old (`kPeekLag`), so the flare trails the sun by three frames and
  lingers about three after it is covered.
- The result lands in game memory, and only for a frame or two: it is not carried into later
  frames. Replays at internal scales 1 and 4 hash identically through all of the above; at
  different aspect ratios one state hash differs while the flare fades. For netplay that means
  peers should use the same aspect ratio, or run with `GCN_EFB_PEEK=0` (depth reads give 0, as
  before).

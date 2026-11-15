# Mission 1's boss vanishing (issue #42)

In mission 1, the Aparoid boss on Fortuna disappeared in its final phase and its meteor
attack looped forever, so the mission could not be finished. xSNAIRZx's port of the same
disc lists the same "first-boss visibility" bug with no cause. The cause was in gcn-recomp's
floating-point emulation, not in this game's tables: the recompiled code ignored the
processor's non-IEEE mode.

## What happened

- The boss (module `mb01b`) runs a script: a step byte at +0x124 picks the next action, held
  at +0xF6. In the final phase, steps 10 to 17 loop through actions 1, 5, 1, 6, 1, 6, 1, 4
  until the boss dies. The loop is by design: the boss has to be shot down to end it.
- During the meteor storm (action 5), the boss's position on its 2D plane (floats at +0x118
  and +0x11C) is stepped toward (0, 0) each frame: `pos += normalize(target - pos) *
  speed(distance)`. The distance shrinks by 5% a frame.
- On route `mission1-boss`, at frame 39169 the position was (5.1e-20, -1.6e-20). Its squared
  length, about 2.9e-39, is below the smallest normal single-precision number (1.2e-38). In
  that frame the position became (+inf, -inf), and the boss's matrix (from +0x70) NaN.
  From then on the boss is drawn nowhere and can't be hit, so the final phase never ends.

## Why

The SDK's `OSInit` calls `PPCSetFpNonIEEEMode`, which sets FPSCR[NI]. In that mode the
GameCube's processor returns zero for an arithmetic result that would be denormal. On the
console the squared length becomes 0, the SDK's vector-magnitude routine returns 0, the
speed is 0 and the boss holds still. gcn-recomp kept the denormal. The reciprocal square
root estimate of it (about 1e20) squares to infinity in single precision, the refinement
step gives -inf, and the normalised vector is infinite.

The fix is in our gcn-recomp fork (`2474ece`): every arithmetic result is rounded through
`RND_S`/`RND_D` in `runtime/recomp.h`, which flush to a signed zero while NI is set.
Loads, stores and moves keep denormals, as on the console.

With the fix the position stops at about 1e-19 and the boss comes back down after the
storm (frame ~39540 on the route), visible and hittable. Its script runs as before: at
every RAM snapshot taken, the step and action match the unfixed run's.

## Checking it again

The boss object is at 0x80D48760 on that route (`GCN_RAM_DUMP_AT` in
`gcn-recomp/docs/diagnostics.md`). Its fields +0x118 and +0x11C must stay finite through
the meteor storms: the first one, which broke at frame 39169, and the next, from about
frame 41700.

## Still open

A new guest thread takes its FPSCR from its context in memory, so it starts with NI off.
On the console a thread whose context has no saved FPU state inherits the live FPU state,
NI included. The boss runs on the main thread, which has NI set, so this didn't matter
here.

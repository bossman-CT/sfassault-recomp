# Online versus: netplay (issue #36)

The plan is delay-based lockstep, the way Dolphin's netplay works. Each player runs the game, only
controller inputs cross the network, and every PC computes the same match. That only works if two
instances fed the same inputs stay identical. Phase 1 checks this on one PC. This file has the
results, what has to match at the start, the risks, and a design for phase 2.

## Where phase 1 stands (paused 2026-11-15)

Done: the state-hash tool, and the same-settings, scale and aspect results below.

Left:

- **The regression check isn't done.** `tools\regress.ps1 -Route fortuna,title-short -Every 600`
  hasn't been run against this pin with the hash off. A fresh baseline from main's pin
  (2474ece) was being recorded when work paused. The pin also brings in fork commit 0033216
  (VI rate, replay by virtual time, input log v6) from the 120 fps work (#12, PR #46), so the
  check covers that commit too.
- **No versus route yet** (see "What the test does not cover").

## Phase 1 result: two instances stay identical

Two instances replaying the same route stayed **identical in every byte of guest memory, at every
checkpoint**, even with different internal resolutions and aspect ratios. The instances ran at the
same time on one PC, each with its own copy of the route.

| Run (`tools\determinism.ps1`) | Route | Checkpoints | Result |
|---|---|---|---|
| Same settings twice | `fortuna` (frames 60 to 50,820) | 847, one every 60 frames | SAME |
| `--scale=1` vs `--scale=4` | `fortuna` (60 to 50,520) | 842 | SAME |
| `--aspect=4:3` vs `--aspect=16:9` | `fortuna` (60 to 50,340) | 839 | SAME |
| Same settings twice (smoke test) | `title-short` (10 to 2,660) | 266, one every 10 frames | SAME |

Each checkpoint compares hashes of all 24 MB of main RAM, 16 MB of ARAM, the locked cache, the
virtual time and the pad-poll count. The `fortuna` route covers boot, the title screen, the
briefing, the space battle and Fortuna base, about 14 minutes of play. Each pair took 4 to 5
minutes, with all six instances running at once.

No region of memory differed between runs, so nothing had to be excluded from the hash.

### The tool

- `GCN_STATE_HASH=<every>[:<file>]` (our gcn-recomp fork, `runtime/state_hash.cpp`; its
  `docs/diagnostics.md` has the details). It hashes guest state at the end of every `<every>`th
  submitted frame. It runs on the guest thread at the frame boundary, where no other thread
  writes guest memory, and it only reads. Each log line has the frame, the poll count, the
  virtual time, the overall hash, hashes of RAM, ARAM and the locked cache, and a hash for each
  1 MB block. One hash takes 2.1 ms (2.8 to 3.0 ms with six instances running).
- `gcn-recomp\tools\state_hash_diff.py a.txt b.txt` compares two logs. It reports the first
  frame that differs and which blocks differ there.
- `tools\determinism.ps1 -Route <name> [-A '<args>'] [-B '<args>'] [-Every N] [-UpTo frame]`
  runs both instances (hidden, silent, unpaced, default settings) and compares their logs.
  Output goes to `build\determinism\<tag>\`.
- Finding a divergence: the diff names the frame and the blocks. `GCN_RAM_DUMP_AT=<frame>` on
  each side gives the bytes to compare. `GCN_WATCH_ADDR` on a replay then finds what wrote them.

The checkpoints use submitted frames (the guest's frame boundaries), not presented frames.
The presented-frame count belongs to the render thread, so it isn't part of the guest's state.

### Why the renderer can't cause a desync in this game

The renderer has three ways to write into guest memory. This game uses none of them:

- **EFB copies to RAM.** A copy reaches RAM only if the game installs a write-back filter
  (`gx::set_copy_writeback`, landed on the guest thread two display copies later). This game
  doesn't, so texture copies stay on the GPU. Display (XFB) copies never land in RAM either.
- **EFB peeks.** CPU reads of the EFB go to `efb_peek`. Only the runtime's weak default exists,
  which returns 0, so every PC reads the same 0. That isn't what a console returns. If this game
  turns out to need real peeks, a real implementation would read the GPU's EFB at the
  player's resolution. Netplay would then need the peek answered from a 1x copy (a small
  native-resolution readback of just that pixel's block) so every player reads the same value.
- **Frame pacing.** The guest waits at a frame's end for the front end to catch up
  (`kFeMaxFrames`). Virtual time doesn't move during that wait, so a slower renderer
  (`--scale=4`) changes nothing the guest can see.

The audio device thread doesn't touch guest memory either. It only drains a host-side sample
ring. AX mixing runs when the guest's DSP mail asks for it, on the guest's timeline.

### What the test does not cover

- **Versus mode itself.** No versus route exists yet. Recording one needs two players (or a
  `GCN_INPUT` script with `2/` presses) through the versus menus. Then
  `.\tools\determinism.ps1 -Route <versus-route>` checks it like `fortuna`. This is the next
  step to finish phase 1 as the issue describes it ("a full versus match on a few stages").
- **The live clock.** A replay makes the clock jumps its recording made (see "Risks" below) and
  none of its own, so these runs never exercised the one host-timing path that reaches the guest.
- **Two different PCs.** The floating-point analysis below suggests two x86-64 builds will
  agree, but it hasn't been tested.

## What must match at the start

Nothing can be synced once a match is under way: there are no save states, so both instances must
boot together and see the same world from the first instruction.

| What | Why it matters | How phase 2 gets it |
|---|---|---|
| Disc | Every DVD read lands in RAM | SHA-1 of the image (`065341b8575adea00fd0a75e389f135d32840423` for GF7E01 rev 0), compared in the handshake. Cache it: hashing 1.4 GB takes seconds |
| Build | The recompiled code is the game | A build ID compiled in by CMake: this repo's commit, the gcn-recomp commit, `recomp/*.txt` (patches, HLE, idle) and `main.dol`'s SHA-1. Each player builds from their own disc, so the exe itself won't be byte-identical across compilers. The ID should also carry the compiler and its version: refuse a different ID, and warn on a different compiler |
| Memory card | It decides what versus has unlocked (stages, characters, modes), and the game loads its save from it | See "Unlock state" below |
| RTC | The calendar the game reads; it may seed randomness | The host's start time in the handshake, given to `rtc_set_base()` before boot, as a replay does with its log's `# epoch` |
| Pads plugged in | The game reads which ports have a controller | Set by the session (one channel per player), not by each PC's USB devices. A replay already ignores live pads completely |
| Guest-affecting settings | Settings flagged `AffectsGuest` and the `GCN_*` variables that change the guest: `GCN_VI_RATE`, `GCN_CPU_SCALE`, `GCN_CATCHUP`, `GCN_CLOCK=host` (never allowed), step scaling | The host's values in the handshake, applied on both sides. This game has no `AffectsGuest` settings yet. Its 120 fps work (#12) would add some |
| System fonts | `OSInitFont` reads them into RAM | They come from the fork's `fonts/`, so the same build ID covers them. A missing font dir (a run from another working directory) changes RAM, so the handshake should include their hash |
| SRAM | Language, sound mode | A constant in the runtime (`hw/exi.cpp`), so it's the same for everyone |

### Unlock state

The issue suggests agreeing on unlock flags rather than sending card files. Two ways:

1. **Use the host's card.** The host sends a digest of its card, and the joiner uses the
   host's bytes on a scratch copy (it's 512 KB, sent once). This is the least work, and it's
   exact, but it does send a save file.
2. **A shared fresh card.** Each player starts with a blank scratch card, and a game hook
   (in this repo's `src/`) sets the versus unlock flags in RAM at a fixed frame. The flags
   are either "everything" or a list the host chooses. This keeps saves private, but it needs
   the unlock flags' addresses found first. That work overlaps with the "unlock everything"
   cheat on the roadmap.

Option 2 fits the issue best. Option 1 is a fallback until the flags are known. Either way, the
card is a scratch copy during a match. Saves the game makes then are thrown away, so a player's
real card is never touched.

## Risks for later phases

1. **The clock's catch-up jumps (the main one).** In a live run, when the host falls more than
   4 ms behind real time, virtual time jumps to catch up (`clock_pace` in the fork's
   `cpu.cpp`). How often that happens depends on the host's speed, so two PCs would jump at
   different moments and part within a frame. Waiting for a late remote input would cause
   jumps too, since the guest stands still in real time while it waits. Phase 2 must turn
   catch-up off in netplay, and re-anchor the pacer after each wait (as the menu's pause does)
   rather than catching up. Without catch-up, a slow PC slows the match down for everyone,
   which is what Dolphin does. Later the jumps could be shared: each input packet would carry
   the sender's lag, and both sides would make the larger jump at the same SI tick.
2. **Polls are per register read.** The SI reads the live pad on every read of its registers:
   about 9 a frame here (458,513 polls over `fortuna`'s 50,322 frames). Lockstep can't wait on
   each of those. Netplay should latch the pads once per tick of the SI's 120 Hz timer
   (`si_poll_tick`, a virtual-time event) and answer every read until the next tick from that
   latch. The tick count is then the input frame the peers exchange.
3. **Floating point across PCs.** The generated code is built with `-ffp-contract=off`, no
   `-march` and no fast-math. `frsqrte` and `fres` are exact `1.0 / sqrt(x)` and `1.0 / x`, not
   host estimate instructions, which differ between Intel and AMD. The game HLEs no maths (only
   context switching, `OSReport`, `OSPanic` and `OSResetSystem`). So each operation is plain
   IEEE and should round the same on any x86-64 PC, whichever clang built it. Two exceptions:
   - The step scaling for higher frame rates (`step.cpp`) uses the host's `pow`, which can
     differ between C libraries. It only matters once that scaling is on.
   - NaN payloads differ between x86-64 and ARM64. The boss bug (#42) showed this game's maths
     can reach infinities and NaNs. Cross-architecture play (a Mac on Apple silicon against a PC) should stay off
     until it's tested.
4. **Pausing.** The menu's pause is invisible to the guest, but in lockstep a paused player
   stalls the other. Netplay should send the pause to both and show who paused.
5. **Two instances on one PC (phase 2's setup).** Every instance sees every controller and the
   keyboard. Each needs to be told which local device is its player's.

## Phase 2 design: two instances on one PC over localhost

The generic parts go in our gcn-recomp fork, as a new `runtime/netplay.cpp`. Game-specific setup
(the unlock state) goes in this repo.

**Starting a session.** `--netplay-host=<port> [--netplay-delay=<ticks>]` on one instance and
`--netplay-join=127.0.0.1:<port>` on the other. Each also gets `--netplay-pad=<n>`, the local
controller to use. The host's player is P1 and the joiner is P2. Both connect before the guest
boots. The handshake (the host sends, the joiner checks and confirms) carries:

- protocol version, build ID, disc SHA-1 and font hash (refuse on any mismatch, with a message
  naming which one);
- the start time, the guest-affecting settings, the input delay and the channel each player has;
- the card digest, or the card itself (see "Unlock state").

Then both boot. No guest instruction runs before the handshake finishes.

**Input exchange, keyed by SI tick.** The 120 Hz SI timer is the input clock. At tick `T`
(on the guest thread, at the same instruction on both sides):

1. Read this PC's pad and send it, tagged `T + delay`.
2. Wait until every player's input for `T` has arrived. For the first `delay` ticks, all
   players have a neutral pad (connected, nothing pressed).
3. Latch the inputs for all four channels. Every SI read until the next tick returns the
   latched state. `input_pad_poll` already does this for a replay, so the netplay inputs can
   enter there.

The wait blocks the guest thread in real time, and virtual time doesn't move. When it ends, the
pacer is re-anchored rather than caught up. Catch-up jumps are off for the whole session.

**Delay.** Each tick is 8.3 ms. On localhost 1 tick is plenty. Over the internet, 2 to 4 ticks
(17 to 33 ms) covers a round trip of up to about 60 ms. Phase 3 can pick the delay from the
measured round trip.

**Transport.** UDP, one packet per tick, each carrying:

- the sender's inputs for every tick not yet acknowledged (so a lost packet is covered by the
  next one);
- an acknowledgement of the other side's last tick;
- the sender's latest state hash.

Localhost never loses packets. Phase 2 should still test loss and lag with switches that drop or
delay packets (`GCN_NETPLAY_DROP=<percent>`, `GCN_NETPLAY_LAG=<ms>`), because phase 3 will meet both.

**Desync detection.** Every 60 frames (once a second), each side computes `state_hash_compute()`
(2 to 3 ms) and sends `(frame, hash)`. When the two hashes for a frame differ, both stop with
`desync at frame N`. Each then keeps its state-hash log, a RAM dump of that frame and its input
log.

**Recording.** Each side writes a normal input log with all four channels. When the two are in
sync, the logs are identical. Either log replays the match exactly on one PC, so a desync can be
reproduced, bisected with `tools\determinism.ps1` and fixed offline.

**What phase 2 changes in the fork:**

- `hw/si.cpp`: latch per tick in netplay.
- `input_log.cpp`: netplay inputs enter where replay inputs do.
- `cpu.cpp` (`clock_pace`): no catch-up, and re-anchor after a wait.
- `main.cpp`: the switches and the handshake before boot.
- `state_hash.cpp`: already there.

**Testing phase 2.**

1. Two windows on one PC, each with its own controller, both recording. Play a versus match,
   then check the two input logs are identical, and that replaying each gives matching hash logs.
2. Run one instance at `--scale=4` and the other at `--scale=1`, so they run at different speeds
   and the wait path gets used.
3. Run with `GCN_NETPLAY_LAG` and `GCN_NETPLAY_DROP`, and confirm the hashes still match.

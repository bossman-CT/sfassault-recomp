# recomp/

The per-game tables gcn-recomp reads when it translates the executable. Each is a plain
text list of addresses; see gcn-recomp's README for the exact format.

| File | What it lists |
|---|---|
| `hle.txt` | Functions the runtime replaces: at least `OSSaveContext`, `OSLoadContext`, `OSReport`, `OSPanic` and `OSResetSystem` |
| `special_calls.txt` | The address of `OSSaveContext`, whose call sites become resume points |
| `idle.txt` | The backward branch of the scheduler's idle loop, where the runtime skips ahead to the next event |
| `names.txt` | Functions named by hand |
| `patches.txt` | Instruction patches that fix game-specific problems, each with a comment saying why |
| `steps.txt` | The game's per-frame steps, needed to run above its native frame rate |

All are filled in for `GF7E01`. For 120 fps (issues #12 and #59, docs/dev/frame-rate.md),
`patches.txt` routes the game's stores to its frames-per-second variable through
`src/frame_rate.cpp`, and makes its retrace callback count a faster retrace as part of a
field. `steps.txt` lists no steps yet, only sites known not to scale.

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

None of these exist yet. They come out of milestone 0 in
[docs/status.md](../docs/status.md), except `steps.txt`, which belongs to the
high-frame-rate work.

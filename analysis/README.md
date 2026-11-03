# analysis/

The layout of the game's executable (`main.dol`), in
[decomp-toolkit](https://github.com/encounter/decomp-toolkit)'s format:

- `config.yml`: dtk's project config for `GF7E01`
- `symbols.txt`: every function and data object, by address. The recompiler reads this.
- `splits.txt`: which addresses belong to which original source file, where known

These files hold addresses, sizes and names only. They contain **no game code or data**,
and must stay that way: the disassembly dtk writes (`out/`, `all.s`) is ignored by git.

Nothing is here yet. Producing these files is milestone 0 in
[docs/status.md](../docs/status.md).

# analysis/

The layout of the game's code, in
[decomp-toolkit](https://github.com/encounter/decomp-toolkit)'s format, for USA `GF7E01`
revision 0 (`main.dol` SHA-1 `ae7201e7acd5a29acc807d7668a856392fd37ff9`):

- `symbols.txt`: every function and data object in `main.dol`, by address. The recompiler
  reads this.
- `splits.txt`: which addresses belong to which original source file, where known.
- `rels/<module>.txt` and `rels/<module>.splits.txt`: the same for each of the 23
  relocatable modules (`m0.rel`, `md0.rel`, ...) the game links at run time.
- `config.yml`: dtk's project config covering `main.dol` and every module, for
  regenerating or refining the symbols. The build doesn't read it.

SDK functions the symbols leave unnamed are named in [`recomp/names.txt`](../recomp/names.txt).

These files hold addresses, sizes and names only. They contain **no game code or data**,
and must stay that way: the disassembly dtk writes (`out/`, `all.s`) and the extracted
disc (`orig/`) are ignored by git.

## Sources

- `symbols.txt`, `splits.txt`: from [gabefletch/StarFoxAssault](https://github.com/gabefletch/StarFoxAssault)
  (`config/GF7E01`), CC0.
- `rels/`: from [xSNAIRZx/star-fox-assault-recomp](https://github.com/xSNAIRZx/star-fox-assault-recomp)
  (`decomp/config/GF7E01/<module>/`), MIT; its license is in [`rels/LICENSE`](rels/LICENSE).

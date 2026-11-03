# CLAUDE.md

A native PC port of Star Fox Assault (GameCube, USA `GF7E01`), built by recompiling the
game's executable with gcn-recomp (our fork, see below). Several Claude sessions may work on this repo at once,
each on its own branch and issue.

## Rules

- **Never commit game data.** No disc images, nothing extracted from the disc (`main.dol`,
  `.rel`, assets), nothing from `build/`, and no disassembly or decompiled code pasted from
  tools. Addresses, sizes, names and descriptions are fine. Run
  `python tools/check_no_game_data.py` before committing.
- The user's disc image lives in `rom/` (ignored by git). Read it; never modify, move or
  upload it.
- `gcn-recomp/` is our fork, bossman-CT/gcn-recomp, of ethannicholas/gcn-recomp. Change it
  only for fixes that aren't specific to this game, following its own `CLAUDE.md` (nothing
  game-specific, commit on its `main`), then push the fork and update the submodule pin
  here. Each fork commit should be offered upstream as a PR, but only with the user's OK.
  Game-specific fixes go in this repo's `recomp/` or `src/`.
- Work on a branch named after the issue (for example `m0/dol-hash`) and open a pull
  request to `main` that links the issue. Don't push to `main`.
- When a roadmap item is done, tick it in `docs/status.md` in the same pull request.

## Layout

- `CMakeLists.txt`: calls gcn-recomp's `gcn_add_game()`
- `analysis/`: dtk config and symbols for `main.dol`
- `recomp/`: per-game recompiler tables (see `recomp/README.md`)
- `src/`: game-specific runtime code, once there is any
- `docs/status.md`: the roadmap; GitHub milestones and issues mirror it
- `tools/`: project scripts

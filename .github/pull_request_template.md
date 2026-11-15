## What and why

<!-- What this changes and why. Link the issue: "Fixes #12". -->

## How it was tested

<!-- Which mission, menu or mode; OS and GPU. "Not tested in game" is OK for docs and tooling. -->

### `tools\regress.ps1` result

<!--
GitHub's check can't build or run the game (that needs a disc), so this is the only test
a reviewer sees. Build main and run `.\tools\regress.ps1 -Update`, then build this branch
and run `.\tools\regress.ps1`, and paste the table it prints. Explain every CHANGED or
STOPPED route. Don't attach the frames or side-by-sides: they're images of the game.
See CONTRIBUTING.md, "Testing".
-->

```text

```

## Checklist

- [ ] No game data: no disc images, extracted files, generated code, pasted disassembly, or anything from `build/` or `saves/`
- [ ] The no-game-data hook is on (`git config core.hooksPath .githooks`)
- [ ] One issue per branch, and the issue is linked above
- [ ] Every new entry in `recomp/patches.txt` has a comment saying what it fixes
- [ ] Nothing specific to this game in `gcn-recomp/`
- [ ] `docs/status.md` updated if this finishes a roadmap item

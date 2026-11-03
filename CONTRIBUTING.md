# Contributing

Thanks for helping. This file covers the rules, how to pick up work, and what a good
pull request looks like.

## The one rule that can't bend: no game data

This project is legal to host because it contains only original code and addresses.
**Never** put any of the following in a commit, issue, pull request, comment or
attachment:

- Disc images, or links to download them
- Files extracted from the disc: `main.dol`, `.rel` files, textures, models, audio, movies
- Generated code: anything from `build/`, including recompiled C
- Disassembly or decompiled code copied out of Ghidra, IDA or dtk
- Code from leaked Nintendo SDKs or other leaked material

Addresses, sizes, function names and short descriptions of what the game does are fine.
Screenshots and short clips of the running game to show a bug are fine.

CI checks every pull request for disc images and large binary files, but it can't catch
everything, so reviewers will reject anything that looks like game data.

## Picking up work

1. Find an issue. [`good first issue`](../../issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)
   and [`help wanted`](../../issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22)
   are good starting points. [docs/status.md](docs/status.md) shows how they fit together.
2. Comment on the issue to say you're taking it, so two people don't do the same work.
3. If there's no issue for what you want to do, open one first for anything bigger than
   a small fix.

## Making a pull request

1. Fork the repository and create a branch from `main`.
2. Keep each pull request to one change. Small pull requests get reviewed faster.
3. Explain **what** changed and **why**, and link the issue (`Fixes #12`).
4. Say how you tested it: which mission or menu, on which OS and GPU.
5. Every patch in `recomp/patches.txt` needs a comment saying what it fixes.
6. Update [docs/status.md](docs/status.md) when you finish a roadmap item.

## Project-specific or upstream?

Anything that isn't specific to Star Fox Assault, such as a GX rendering bug, an audio
problem, or a missing hardware feature, belongs in
[gcn-recomp](https://github.com/ethannicholas/gcn-recomp), which several other games share.
Open the issue or pull request there and link it from ours. Fixes that only make sense
for this game go here, in `recomp/` or `src/`.

## Reporting bugs

Use the **Bug report** template. Include your OS, GPU, the commit you built, the mission
or menu, and steps to reproduce it. Logs and screenshots help a lot.

## AI-assisted contributions

Using AI tools is fine. You're still responsible for the change: understand it, test it
in the game, and be ready to answer questions about it in review.

## License

By contributing, you agree that your contribution is released under the project's
[MIT license](LICENSE).

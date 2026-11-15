# Widescreen

`--aspect=16:9` shows the game at 16:9 (issue #11). It is off by default. Any wider aspect works
the same way, for example `--aspect=21:9`. Setting `GCN_ASPECT=16:9` in the environment does the same.

```powershell
.\build\sfassault.exe --aspect=16:9 --scale=3
```

The widening itself is generic and lives in gcn-recomp (its `docs/graphics.md`, "Widescreen"). It
narrows every draw's x after the viewport and stretches the frame when it is shown, which keeps
the EFB, its copies and the game's own screen-space maths as they were. This file records what
is specific to this game.

## What is shown wide

| Screen | Shown | Why |
|---|---|---|
| Logos, title screen, menus, the save prompt | 4:3 | 2D only, no 3D scene across the frame |
| Movies (the opening, mission intros) | 4:3 | drawn as 2D pictures |
| Mission briefing | 16:9 | the briefing's 3D window spans the frame. The window's frame and borders span it too, and are stretched with it (see below) |
| Missions, including the pause menu over them | 16:9 | |

A game that switches between a 2D screen and a 3D one switches between 4:3 and 16:9 with it.

## HUD placement

`src/widescreen.cpp` sets the anchor split to 0.5 (`gx::render_set_wide_anchor`). A 2D draw
lying wholly in the outer half of one side of the 4:3 frame moves out with that side. Everything
else stays centred at its own proportions. In missions:

- the bomb and shield gauge (bottom left) and the hit counter (top right) go to the wide
  frame's corners;
- the comm box crosses the centre's half, so it stays centred, and the portrait inside it stays
  with it;
- labels over allies and lock-on brackets stay on their targets. The game places them with its
  4:3 projection, and the scene moved by the same map. By the same rule, though, a bracket
  lying wholly in the outer half of a side moves out with the HUD, an eighth of the 4:3
  frame's width off its target at 16:9. None of the frames checked showed it, but it follows
  from the rule.

`GCN_WIDE_ANCHOR` overrides the split, for trying others (1 keeps everything centred).

## Culling

The game does not cull mission 1 to its 4:3 view, so nothing pops in at the new sides. With
`GCN_WIDE_LOG=20` (how many scene draws lie wholly past the 4:3 frame's sides, and how far out
the farthest starts), a replay of the `fortuna` route showed:

| Stretch | Samples | With draws wholly past the 4:3 sides | Farthest one past the 16:9 sides too |
|---|---|---|---|
| Space battle | 1419 | 1016 | 399 |
| Fortuna base (frames 32800 to 34400) | 85 | 77 | 24 |

A replay of the base at 2.4:1, much wider than 16:9, shows the level and its objects out to
the edges, with nothing cut at a line. The on-foot and Landmaster missions are not checked yet.
If one of them culls tightly, the game's 3D projection parameters are the place to start:

- `fn_80180218` and `fn_801803D0` store the projection's aspect, vertical field of view,
  near and far planes in four globals at `0x803868F8` to `0x80386904` (sbss);
- `fn_8016E80C` reads them and calls `C_MTXPerspective` (`fn_8013AFB8`, unnamed in the
  symbols), the only caller of it in `main.dol`;
- `fn_800B3B38` saves all four, apparently to draw with another projection and put them back.

## Known limits

- At `--scale=1` the scene's pixels are stretched across by 4/3. `--scale=2` or more covers it.
- The briefing's frame spans the whole picture, so it is stretched with the wider picture, not
  kept at its proportions.
- The lock-on bracket case above.

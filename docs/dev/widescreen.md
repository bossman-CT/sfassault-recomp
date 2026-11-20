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
  with it. In the cutscenes' wider comm box the portrait lies wholly in the left outer half,
  but it lies on the box, so it stays with it too (see the pause menu, next);
- the pause menu stays whole. In mission 2, on foot, the game draws its panel at the left
  of the 4:3 frame (x 16 to 184), with the map to its right; in mission 1's Arwing stages
  it is in the middle, with no map. The panel crosses the split, so it stays centred. The
  timer's digits, the CONTINUE / QUIT highlight and four of the five medal flags lie wholly
  in the left outer half, though, and the rule by position moved them out to the wide
  frame's left edge, off the panel (issue #55). gcn-recomp keeps a draw lying on a centred
  2D panel drawn before it centred with the panel, so they stay in their boxes. A panel is
  a 2D shape lying wholly in the frame. In the space battle an untextured 2D draw (the
  orange lines, it seems) spans most of the frame but reaches past its left edge, so it is
  not one, and the gauge and the counter under it still move out;
- the markers the game places over a point of the scene stay on it: the aiming reticle (a circle
  and brackets), the lock-on brackets, the wingmen's name labels, the bonus popups. The game
  places them with its 4:3 projection and the scene moved by the same map. The rule by position
  alone would have anchored them as they crossed the split, though: the reticle jumped an
  eighth of the frame outward there, its circle and its brackets crossing at different
  frames and tearing apart for a few, and a label left its ship (issue #79).
  `src/widescreen.cpp` lists their textures (`render_set_wide_centred_textures`, by the content
  hashes the draw log's `t0=` shows), and their draws are never anchored. The list is the
  textures that were anchored on one side and not on the other during replays of a space-battle
  route and a long playthrough (about 170,000 frames, several missions), found by tallying each texture's class per draw; a marker added by a
  later mission that jumps is a texture to add (log with `GCN_DRAWLOG`);
- the reticle stays inside the 4:3 frame's width, so it does not reach the wide frame's
  edges: the game keeps it there itself (its centre stayed between about 24 and 616 of 640 across
  in 25,000 frames of the space battle), and what it aims at is what is under it, so showing it farther out would
  put it off its target. Aim feeds targeting, so this was not changed.

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
- A marker whose texture is not in the list (see the markers above) is still anchored when it
  crosses the split.

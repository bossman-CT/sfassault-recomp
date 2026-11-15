# Quality of life (issue #41)

Options in Game > Quality of life, all off by default. Each is a game-specific patch in
`recomp/patches.txt` that asks a function in `src/` what to do, behind a setting the guest
reads (`settings::AffectsGuest`): latched at a frame boundary and logged in the input log,
so a replay does what its recording did, and a recording made with an option off replays
exactly as before.

## Skip the intro

`game.skip_intro` (`src/skip_intro.cpp`): Off, Logos, or Logos and movie.

### How the game boots

The game's top level is a flow object (its pointer at 0x80385B98, `sda-30088`), whose state
at +0x34 the function at 8000ECC8 steps every frame. The states seen from power-on to a
mission, in order (jump table at 8021D0DC):

| State | What it does |
|---|---|
| 35 | Starts the logo task (update 800110F0), goes to 36 |
| 36 | Waits for the logo task to end (the flow's child count at +8), goes to 3 |
| 3 | Pops the logo task's resource level (fn_80016FA8), starts movie 0 (fn_800C2E18), goes to 4 |
| 4 | Waits for the movie to end, starts loading the title, goes to 5 |
| 5 | Waits for the load, starts the title, goes to 6 |
| 6 | The title and its menus (m2.rel); goes to 7 for a mission, or back to 3 for the demo |
| 0 | Back to the title from a game: like 3 without the movie |

The logo task's own state is at +0x34 too. State 0 loads `ns_logos.nut` (on a resource
level of its own) and state 1 waits for it and a short timer; 2 to 5 are the health warning
("Press any button"), 6 to 8 the Nintendo logo, 9 to 11 namco, 12 to 14 Sofdec. State 15
removes the task.

Movie 0 is `loop_demo`, the opening movie, which is also the title's demo when it is left
alone. The movie table at 8025A4A8 (20 bytes an entry, name first) lists it and nine story
movies.

### The patches

- 800111C0, the logo task's step out of state 1: to 15 with the option on. The logo file is
  loaded and freed as before, so the flow's state 3 pops the same resource level.
- 8000F19C, the call in the flow's state 3 that starts the movie: left out the first time
  after boot with "Logos and movie". State 4 then finds no movie and loads the title, as
  state 0 does. A flag set by the first patch says which start is the first after boot, so
  the title's demo still plays.

On the `title-short` route with "Logos and movie" the title shows at about frame 300; the
original reaches it at about 720 when every logo and the movie are skipped with buttons,
and plays the whole movie (over 2600 frames) when nothing is pressed.

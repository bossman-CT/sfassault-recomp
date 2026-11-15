# Cutscenes (issue #41)

Game > Quality of life > Skip cutscenes at once (`game.skip_cutscenes`, `src/cutscenes.cpp`),
off by default. A patch in `recomp/patches.txt` asks that file what to do, behind a setting
the guest reads (`settings::AffectsGuest`): latched at a frame boundary and logged in the
input log, so a replay does what its recording did.

## The game's cutscenes

Start ends every cutscene seen so far, but the game ignores a press it isn't ready for, so
some have to be watched for a while however often Start is pressed.

**Movies** (Sofdec). The movie task (update 800C3258, made by fn_800C2E18) plays an entry of
the table at 8025A4A8, 20 bytes an entry, its name first: `loop_demo` (the opening movie and
the title's demo) and nine story movies. In mission 1, Fortuna's intro and the Arwings'
launch after the briefing are movies. The task's state is at +84: it tests Start (the pad's
pressed bits at 0x8029A8C0) only once the movie plays, about a second after it starts, and
ends the movie (a fade, +128 = 1) only if it has played 30 frames (+100, which counts
unpaused frames) and isn't paused (+106).

**Scenes the game plays itself.** The scene task (update 800AB4D0, made by fn_800A9E18) plays
one of the 36 entries of the table at 8024923C (12 bytes: the scene's id first). In mission 1
the end of the space battle is scene 1 and the descent to the planet scene 5. Each frame of
play it ends if it has finished (+119) or Start is pressed (800ABE38), but only if the flow
object's +128 is 0 (fn_8001197C) and a camera timing test passes. The flow sets +128 while it
loads the mission's next part: in the `fortuna` route the descent starts at frame 16449,
the load runs to 16588, and the scene to 17053. A Start pressed before 16588 is ignored.

## The option

A Start press is kept for as long as the same cutscene runs, so it ends as soon as the game
lets it, and a movie needs to have played 1 frame, not 30. The game's own tests still decide
when, so nothing ends before what follows has loaded.

- 800C327C: the movie task's first instruction, every frame: notes a press.
- 800C3374: where it tests Start: a kept press counts.
- 800C3398: the 30 frames it must have played, 1 with the option.
- 800ABE38: where a scene tests Start: a press is kept, and counts.

A cutscene reports itself every frame it runs, so a gap of a frame means it has ended and a
kept press is dropped, whatever task comes at that address next.

Measured on copies of the `fortuna` route with a Start press added:

| Press | Original | With the option |
|---|---|---|
| Frame 2190, Fortuna's intro movie still starting | ignored; the movie plays to 2491 | ends at 2240 |
| Frame 16470, the descent loading the next part | ignored; the scene plays to 17053 | ends at 16589 |

Only mission 1 has been looked at. Other missions may have cutscenes of other kinds.

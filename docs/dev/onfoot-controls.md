# On-foot controls and the player's mode

Issue #48 (a modern on-foot scheme) and #40 (per-vehicle settings) both need to know how
the game controls a pilot on foot and what each player is doing. This is what the game does,
measured on USA `GF7E01`, and what the port adds on top.

## The game's three controller settings

Each player profile picks one (Options > User Settings > a player > Controller Setting).
**Guest**, which versus and a fresh file use, is Single Stick. Each profile also has
**Invert Y Axis** for the Arwing, the Landmaster and the pilot (all YES by default: up looks
down), and Rumble. The settings are saved with the profile on the memory card.

On foot (the Pilot tab of Controller Details):

| Control | Single Stick | Dual Stick | R Button Run |
|---|---|---|---|
| Control stick | move and turn | move and strafe, relative to the view | aim (turn and pitch) |
| C-stick | switch weapons | aim (turn and pitch) | switch weapons |
| R | aim, with the stick | shoot | run forward |
| A | shoot | switch weapons | shoot |
| L | fix alignment; L + stick rolls | reset alignment; roll | fix alignment; L + stick rolls |
| B / X / Y / Z | barrier / scope on-off / jump / enter a vehicle | same | same |
| D-pad | switch maps | same | same |

The Landmaster aims the same way (R + stick in Single Stick, the C-stick in Dual Stick, the stick in
R Button Run, where R drives forward); the Arwing flies with the stick in all three.

Dual Stick is the twin-stick layout of current shooters. The other two keep move and aim on
one stick.

## How aiming responds

Measured in versus on Katina Outpost, at the game's 60 frames a second, by holding the
C-stick at fixed deflections (`GCN_INPUT`'s `CR20` and so on, in units of the pad's 0 to 100)
and reading the view's yaw matrix and the aim pitch each frame (`GCN_WATCH`):

- **Turning and pitch are rates.** Turn rate against C-stick deflection in Dual Stick:

  | Deflection | 10 | 20 | 30 | 40 | 50 | 60 | 70 | 80 to 100 |
  |---|---|---|---|---|---|---|---|---|
  | Degrees a frame | 0 | 0.72 | 1.19 | 1.70 | 2.17 | 2.65 | 3.17 | 3.36 |

  Linear at about 0.048 degrees a frame per unit of deflection, with nothing below about 10,
  and capped at 3.36 degrees a frame (about 200 degrees a second) from about 75. The pitch
  follows the same curve. Single Stick turns at the same 3.36 at full deflection.
- **No acceleration, little lag.** The rate follows the stick through a smoothing of about
  45% a frame, so it is at full rate within about five frames and stops as quickly.
- **Pitch is clamped at ±67.5 degrees.**
- Below the turning threshold the C-stick moves the camera a little without turning the
  pilot.

What feels clunky, then, is a linear response with a low ceiling: fine aim and fast turns
share one straight line that tops out at 200 degrees a second.

## Where the game keeps it

main.dol addresses (so the same in every scene):

| Address | What |
|---|---|
| `0x802A2B60` + 0x48 × player | +0: the player's pilot object (`PILOT(FOX)` and so on). The same object while the pilot sits in a vehicle |
| `0x802A6620` + 0xA0 × player | +0: the object the player's camera follows (the pilot or vehicle in versus, a camera object of its own in a story mission); +0x10: its yaw as a 3×4 matrix (yaw = atan2(m[0], m[2])) |
| `0x80386B04`, `0x8038593C` | the game is paused: the first set, the second clear |

A game object has its name (a string, or `NULL`) at +0x1C and its update function at +0x20;
the top byte of +4 says what kind of object it is (5 a pilot, 6 an Arwing). A pilot's update
is `0x8007871C`. A pilot's +0x100 is the vehicle it rides (0 on foot), the byte at +0x136 the profile's
controller setting (0 Single Stick, 1 R Button Run, 2 Dual Stick) and its +0x370 the aim pitch, as a sine (positive up).

**The player's mode** (`src/game_mode.h`): on foot when the pilot rides nothing; in a vehicle,
an Arwing for kind 6 (both the all-range and the on-rails Arwing of mission 1) and taken to be
a Landmaster for kind 7. A parked object of kind 7 sits in versus on Katina Outpost beside one
of kind 6, but no Landmaster has been seen with a player in it yet. While the game is paused
(`0x80386B04` set, `0x8038593C` clear, the same in story and versus) there is no mode, nor
without a pilot (the title and menus).

The camera's table, `0x802A6620`, is no guide to the mode: in versus each entry is the
player's pilot or vehicle, but in a story mission on foot the camera follows a camera object
of its own.
`SFASSAULT_MODELOG=1` prints each player's mode as it changes.

## What the port adds

**Controls > Aiming** in the menu (F1): **Flip aim up/down on foot**, **in the Arwing** and
**in the Landmaster**, all off by default. Each flips the stick that aims in that mode on top of
the game's own Invert Y, which is per profile and reachable only from the title screen's
Options; these can change mid-mission (`src/controls.cpp`, through `input_ports::add_filter`,
read every update). On foot the stick follows the profile's controller setting, read from the
pilot (the byte at +0x136: 0 Single Stick, 1 R Button Run, 2 Dual Stick): the C-stick in Dual Stick, the
control stick in R Button Run, the control stick while R is held in Single Stick. The Arwing
flips the control stick. The page also says where the game's own settings are.

The input log records the pad after the flip, so a replay plays back what the game saw,
whatever the switches say now.

**Controls > Controller**: the face-button layout, the gyro and its sensitivity for every port,
the same settings as `controls.txt`'s. Each reads "From controls.txt" until changed in the menu,
and then wins over the file.

The game's modes (`src/game_mode.h`) are there for aiming settings that differ by mode, such as
the Modern on-foot scheme (#48).

**Controls > On foot > Aiming: Modern** (`src/onfoot_modern.cpp`, off by default), with the
game's Dual Stick controller setting, where the C-stick aims. With the others the game's own
aiming stays. It rebuilds the C-stick from:

- **the right stick**, read raw (`input_ports::raw`): a radial dead zone (0.08), then each axis's
  part of the push (5% each, so pushing straight up doesn't drift sideways) through a response
  curve (exponent 2) from the game's slowest turn up to a speed for each axis (0.75 and 0.6 of the
  top speed). Holding the stick at the edge for 0.12 s raises that to the top speed over 0.25 s
  (acceleration);
- **the gyro** (on foot only, from the port's own gyro dead zone, smoothing and hold button) and
  **the mouse** (port 1, captured while the menu is closed; left button R, right button X,
  wheel A), by how far they move.

It works in the game's terms, a share of the top speed each game frame, so the same settings hold
in one-player and split-screen play and at any frame rate. The share maps onto the C-stick as
the game's response does: nothing below 13, 14 for its slowest turn, 75 for the top speed. What a
frame can't give is kept and given later: a mouse flick past the top speed a moment later, a
turn slower than the game's slowest as pushes of three frames at that speed.

Measured on the Dual Stick versus route, defaults, degrees a frame (Original in brackets):
right stick at 30%: 0.51 (1.19); at 60%: 1.04 (2.66); all the way: 2.5, then 3.35 after the
acceleration (3.36). Mouse, 10 counts an update: within 10% of 0.08 degrees a count, a little
over. Mouse slower than the game's slowest turn: about 80% of it, the game's smoothing taking
part of each push.

## Testing without the story

Every recording so far is mission 1, which is all Arwing. Versus starts on foot: two
scripted routes, made with `GCN_INPUT` from a fresh memory card, reach a two-player match on
Katina Outpost (Fox against Falco). They live under `saves/routes/`, which is not committed;
how they were made is in the pull request that added this page (#48).

// What each player is controlling right now: on foot, an Arwing, a Landmaster, or nothing
// (the title and menus, the pause screen, loading). Read from
// guest memory, so it is what the game itself is doing; see docs/dev/onfoot-controls.md for
// where it comes from.
//
// Any thread may ask; the answer is read afresh each time and is only a snapshot. It is
// meant for the input filter (controls.cpp), whose result the input log records, so a
// replay does not depend on when it was read.
#pragma once

namespace game_mode {

enum class Mode { None, OnFoot, Arwing, Landmaster };

// Player 0 to 3: the controller on port 1 to 4 in versus, player 0 in the story.
Mode of_player(int player);
const char* name(Mode m);

// The controller setting the player's profile picked (Options > User Settings), which says
// which stick aims on foot. Unknown with no player.
enum class Controls { Unknown, SingleStick, DualStick, RButtonRun };
Controls controls_of(int player);
const char* name(Controls c);

}  // namespace game_mode

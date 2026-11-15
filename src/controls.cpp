// Controls > Aiming (issues #40 and #48): flip Y on foot, in the Arwing, in the Landmaster.
//
// Invert Y for each vehicle is the game's own setting, kept with each player profile on the
// memory card and reachable only from the title screen's Options. These switches flip the
// aiming stick's up and down on top of it, one for each mode (game_mode.h), so the player can
// change it at any time, mid-mission or paused, and the game's own setting stays as it is.
// All are off by default: the game plays as the original.
//
// The stick that aims on foot depends on the profile's controller setting: the C-stick in Dual
// Stick, the control stick in R Button Run, and the control stick while R is held in Single
// Stick (the stick alone walks there). In the Arwing it is the control stick in all three.
// The Landmaster's controller details show the same sticks aiming as on foot.
//
// The flip is applied to the pad on its way to the game (input_ports::add_filter) and read
// afresh every update, so it takes effect at once. The input log records the flipped pad, so
// a replay plays back what the game saw whatever the switches say now.
//
// On foot with Dual Stick, Controls > On foot > Aiming = Modern (onfoot_modern.cpp) rebuilds the
// C-stick before the flip.
//
// SFASSAULT_MODELOG=1 prints each player's mode and controller setting as they change.
#include "game_mode.h"
#include "input_ports.h"
#include "menu.h"
#include "onfoot_modern.h"
#include "settings.h"

#include <cstdio>
#include <cstdlib>

namespace {

using game_mode::Controls;
using game_mode::Mode;

const char* const kPath = "Controls/Aiming";

auto& s_flip_onfoot =
    settings::toggle("controls.onfoot.flip_y", kPath, "Flip aim up/down on foot", false)
        .help("Flips up and down for aiming on foot, on top of the game's own Invert Y Axis for the pilot. With "
              "the game's default (YES: up looks down), turning this on makes up look up. Flips the C-stick in "
              "Dual Stick, the control stick in R Button Run, and the control stick while R is held in Single "
              "Stick.");
auto& s_flip_arwing =
    settings::toggle("controls.arwing.flip_y", kPath, "Flip up/down in the Arwing", false)
        .help("Flips the flight stick's up and down, on top of the game's own Invert Y Axis for the Arwing. With "
              "the game's default (YES), turning this on makes up climb.");
auto& s_flip_landmaster =
    settings::toggle("controls.landmaster.flip_y", kPath, "Flip aim up/down in the Landmaster", false)
        .help("Flips up and down for aiming in the Landmaster, on top of the game's own Invert Y Axis for it. "
              "The same sticks aim there as on foot: the C-stick in Dual Stick, the control stick in R Button Run, "
              "the control stick while R is held in Single Stick.");

const bool g_note = (menu::note(kPath,
                                "The game's own Invert Y is one setting per player: Options > User Settings > "
                                "your player > Controller Setting, then A (Invert Y Axis Arwing, LandMaster and "
                                "Pilot, YES unless changed; Guest keeps the defaults). The switches above flip "
                                "it from here, any time."),
                     menu::note(kPath,
                                "The same screen picks how you play on foot: Single Stick (move and turn with the "
                                "control stick, aim with R), Dual Stick (move with the control stick, aim with the "
                                "C-stick) or R Button Run (aim with the control stick, run with R)."),
                     true);

const bool g_modelog = getenv("SFASSAULT_MODELOG") != nullptr;
Mode g_last[4];
Controls g_last_controls[4];

uint8_t flip(uint8_t v) { return (uint8_t)(v ? 256 - v : 255); }  // about the centre, 0x80

// Flips whichever stick aims for `c` on foot (and, it is taken, in the Landmaster).
void flip_aim(PadState& p, Controls c) {
    switch (c) {
    case Controls::RButtonRun: p.stick_y = flip(p.stick_y); break;
    case Controls::SingleStick:
        if (p.buttons & PAD_R) p.stick_y = flip(p.stick_y);
        break;
    default: p.cstick_y = flip(p.cstick_y); break;  // Dual Stick, and an unreadable setting
    }
}

void filter(int port, PadState& p) {
    if (port < 0 || port > 3) return;
    const Mode m = game_mode::of_player(port);
    if (g_modelog) {
        const Controls c = game_mode::controls_of(port);
        if (m != g_last[port] || c != g_last_controls[port])
            fprintf(stderr, "[sfassault] player %d: %s, %s\n", port + 1, game_mode::name(m), game_mode::name(c));
        g_last[port] = m;
        g_last_controls[port] = c;
    }
    // Modern (onfoot_modern.cpp) rebuilds the C-stick first, up on the right stick being up on
    // the C-stick, so the flip below means the same with either scheme.
    const Controls c = m == Mode::OnFoot || m == Mode::Landmaster ? game_mode::controls_of(port) : Controls::Unknown;
    onfoot_modern::apply(port, p, m == Mode::OnFoot && c == Controls::DualStick);
    if (!p.connected) return;
    switch (m) {
    case Mode::OnFoot:
        if (s_flip_onfoot) flip_aim(p, c);
        break;
    case Mode::Landmaster:
        if (s_flip_landmaster) flip_aim(p, c);
        break;
    case Mode::Arwing:
        if (s_flip_arwing) p.stick_y = flip(p.stick_y);
        break;
    default: break;
    }
}

const bool g_added = (input_ports::add_filter(filter), true);

}  // namespace

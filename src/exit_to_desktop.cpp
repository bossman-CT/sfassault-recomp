// Exit to Desktop from the main menu (issue #57): Game > Quality of life, off by default.
//
// The game has no way out of its own: closing the window or the menu's Quit. With this on,
// while the title or the main menu after it is up (game_mode::front_end), a line at the
// bottom right says "Y: Exit to Desktop" (the button's name on the pad in use: Triangle on a
// PlayStation controller, S with no controller), and Y asks "Quit the game?" in the menu's window
// (menu::ask): the game paused and its pad at rest, Yes (A) ending the run as the menu's Quit
// does (menu::quit, which saves settings.txt and the input log), No (B) going back.
//
// Y is a button neither screen uses. Its press is taken out of the pad on those screens, so
// the game never sees it; that is done on the pad's way to the game (input_ports::add_filter),
// which the input log records, so a replay needs nothing more. The question is the
// frontend's alone and never reaches the game. Nothing changes with the option off.
#include "game_mode.h"
#include "input_ports.h"
#include "menu.h"
#include "settings.h"

#include <cstdlib>

namespace {

auto& s_exit = settings::toggle("game.exit_to_desktop", "Game/Quality of life", "Exit to Desktop from the main menu")
                   .help("On the title and the main menu, Y asks whether to close the game: the controller's top "
                         "face button (Triangle on a PlayStation controller), or S on the keyboard. The hint on "
                         "screen names the button of the controller in use. The game itself never sees that press.");

bool offered() { return s_exit.get() && game_mode::front_end() != game_mode::FrontEnd::Other; }

uint16_t g_held[4];  // per port: Y as it was at the last update
int g_active;        // the port that last had anything pressed or a stick moved

// The game's Y is the top face button in both of Controls > Controller > Face buttons' layouts,
// so the layout does not change which button it is, only the pad's make does: SDL reports the
// controller's own type (the real one's behind Steam Input, which SDL 2.30 looks through when
// Steam says what it is), and the top button of a PlayStation controller is Triangle. Xbox
// and Nintendo pads label it Y (SDL names a Nintendo pad's buttons by their labels). With no
// controller on the port, the keyboard's S.
const char* hint_text() {
    if (!offered()) return nullptr;
    input_ports::Raw r;
    if (!input_ports::raw(g_active, r) || !r.pad) return "S: Exit to Desktop";
    return r.playstation ? "Triangle: Exit to Desktop" : "Y: Exit to Desktop";
}

bool active(const PadState& p) {
    auto off = [](uint8_t v) { return abs((int)v - 0x80) > 0x30; };
    return p.buttons || off(p.stick_x) || off(p.stick_y) || off(p.cstick_x) || off(p.cstick_y);
}

void filter(int port, PadState& p) {
    if (port < 0 || port > 3) return;
    if (active(p) && p.connected) g_active = port;
    const bool down = p.buttons & PAD_Y;
    const bool pressed = down && !(g_held[port] & PAD_Y);
    g_held[port] = p.buttons;
    if (!down || !offered()) return;
    p.buttons &= (uint16_t)~PAD_Y;
    if (pressed) menu::ask("Quit the game?", menu::quit);
}

struct Init {
    Init() {
        input_ports::add_filter(filter);
        menu::hint(hint_text);
    }
} s_init;

}  // namespace

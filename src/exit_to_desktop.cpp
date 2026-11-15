// Exit to Desktop from the main menu (issue #57): Game > Quality of life, off by default.
//
// The game has no way out of its own: closing the window or the menu's Quit. With this on,
// while the title or the main menu after it is up (game_mode::front_end), a line at the
// bottom right says "Y: Exit to Desktop", and Y asks "Quit the game?" in the menu's window
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

namespace {

auto& s_exit = settings::toggle("game.exit_to_desktop", "Game/Quality of life", "Exit to Desktop from the main menu")
                   .help("On the title and the main menu, Y asks whether to close the game: the controller's top "
                         "face button (Triangle on a PlayStation controller), or S on the keyboard. The game "
                         "itself never sees that press.");

bool offered() { return s_exit.get() && game_mode::front_end() != game_mode::FrontEnd::Other; }

const char* hint_text() { return offered() ? "Y: Exit to Desktop" : nullptr; }

uint16_t g_held[4];  // per port: Y as it was at the last update

void filter(int port, PadState& p) {
    if (port < 0 || port > 3) return;
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

// See game_mode.h. The addresses are main.dol's (USA GF7E01, revision 0), found as
// docs/dev/onfoot-controls.md describes.
//
// The game keeps a table of four, one entry per player, at 0x802A2B60, 0x48 apart: +0 the
// player's pilot, the object that walks about on foot. It stays the same object while the
// pilot sits in a vehicle. A game object has its name (a string, "PILOT(FOX)" and so on, or
// "NULL") at +0x1C and its update function at +0x20; the top byte of +4 says what kind of
// object it is. A pilot's +0x100 is the vehicle it rides, 0 on foot, and the byte at +0x136
// the profile's controller setting: 0 Single Stick, 1 R Button Run, 2 Dual Stick (seen with
// each, and the same in the story as in versus).
//
// On foot: the pilot rides nothing. In a vehicle: kind 6 is the Arwing (all-range and on
// rails, seen in mission 1), kind 7 is taken to be the Landmaster (a parked one in versus on
// Katina Outpost has it). While the game is paused (0x80386B04 set, 0x8038593C clear; both
// agree in every scene looked at, story and versus) there is no mode, so the pause screen's
// cursor is never flipped. Without a pilot (the title, menus) there is none either.
#include "game_mode.h"
#include "mmu.h"

#include <cstdint>

namespace game_mode {
namespace {

constexpr uint32_t kPlayers = 0x802A2B60, kPlayerStride = 0x48;
constexpr uint32_t kPaused = 0x80386B04, kRunning = 0x8038593C;
constexpr uint32_t kPilotUpdate = 0x8007871C;
constexpr uint32_t kObjKind = 0x04, kObjUpdate = 0x20, kPilotVehicle = 0x100, kPilotControls = 0x134;
constexpr uint8_t kKindArwing = 6, kKindLandmaster = 7;

bool is_object(uint32_t a) { return a >= 0x80000000u && a < 0x81800000u && (a & 3) == 0; }

uint32_t peek(uint32_t a) {
    uint32_t v = 0;
    return vm_peek32(a, &v) ? v : 0;
}

}  // namespace

Mode of_player(int player) {
    if (player < 0 || player > 3) return Mode::None;
    const uint32_t pilot = peek(kPlayers + kPlayerStride * player);
    if (!is_object(pilot) || peek(pilot + kObjUpdate) != kPilotUpdate) return Mode::None;
    if (peek(kPaused) != 0 || peek(kRunning) == 0) return Mode::None;
    const uint32_t vehicle = peek(pilot + kPilotVehicle);
    if (!vehicle) return Mode::OnFoot;
    if (!is_object(vehicle)) return Mode::None;
    switch (peek(vehicle + kObjKind) >> 24) {
    case kKindArwing: return Mode::Arwing;
    case kKindLandmaster: return Mode::Landmaster;
    default: return Mode::None;
    }
}

Controls controls_of(int player) {
    if (player < 0 || player > 3) return Controls::Unknown;
    const uint32_t pilot = peek(kPlayers + kPlayerStride * player);
    if (!is_object(pilot) || peek(pilot + kObjUpdate) != kPilotUpdate) return Controls::Unknown;
    switch ((peek(pilot + kPilotControls) >> 8) & 0xFF) {  // the byte at +0x136
    case 0: return Controls::SingleStick;
    case 1: return Controls::RButtonRun;
    case 2: return Controls::DualStick;
    default: return Controls::Unknown;
    }
}

const char* name(Controls c) {
    switch (c) {
    case Controls::SingleStick: return "Single Stick";
    case Controls::DualStick: return "Dual Stick";
    case Controls::RButtonRun: return "R Button Run";
    default: return "unknown";
    }
}

// The title and its menus are m2.rel (module 6), whose menu task (update at 0x11B9C into the
// module's code) keeps the screen it shows in the byte at +0x50 -- 0x5B the title waiting
// for Start, 0x5D the main menu, 0x5A and 0x5C, 0x5E and 0x5F coming and going -- and a 1 at
// +0x88 while that screen takes input. Found by logging the task through the title, the
// main menu, player select and back (see docs/dev/exit-to-desktop.md). The task is found
// through the task tree (0x802B2900: first task at +0, next at +0x18, update at +0x20) and
// the SDK's module list (0x800030C8: id at +0, next at +4, section table at +0x10).
FrontEnd front_end() {
    constexpr uint32_t kModules = 0x800030C8, kTasks = 0x802B2900;
    constexpr uint32_t kMenusModule = 6, kMenuUpdate = 0x11B9C;
    uint32_t m = peek(kModules), code = 0;
    for (int n = 0; m && n < 64 && !code; n++, m = peek(m + 4))
        if (peek(m) == kMenusModule) code = peek(peek(m + 0x10) + 8) & ~1u;  // section 1
    if (!code) return FrontEnd::Other;
    uint32_t t = peek(kTasks);
    for (int n = 0; is_object(t) && n < 512; n++, t = peek(t + 0x18)) {
        if (peek(t + kObjUpdate) != code + kMenuUpdate) continue;
        if (peek(t + 0x88) != 1) return FrontEnd::Other;
        switch (peek(t + 0x50) >> 24) {
        case 0x5B: return FrontEnd::Title;
        case 0x5D: return FrontEnd::MainMenu;
        default: return FrontEnd::Other;
        }
    }
    return FrontEnd::Other;
}

const char* name(Mode m) {
    switch (m) {
    case Mode::OnFoot: return "on foot";
    case Mode::Arwing: return "Arwing";
    case Mode::Landmaster: return "Landmaster";
    default: return "none";
    }
}

}  // namespace game_mode

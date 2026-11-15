// See game_mode.h. The addresses are main.dol's (USA GF7E01, revision 0), found as
// docs/dev/onfoot-controls.md describes.
//
// The game keeps two tables of four, one entry per player:
// - 0x802A2B60, 0x48 apart: +0 the player's pilot, the object that walks about on foot. It
//   stays the same object while the pilot sits in a vehicle.
// - 0x802A6620, 0xA0 apart: +0 the object the player's camera follows, then its matrices.
// A game object has its name (a string, "PILOT(FOX)" and so on, or "NULL") at +0x1C and its
// update function at +0x20; the top byte of +4 says what kind of object it is. A pilot's
// +0x100 is the vehicle it rides, 0 on foot, and the byte at +0x136 the profile's controller
// setting: 0 Single Stick, 1 R Button Run, 2 Dual Stick (seen with each, and the same in
// mission 1 as in versus).
//
// On foot: the camera follows the pilot, whose update is the pilot's. In a vehicle: the
// camera follows the pilot's vehicle; kind 6 is the Arwing (all-range and on rails, seen in
// mission 1), kind 7 is taken to be the Landmaster (a parked one in versus on Katina Outpost
// has it). Anything else -- menus, the pause screen, a
// cutscene's camera -- is None.
#include "game_mode.h"
#include "mmu.h"

#include <cstdint>

namespace game_mode {
namespace {

constexpr uint32_t kPlayers = 0x802A2B60, kPlayerStride = 0x48;
constexpr uint32_t kViews = 0x802A6620, kViewStride = 0xA0;
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
    const uint32_t view = peek(kViews + kViewStride * player);
    if (!is_object(pilot) || !is_object(view)) return Mode::None;
    if (view == pilot) return peek(pilot + kObjUpdate) == kPilotUpdate ? Mode::OnFoot : Mode::None;
    if (view != peek(pilot + kPilotVehicle)) return Mode::None;
    switch (peek(view + kObjKind) >> 24) {
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

const char* name(Mode m) {
    switch (m) {
    case Mode::OnFoot: return "on foot";
    case Mode::Arwing: return "Arwing";
    case Mode::Landmaster: return "Landmaster";
    default: return "none";
    }
}

}  // namespace game_mode

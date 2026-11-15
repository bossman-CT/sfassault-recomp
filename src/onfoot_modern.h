// The Modern on-foot scheme (issue #48): see onfoot_modern.cpp.
#pragma once
#include "hw/pad.h"

namespace onfoot_modern {

// Whether Controls > On foot > Aiming is Modern.
bool enabled();
// Port `port`'s pad: with Modern on and `active` (its player on foot, with the Dual Stick
// controller setting), the C-stick rebuilt from the controller's right stick, its gyro and,
// on port 1, the mouse, up on them being up on the C-stick. Otherwise it lets go of the gyro
// and the mouse and forgets what was pending.
void apply(int port, PadState& p, bool active);

}  // namespace onfoot_modern

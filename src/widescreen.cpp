// Widescreen settings for Star Fox Assault's HUD (issue #11). gcn-recomp does the widening
// itself (--aspect=16:9, render_set_aspect); this only says where the HUD goes.
//
// The in-mission HUD sits in the 4:3 frame's corners: the bomb and shield gauge at the
// bottom left, the hit counter at the top right. Both lie wholly in the outer quarter of
// their side of the frame. The comm box and the portrait in it cross the centre's half,
// so they stay centred under the bottom edge. The pause menu's panel, at the left of the
// frame on foot, crosses the half too, and the timer, highlight and flags on it go with it
// (gcn-recomp keeps a draw lying on a centred panel centred).
// With the split at 0.5, the gauge and the counter keep their distance from the wide
// frame's corners, and everything else stays centred, with its proportions.
// See docs/dev/widescreen.md.
#include "gx/render_gl.h"

namespace {
struct WidescreenHud {
    WidescreenHud() { gx::render_set_wide_anchor(0.5f); }
} s_widescreen_hud;
}  // namespace

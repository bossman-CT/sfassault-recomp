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
//
// The markers the game puts over a point of the scene move with that point, so they cross
// the split and, by position alone, would jump to the edge as they do, their pieces tearing
// apart where each crosses it (issue #79). The aiming reticle, its circle and brackets, is
// the one that shows it most: the game keeps it inside the 4:3 frame's width, and it must
// stay where it aims, so it is centred like the scene, not anchored. These textures' draws
// stay centred (render_set_wide_centred_textures). The list is the 2D textures that cross
// the split in replays of a space-battle route and a long playthrough (the draw log's `t0=` hashes): the reticle and lock-on
// brackets, the labels and the markers of allies and enemies, and the flashes and effects
// laid over the scene.
// See docs/dev/widescreen.md.
#include "gx/render_gl.h"

#include <cstdint>

namespace {
// Content hashes of the textures, as GCN_DRAWLOG prints them.
constexpr uint64_t kSceneMarkers[] = {
    0xfd76946037015a35, 0x0f173a78621357ac,  // reticle: the circle, the brackets
    0x3372c4b30e503d77, 0x8cf5a057de96756a,  // lock-on brackets
    0x6c716c1541fca279, 0x1624a4f7548fc57f, 0x4dcc7ec584c423b5, 0x7704753e8017f414,
    0x55e5ccc57641fc3d, 0xdbac22d3e15980c6, 0xe817f73a63a7d61e, 0xe843a5c5ad428d77,
    0xf48a16a5d87426ee, 0xfad3148c9905750f, 0x54fe824ccbb7a37c, 0xbf88ee9c80e9bb40,
    0x2e9c66f818445369, 0xcd9bbec80747f48d, 0x6432a6aacfad6180, 0x1960c099b160fc53,
    0x209ac4fb2f3eebed, 0x5d444f2a66b55532, 0xfcf23dabf96043ca, 0x49ecec18027be227,
    0xeb16cf3aa5e9b4d6, 0x9c73aa5e639f09ec,
};

struct WidescreenHud {
    WidescreenHud() {
        gx::render_set_wide_anchor(0.5f);
        gx::render_set_wide_centred_textures(kSceneMarkers, sizeof(kSceneMarkers) / sizeof(kSceneMarkers[0]));
    }
} s_widescreen_hud;
}  // namespace

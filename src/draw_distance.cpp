// Draw distance (issues #76, #28): Graphics > Rendering > Draw distance.
//
// The game draws a model only within a range of the camera, and fades it out over the last
// fifth of that range: full strength up to 0.8 of it, nothing at it and beyond. The range is
// a per-object factor (1 for the fighters) times a scene-wide distance the game works out
// every frame from the camera's field of view, the number of players and the mission
// (fn_80071054). In the mission 3 space battle that distance runs between about 100 and 300
// units while Star Wolf fights 200 to 400 units out, so their fighters are not drawn at all
// until they come close, and pop in when they do; everything else fades in late the same way.
// That is the original game's behaviour, not a fault of the port.
//
// The scene-wide distance and an offset added to it live in two variables (sda-29516,
// 0x80385DD4, and sda-29512, 0x80385DD8). Every store the game makes to them goes through
// sfassault_draw_range (recomp/patches.txt), which multiplies them by the setting. Only the
// draw pass reads them: each model's draw callback asks whether, and how strongly, to draw
// it. But drawing more is more work for the guest's CPU, and the game's timing follows its
// CPU time: a heavy scene that no longer fits its frame shifts what happens after it. So the
// setting is one the guest sees, latched at a frame boundary and logged in the input log,
// and a replay draws at the distance it was recorded with. Its default is Original, which
// is what every recording made before it existed was made with.
//
// See docs/dev/draw-distance.md.
#include "settings.h"

#include <cstdlib>

namespace {

// 1 is the original; each step multiplies the distance; the last step, Unlimited, puts it far
// beyond the farthest thing the game can draw (its far plane is 50,000 units), so no
// distance limit or fade is left.
constexpr float kUnlimitedStep = 9.0f;
constexpr double kUnlimitedFactor = 1000.0;

void relabel(float v);

auto& s_distance = settings::slider("graphics.draw_distance", "Graphics/Rendering", "Draw distance", 1.0f, 1.0f,
                                    kUnlimitedStep)
                       .step(1.0)
                       .format("Original")
                       .flags(settings::AffectsGuest)
                       .help("How far away the game draws ships, enemies and scenery, and where they fade in. "
                             "Original is the game's own distance, where Star Wolf's fighters and distant objects "
                             "appear late. 2x to 8x multiply it. Unlimited draws everything the level has loaded, "
                             "with no fade. Higher values cost more GPU time, and in the heaviest scenes the extra "
                             "work can run past a frame of the console's CPU time, which costs the game a frame "
                             "there.")
                       .on_change(relabel);

const char* format_for(float v) {
    if (v <= 1.0f) return "Original";
    if (v >= kUnlimitedStep) return "Unlimited";
    return "%.0fx";
}
void relabel(float v) { s_distance.format(format_for(v)); }

// The value loaded from settings.txt gets its label before the menu first shows it.
struct Label {
    Label() {
        settings::on_guest_frame([](uint32_t) {
            static bool done;
            if (!done) { done = true; relabel(s_distance.get()); }
        });
    }
} s_label;

// For testing: SFASSAULT_DRAW_DISTANCE=<1 to 9> draws at that step whatever the setting or a
// replay's log says, to compare a recording's frames at another distance (the replay may then
// part from the recording in heavy scenes; see the top).
float forced() {
    static const float v = [] {
        const char* e = getenv("SFASSAULT_DRAW_DISTANCE");
        return e ? (float)atof(e) : 0.0f;
    }();
    return v;
}

double factor() {
    const float v = forced() > 0.0f ? forced() : s_distance.get();
    if (v >= kUnlimitedStep) return kUnlimitedFactor;
    return v < 1.0f ? 1.0 : (double)v;
}

}  // namespace

// The game's stores to the draw distance and its offset (recomp/patches.txt): returns what
// to store instead.
extern "C" double sfassault_draw_range(double v) { return v * factor(); }

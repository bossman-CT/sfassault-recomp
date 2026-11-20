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
// 0x80385DD4, and sda-29512, 0x80385DD8). The game reads them at 23 places, all in model draw
// code; every read goes through sfassault_draw_range (recomp/patches.txt), which gives the
// value times the setting. The game's stores are untouched. A model past the original
// distance is drawn only while the frame's display-list buffer has room to spare, so the extra
// models can never overflow it (the game never checks).
//
// Drawing more is still more work for the guest's CPU, and the game's timing follows its CPU
// time: a heavy scene that no longer fits its frame shifts what happens after it. So the
// setting is one the guest sees, latched at a frame boundary and logged in the input log,
// and a replay draws at the distance it was recorded with. Its default is Original, which
// is what every recording made before it existed was made with.
//
// See docs/dev/draw-distance.md.
#include "runtime.h"
#include "settings.h"

#include <cstdio>
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

// The display-list buffer the game builds each frame's draws in (see the top): which half is
// this frame's (sda-26624), the write state whose +8 is where the next list goes (sda-26160),
// and the top of the half's queue nodes, which grow down to meet the lists (+40 in the table
// at 0x802FB050). The free space is what lies between, as fn_801752A4 works it out; a half
// is half the size given at boot (sda-26392).
constexpr uint32_t kHalfIndex = 0x80386920u, kWriteState = 0x80386AF0u, kTotalSize = 0x80386A08u;
constexpr uint32_t kQueueTable = 0x802FB050u;
bool in_ram(uint32_t a) { return a >= 0x80000000u && a < 0x81800000u; }

// A model beyond the original distance is drawn only while a frame's half still has this
// share of its room free. The heaviest frames at the original distance leave far more than
// that (docs/dev/draw-distance.md), so the extra models never take the room the game's own
// models need, and a frame cannot overflow the buffer.
constexpr double kKeepFree = 0.5;

uint32_t g_min_free = 0xFFFFFFFFu, g_half = 0;

bool room_for_more() {
    const uint32_t state = mem_r32(kWriteState);
    const uint32_t half = mem_r32(kTotalSize) / 2;
    if (!in_ram(state) || half == 0) return false;
    const uint32_t cur = mem_r32(state + 8);
    const uint32_t top = mem_r32(kQueueTable + (mem_r32(kHalfIndex) & 1) * 4 + 40);
    if (!in_ram(cur) || !in_ram(top) || top < cur) return false;
    const uint32_t free = top - cur;
    if (free < g_min_free) g_min_free = free;
    g_half = half;
    return free > half * kKeepFree;
}

// SFASSAULT_DRAW_DISTANCE_LOG=1: the least free room in the buffer over each 600 frames.
bool getenv_log() {
    static const bool on = getenv("SFASSAULT_DRAW_DISTANCE_LOG") != nullptr;
    return on;
}
struct Log {
    Log() {
        if (!getenv_log()) return;
        settings::on_guest_frame([](uint32_t frame) {
            if (frame % 600 || g_min_free == 0xFFFFFFFFu) return;
            fprintf(stderr, "[draw distance] frame %u: least free %u of %u bytes (%.0f%%)\n", frame, g_min_free,
                    g_half, g_half ? 100.0 * g_min_free / g_half : 0.0);
            g_min_free = 0xFFFFFFFFu;
        });
    }
} s_log;

}  // namespace

// The game's reads of the draw distance and its offset (recomp/patches.txt), all in model
// draw code: returns what the code is to use instead. At Original that is the value itself,
// and the buffer is not looked at.
extern "C" double sfassault_draw_range(double v) {
    const double f = factor();
    if (f == 1.0) {
        if (getenv_log()) room_for_more();
        return v;
    }
    return room_for_more() ? v * f : v;
}

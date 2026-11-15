// The Modern on-foot scheme (issue #48): Controls > On foot > Aiming = Modern.
//
// The game turns and pitches the pilot at a rate set by the C-stick (Dual Stick controller
// setting), in step with its own frames: nothing below 13 of the pad's 100, then in
// proportion to the deflection above 5, up to its top speed from 75 (about 200 degrees a
// second in one-player play; docs/dev/onfoot-controls.md). Modern works in those terms, a
// share of the top speed each game frame, so it holds in any scene and at any frame rate. It
// decides the share the player wants -- from the right stick through a radial dead zone, a
// response curve from the game's slowest turn up to a speed set for each axis, and an
// acceleration; from the gyro and the mouse by how far they move -- and gives the game the
// deflection for it. What a deflection can't give in a frame (a mouse flick past the top
// speed, the rounding to whole units, a turn slower than the game's slowest, given as short
// pushes at that speed) is kept and given in the following frames.
//
// This only changes the pad on its way to the game, so the input log records the result and
// replays need nothing more. Movement (the left stick) and every button are left as they
// are, except with the mouse: left button shoots (R), right button toggles the scope (X),
// the wheel switches weapons (A).
#include "onfoot_modern.h"
#include "input_ports.h"
#include "settings.h"
#include "gx/render.h"

#include <algorithm>
#include <cmath>

namespace onfoot_modern {
namespace {

const char* const kPath = "Controls/On foot";

auto& s_scheme = settings::choice("controls.onfoot.scheme", kPath, "Aiming", {"Original", "Modern"}, 0)
                     .help("Original: the game's own aiming. Modern: the right stick with a dead zone, a response "
                           "curve, acceleration and a speed for each axis, plus gyro and mouse aiming if turned on "
                           "below. Works with the game's Dual Stick controller setting (Options > User Settings > "
                           "your player > Controller Setting), where the C-stick aims; with the others the game's "
                           "own aiming stays. Up and down follow the game's Invert Y Axis for the pilot and Flip "
                           "aim up/down on foot, as with Original.");
bool modern() { return s_scheme.get() == 1; }
auto& s_sens_x = settings::slider("controls.onfoot.sensitivity_x", kPath, "Horizontal speed", 0.75f, 0.2f, 1.0f)
                     .format("%.2f")
                     .help("Turn speed with the right stick all the way over, as a share of the game's top speed "
                           "(about 200 degrees a second). Acceleration takes it the rest of the way.")
                     .enabled_if(modern, "With Aiming set to Modern");
auto& s_sens_y = settings::slider("controls.onfoot.sensitivity_y", kPath, "Vertical speed", 0.6f, 0.2f, 1.0f)
                     .format("%.2f")
                     .help("Looking up and down with the right stick all the way, as a share of the game's top speed.")
                     .enabled_if(modern, "With Aiming set to Modern");
auto& s_curve = settings::slider("controls.onfoot.curve", kPath, "Response curve", 2.0f, 1.0f, 3.0f)
                    .format("%.1f")
                    .help("How speed grows with how far the stick is pushed. 1 is a straight line, as the game "
                          "has it; 2 and above keep small movements slow for fine aim and save speed for the "
                          "edge.")
                    .enabled_if(modern, "With Aiming set to Modern");
auto& s_deadzone = settings::slider("controls.onfoot.deadzone", kPath, "Dead zone", 0.08f, 0.0f, 0.3f)
                       .format("%.2f")
                       .help("How far the right stick moves before it aims, as a share of its travel, measured "
                             "around the centre (radial). Raise it if the view drifts.")
                       .enabled_if(modern, "With Aiming set to Modern");
auto& s_accel = settings::toggle("controls.onfoot.acceleration", kPath, "Acceleration", true)
                    .help("Holding the right stick at the edge speeds turning up to the game's top speed after a "
                          "moment, for quick turns without giving up fine aim.")
                    .enabled_if(modern, "With Aiming set to Modern");
auto& s_gyro = settings::toggle("controls.onfoot.gyro", kPath, "Gyro aiming", false)
                   .help("Turning the controller (DualShock 4, DualSense, Switch Pro and the like) turns the view, "
                         "on foot only. Uses the gyro dead zone, smoothing and hold button set for the port in "
                         "controls.txt, and none of its other gyro settings.")
                   .enabled_if(modern, "With Aiming set to Modern");
auto& s_gyro_sens = settings::slider("controls.onfoot.gyro_sensitivity", kPath, "Gyro speed", 1.5f, 0.25f, 5.0f)
                        .format("%.2fx")
                        .help("1: in one-player play the view turns about as far as the controller does (split "
                              "screen, where the game runs fewer frames, half as far). The game's top speed still "
                              "limits how fast; a quicker turn catches up a moment later.")
                        .enabled_if(modern, "With Aiming set to Modern");
auto& s_mouse = settings::toggle("controls.onfoot.mouse", kPath, "Mouse aiming", false)
                    .help("On foot, the mouse turns the view (port 1). The window holds the cursor while you play; "
                          "the menu lets it go. Left button shoots, right button toggles the scope, the wheel "
                          "switches weapons.")
                    .enabled_if(modern, "With Aiming set to Modern");
auto& s_mouse_sens = settings::slider("controls.onfoot.mouse_sensitivity", kPath, "Mouse speed", 0.08f, 0.01f, 0.5f)
                         .format("%.3f deg")
                         .help("Degrees the view turns for each count of mouse movement in one-player play (half "
                               "that in split screen). The game's top speed still limits how fast; a flick catches "
                               "up a moment later.")
                         .enabled_if(modern, "With Aiming set to Modern");

// The game's response: the share of its top speed a deflection gives each frame.
constexpr int kZero = 5;     // where the line meets zero
constexpr int kFirst = 14;   // the smallest deflection that turns at once
constexpr int kFull = 75;    // the top speed's deflection
constexpr int kPulse = 3;    // frames a push at kFirst lasts: the game smooths away a single one
constexpr float kTopDegrees = 3.36f;  // degrees a frame at the top speed (one player, 60 fps)
float share_of(int d) { return d >= kFull ? 1.0f : d < kFirst ? 0.0f : (float)(d - kZero) / (kFull - kZero); }
const float kSlowest = share_of(kFirst);

struct Axis {
    float owed = 0.0f;      // top-speed frames asked for and not yet given
    bool rotation = false;  // some of it is the gyro's or the mouse's, which is given in full
    float last = 0.0f;      // what the deflection given last gives each frame
    int pulse = 0;          // frames left of a push at kFirst
    int dir = 0;            // its direction
};

// After `frames` frames of the last deflection, the deflection that gives the most of what is
// owed in the next frame (keeping at most `keep` owed). A turn slower than the game's slowest
// is given as pushes of kPulse frames at that speed.
int deflect(Axis& a, int frames, float keep) {
    a.owed = std::clamp(a.owed - a.last * (float)frames, -keep, keep);
    if (std::fabs(a.owed) < 0.01f) a.rotation = false;
    const float want = std::fabs(a.owed);
    const int sign = a.owed < 0 ? -1 : 1;
    int d = 0;
    if (want >= kSlowest) {
        d = sign * std::clamp((int)std::lround(kZero + want * (kFull - kZero)), kFirst, kFull);
        a.pulse = 0;
    } else if (a.pulse > 0) {  // a push goes on only while what is owed stays slow
        d = kFirst * a.dir;
        a.pulse--;
    } else if (want >= kSlowest * kPulse * 0.5f) {
        a.dir = sign;
        a.pulse = kPulse - 1;
        d = kFirst * sign;
    }
    a.last = share_of(std::abs(d)) * (d < 0 ? -1.0f : 1.0f);
    return d;
}

// The right stick's share of the top speed along one axis: nothing for the first 5% of the
// axis's part of the push (so pushing straight up doesn't drift sideways), then from the
// game's slowest turn up to `top` along the curve.
float shape(float v, float top) {
    const float n = (std::fabs(v) - 0.05f) / 0.95f;
    if (n <= 0.0f) return 0.0f;
    const float hi = std::max(top, kSlowest);
    return (v < 0 ? -1.0f : 1.0f) * (kSlowest + (hi - kSlowest) * std::pow(std::min(n, 1.0f), s_curve.get()));
}

uint8_t to_pad(int d) { return (uint8_t)std::clamp(128 + d, 1, 255); }

struct State {
    Axis x, y;
    float held = 0.0f;   // frames the stick has been at the edge
    float boost = 0.0f;  // the acceleration, 0 to 1
    uint8_t wheel = 0;   // frames the weapon switch (A) is still held for
    bool active = false;
    uint32_t frame = 0;  // the game frame last worked out
    uint8_t out_x = 0x80, out_y = 0x80;  // the C-stick it gave
};
State g_state[4];

}  // namespace

bool enabled() { return modern(); }

void apply(int port, PadState& p, bool active) {
    if (port < 0 || port > 3) return;
    State& s = g_state[port];
    const bool on = active && modern() && p.connected;
    input_ports::want(port, on ? ((s_gyro ? input_ports::WantGyro : 0u) | (s_mouse && port == 0 ? input_ports::WantMouse : 0u)) : 0u);
    if (!on) {
        s = State();
        return;
    }
    input_ports::Raw r;
    input_ports::raw(port, r);
    if (r.mouse) {  // owed until given: degrees, as frames of the top speed
        s.x.owed += r.mx * s_mouse_sens.get() / kTopDegrees;
        s.y.owed -= r.my * s_mouse_sens.get() / kTopDegrees;
        if (r.mx) s.x.rotation = true;
        if (r.my) s.y.rotation = true;
        if (r.mbuttons & 1) { p.buttons |= PAD_R; p.trig_r = 255; }
        if (r.mbuttons & 4) p.buttons |= PAD_X;
        if (r.wheel) s.wheel = 3;
    }
    if (s.wheel) p.buttons |= PAD_A;

    // Worked out once a game frame, since the game reads the pad once a frame, so that what a
    // deflection gave is known: its share for each frame it stood.
    const uint32_t frame = gx::g_frames_submitted.load();
    if (!s.active) {
        s.active = true;
        s.frame = frame - 1;
    }
    if (frame == s.frame) {
        p.cstick_x = s.out_x;
        p.cstick_y = s.out_y;
        return;
    }
    const int frames = (int)std::min<uint32_t>(frame - s.frame, 8);
    s.frame = frame;
    if (s.wheel) s.wheel--;

    // The right stick: a radial dead zone, then each axis's part of the push through the
    // curve, up to its speed, which the acceleration raises to the top speed.
    float want_x = 0.0f, want_y = 0.0f;
    if (r.pad) {
        const float m = std::hypot(r.rx, r.ry), dz = s_deadzone.get();
        if (m > dz) {
            const float n = std::min((m - dz) / std::max(1.0f - dz, 0.01f), 1.0f);
            if (s_accel && n > 0.9f) {
                s.held += (float)frames;
                if (s.held > 7.0f) s.boost = std::min(s.boost + frames / 15.0f, 1.0f);  // after 0.12 s, over 0.25 s
            } else {
                s.held = s.boost = 0.0f;
            }
            const float sx = s_sens_x.get(), sy = s_sens_y.get();
            want_x = shape(n * r.rx / m, sx + (1.0f - sx) * s.boost);
            want_y = shape(n * r.ry / m, sy + (1.0f - sy) * s.boost);
        } else {
            s.held = s.boost = 0.0f;
        }
    }
    // Letting go of the stick stops at once: what its rounding left is dropped, unless the
    // gyro or the mouse is owed some of it.
    if (want_x == 0.0f && !s.x.rotation) s.x.owed = s.x.last * frames, s.x.pulse = 0;
    if (want_y == 0.0f && !s.y.rotation) s.y.owed = s.y.last * frames, s.y.pulse = 0;
    s.x.owed += want_x * frames;
    s.y.owed += want_y * frames;
    // The gyro: degrees a second as shares of the top speed (about 200 degrees a second).
    if (r.gyro) {
        const float k = s_gyro_sens.get() / (kTopDegrees * 60.0f) * frames;
        s.x.owed += r.gx * k;
        s.y.owed += r.gy * k;
        if (r.gx) s.x.rotation = true;
        if (r.gy) s.y.rotation = true;
    }
    // How much may wait: about a frame for the stick, so letting go stops at once; more for
    // the gyro and the mouse, so a quick turn is given in full a moment later (less up and
    // down, where the game stops at its limit).
    const float keep_x = s.x.rotation ? 50.0f : 1.5f + frames, keep_y = s.y.rotation ? 10.0f : 1.5f + frames;
    s.out_x = to_pad(deflect(s.x, frames, keep_x));
    s.out_y = to_pad(deflect(s.y, frames, keep_y));
    p.cstick_x = s.out_x;
    p.cstick_y = s.out_y;
}

}  // namespace onfoot_modern

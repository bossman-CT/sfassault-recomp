// 120 fps (issue #12): Graphics > Display > Frame rate, 60 (the original) by default.
//
// The game paces itself on the retrace: after each frame it waits until a set number of
// fields has passed since the last one (one in play, two in some cutscenes), so it runs
// at 60 or 30 fps. Run on a VI that retraces twice a field (vi_set_rate) it runs at 120 or
// 60. The step scale (step_set_scale) is then 0.5: the patch at 8003AEC4 (recomp/
// patches.txt) uses it to tell the game's own time step it runs at twice its frames per
// second, and any site in recomp/steps.txt is halved. The CPU is made twice as fast too
// (clock_set_cpu_scale), so a frame's work takes the same share of its shorter frame.
//
// The setting is one the guest sees, so it is latched at a frame boundary and logged in
// the input log: a replay switches where its recording did, and stays exact.
//
// For testing: SFASSAULT_FPS=120 forces 120 whatever the setting says, and replays by
// virtual time (input_replay_by_time), so that a route recorded at 60 lands its inputs at
// the same moments at 120. SFASSAULT_FPS_AT=<seconds> holds it off until that much virtual
// time has passed, so a replay gets through the menus first. SFASSAULT_FRAMELOG=<file>
// writes each presented frame's number and virtual time (in time-base ticks), so that
// frames of a 60 and a 120 fps replay can be paired by the moment they show.
//
// See docs/dev/frame-rate.md for what is covered and what is not.
#include "runtime.h"
#include "input_log.h"
#include "settings.h"

#include <cstdlib>
#include <cstdio>

namespace {

auto& s_rate = settings::choice("graphics.frame_rate", "Graphics/Display", "Frame rate", {"60", "120"}, 0)
                   .help("120 runs the game at twice its frame rate and at its own speed. Mission 1's flight "
                         "has been checked; menus move their cursor twice as fast, and other scenes have not "
                         "been checked yet.")
                   .flags(settings::AffectsGuest);

bool g_forced;         // SFASSAULT_FPS=120
uint64_t g_forced_at;  // SFASSAULT_FPS_AT, in time-base ticks
FILE* g_frame_log;
bool g_double;         // running at 120 now

void set_double(bool on) {
    g_double = on;
    vi_set_rate(on ? 2.0 : 1.0);
    clock_set_cpu_scale(on ? 2.0 : 1.0);
    step_set_scale(on ? 0.5 : 1.0);
    fprintf(stderr, "[sfassault] frame rate %d\n", on ? 120 : 60);
}

void on_frame(uint32_t frame) {
    const bool want = g_forced ? now_ticks() >= g_forced_at : s_rate.get() == 1;
    if (want != g_double) {
        set_double(want);
        // A route recorded at 60 has to be replayed by time at 120. A recording made with
        // the setting has its switches in its log, and replays exactly by poll.
        if (g_forced) input_replay_by_time(want);
    }
    step_frame();
    if (g_frame_log) {
        fprintf(g_frame_log, "%u %llu\n", frame, (unsigned long long)now_ticks());
        fflush(g_frame_log);  // a replay is usually ended by killing it
    }
}

struct FrameRate {
    FrameRate() {
        const char* e = getenv("SFASSAULT_FPS");
        g_forced = e && atoi(e) == 120;
        if (const char* at = getenv("SFASSAULT_FPS_AT")) g_forced_at = (uint64_t)(atof(at) * TB_FREQ);
        if (const char* path = getenv("SFASSAULT_FRAMELOG")) g_frame_log = fopen(path, "w");
        settings::on_guest_frame(on_frame);
    }
} s_frame_rate;

}  // namespace

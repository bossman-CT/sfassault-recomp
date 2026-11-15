// 120 fps (issue #12), off unless SFASSAULT_FPS=120 is set.
//
// The game paces itself on the retrace: after each frame it waits until a set number of
// fields has passed since the last one (one in play, two in some cutscenes), so it runs
// at 60 or 30 fps. Run on a VI that retraces twice a field (vi_set_rate) it runs at 120 or
// 60, and every per-frame step listed in recomp/steps.txt is halved (step_set_scale), so
// the game keeps its speed. The CPU is made twice as fast too (clock_set_cpu_scale), so a
// frame's work takes the same share of its shorter frame as before.
//
// SFASSAULT_FPS_AT=<seconds> waits until that much virtual time has passed before
// switching, so that a replay reaches a scene in the same state at both rates (menus are
// not covered yet; a replay at 120 from boot goes elsewhere in them).
//
// SFASSAULT_FRAMELOG=<file> writes each presented frame's number and virtual time (in
// time-base ticks), so that frames of a 60 and a 120 fps replay can be paired by the
// moment they show.
//
// See docs/dev/frame-rate.md for which steps are covered and how to find more.
#include "runtime.h"
#include "gx/render.h"
#include "input_log.h"

#include <cstdlib>
#include <cstdio>

namespace {

bool g_double_rate;
uint64_t g_double_at;  // the virtual time to switch at (SFASSAULT_FPS_AT, seconds)
FILE* g_frame_log;

void on_frame(uint32_t frame) {
    static bool started;
    if (!started && now_ticks() >= g_double_at) {
        started = true;
        if (g_double_rate) {
            vi_set_rate(2.0);
            clock_set_cpu_scale(2.0);
            step_set_scale(0.5);
            // The game reads the pads once a frame, twice as often now: a route recorded
            // at 60 fps lands its inputs at the right moments only when replayed by time.
            input_replay_by_time(true);
            fprintf(stderr, "[sfassault] 120 fps: VI at twice the rate, steps halved\n");
        }
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
        g_double_rate = e && atoi(e) == 120;
        if (const char* at = getenv("SFASSAULT_FPS_AT")) g_double_at = (uint64_t)(atof(at) * TB_FREQ);
        if (const char* path = getenv("SFASSAULT_FRAMELOG")) g_frame_log = fopen(path, "w");
        gx::set_guest_frame_hook(on_frame);
    }
} s_frame_rate;

}  // namespace

// 120 fps (issues #12, #59): Graphics > Display > Frame rate, 60 (the original) by default.
// At 120 everything in a mission runs at twice its original rate: flight at 120, on foot
// and cutscenes (30 in the original) at 60. The front end's menus, loading screen and
// briefing keep the original rate (in_menus, below).
//
// The game paces itself on the retrace. Its main loop keeps a frame budget: a frame adds
// 1/fps of a second, its retrace callback pays 1/60 s back every retrace, and the loop
// waits on retraces until the budget is paid, so it runs at 60 or 30 fps. Run on a VI that
// retraces twice a field (vi_set_rate), with the game told it runs at twice its frames per
// second (the fps variable, below) and each retrace paying half as much (the retrace
// callback, below), it runs at 120 or 60. The step scale (step_set_scale) is then 0.5, and
// any site in recomp/steps.txt is halved. The CPU is made twice as fast too
// (clock_set_cpu_scale), so a frame's work takes the same share of its shorter frame.
//
// The game keeps its frames per second in one variable (sda-30252, 0x80385AF4: 60 in
// flight, 30 on foot and in some cutscenes) and converts by it wherever time meets frames:
// its time step (fn_8003AEB0), the frame budget, and some 80 sites in main.dol and the
// RELs that turn a duration or a rate into frames (t * 60 / fps, fps * 10 frames for ten
// seconds). So the variable holds the game's value divided by the step scale: every store
// the game makes to it goes through sfassault_fps_store (recomp/patches.txt), and a change
// of rate rewrites it. Every reader then follows, in whatever module.
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
#include "mmu.h"
#include "settings.h"

#include <cstdlib>
#include <cstdio>

namespace {

auto& s_rate = settings::choice("graphics.frame_rate", "Graphics/Display", "Frame rate", {"60", "120"}, 0)
                   .help("60 is the original: flight at 60 frames a second, on foot and cutscenes at 30. 120 "
                         "doubles that at the game's own speed: flight at 120, on foot and cutscenes at 60. "
                         "The title, menus and briefings keep their original rate either way.")
                   .flags(settings::AffectsGuest);

bool g_forced;         // SFASSAULT_FPS=120
uint64_t g_forced_at;  // SFASSAULT_FPS_AT, in time-base ticks
FILE* g_frame_log;
bool g_double;         // running at 120 now

}  // namespace

// The fps variable (see the top). g_fps_game is what the game last stored there, and
// g_fps_shown what the variable holds: that divided by the step scale, so the same at 60.
// Nothing is known until the game's first store, at boot (8000E13C).
namespace {
constexpr uint32_t kFpsVar = 0x80385AF4u;
bool g_fps_known;
uint32_t g_fps_game, g_fps_shown;

uint32_t fps_shown(uint32_t fps) { return (uint32_t)((double)(int32_t)fps / step_scale() + 0.5); }

void fps_rewrite() {
    if (!g_fps_known) return;
    g_fps_shown = fps_shown(g_fps_game);
    mem_w32(kFpsVar, g_fps_shown);
}
}  // namespace

// Called by the game's stores to the fps variable (recomp/patches.txt) with the value it
// stores; returns what to store instead.
extern "C" uint32_t sfassault_fps_store(uint32_t fps) {
    g_fps_known = true;
    g_fps_game = fps;
    return g_fps_shown = fps_shown(fps);
}

// The game's retrace callback (fn_8000DE20, recomp/patches.txt) counts in 60 Hz retraces:
// each one adds 1 to a retrace counter, 1/60 s (1092 in 16.16) to a clock, pays 1/60 s of
// the frame budget back (the main loop adds 1/fps a frame and waits on retraces until it
// is paid), and counts towards its once-a-second frame meter. On a VI that retraces N times
// a field, each retrace stands for 1/N of a field: the times are divided by N and the
// counts taken on every Nth retrace. Without that a 120 Hz retrace paid for a 1/60 s frame,
// and what runs at 30 in the original (on foot, the boot logos) ran at 120 on a 60 time
// step: twice its speed (issue #59).
namespace {
uint32_t g_retraces;  // callback runs
bool g_retrace_whole;  // this one ends a console field
}  // namespace
extern "C" void sfassault_retrace(void) {
    const uint32_t n = (uint32_t)(vi_rate() + 0.5);
    g_retrace_whole = n <= 1 || ++g_retraces % n == 0;
}
extern "C" uint32_t sfassault_retrace_whole(void) { return g_retrace_whole ? 1u : 0u; }
extern "C" uint32_t sfassault_retrace_ticks(void) { return (uint32_t)(1092.0 / vi_rate() + 0.5); }

namespace {

void set_double(bool on) {
    g_double = on;
    vi_set_rate(on ? 2.0 : 1.0);
    clock_set_cpu_scale(on ? 2.0 : 1.0);
    step_set_scale(on ? 0.5 : 1.0);
    fps_rewrite();
    fprintf(stderr, "[sfassault] frame rate %d\n", on ? 120 : 60);
}

// Says when the game's frames per second or its retraces per frame (r13-0x6780, read by
// the pacers) change, with what the fps variable shows: which scene runs at which rate.
void log_rates() {
    static uint32_t last_fps = ~0u, last_shown = ~0u, last_retraces = ~0u;
    const uint32_t retraces = mem_r32(0x803869A0u);
    if (!g_fps_known || (g_fps_game == last_fps && g_fps_shown == last_shown && retraces == last_retraces)) return;
    last_fps = g_fps_game, last_shown = g_fps_shown, last_retraces = retraces;
    fprintf(stderr, "[sfassault] game fps %u (shown %u), %u retrace%s a frame\n", g_fps_game, g_fps_shown, retraces,
            retraces == 1 ? "" : "s");
}

// A store to the fps variable that patches.txt doesn't route through sfassault_fps_store
// (through a pointer, say) would leave the game's own value there, and that much of the
// game would run at double speed at 120. Caught at the next frame boundary: taken as the
// game's value, and said once.
void fps_check() {
    if (!g_fps_known) return;
    const uint32_t v = mem_r32(kFpsVar);
    if (v == g_fps_shown) return;
    static bool said;
    if (!said) fprintf(stderr, "[sfassault] the fps variable was set to %u by a store patches.txt misses\n", v);
    said = true;
    g_fps_game = v;
    fps_rewrite();
}

// The front end runs at the original rate:
// - m2.rel, module 6: the menus (title, Select Player, Story, Level Select), linked from
//   the title until a mission is chosen. Their cursors repeat on frame counts.
// - m5.rel, module 9: the loading screen and briefing before a mission (30 fps in the
//   original). At twice that they ran 6 s ahead by the briefing's first lines.
// Read from the guest's own module list (the SDK's, at 0x800030C8: id at +0, next at +4),
// so a replay sees the same.
bool in_menus() {
    uint32_t p = 0;
    if (!vm_peek32(0x800030C8u, &p)) return false;
    for (int n = 0; p && n < 64; n++) {
        uint32_t id = 0, next = 0;
        if (!vm_peek32(p, &id) || !vm_peek32(p + 4, &next)) return false;
        if (id == 6 || id == 9) return true;
        p = next;
    }
    return false;
}

void on_frame(uint32_t frame) {
    static bool first = true;
    if (g_forced && first) {
        // A route recorded at 60 lands its inputs at the right moments at 120 only when
        // replayed by time. A recording made with the setting has its switches in its log,
        // and replays exactly by poll.
        input_replay_by_time(true);
    }
    first = false;
    fps_check();
    const bool on = g_forced ? now_ticks() >= g_forced_at : s_rate.get() == 1;
    const bool want = on && !in_menus();
    if (want != g_double) set_double(want);
    log_rates();
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

// Skip cutscenes at once (issue #41): Game > Quality of life, off by default.
//
// The game has two kinds of cutscene, and Start ends both, but not always:
//
// - Movies (Sofdec: the movie task, update 800C3258; the table at 8025A4A8 lists the
//   opening movie and the story's, such as Fortuna's intro and the Arwings' launch). The
//   task tests Start (the pad's pressed bits at 0x8029A8C0) only once the movie plays,
//   about a second after it starts, and ends it only once it has played 30 frames (the
//   count at +100).
// - Scenes the game plays itself (update 800AB4D0, made by fn_800A9E18 from the table of
//   36 at 8024923C), such as mission 1's descent to the planet. Each frame the scene asks
//   whether it has finished or Start is pressed (800ABE38), and if so ends, but only when
//   the next part of the mission has loaded (the flow object's +128 is 0) and its camera
//   timing allows.
//
// A press that comes before the game can act on it is lost, and the cutscene plays on.
// With the option a press is kept for as long as the same cutscene runs, so it ends as
// soon as the game lets it, and a movie needs to have played 1 frame, not 30. The game's
// own conditions still decide when, so nothing ends before what follows is ready.
//
// The setting is one the guest reads, so it is latched at a frame boundary and logged in
// the input log, and a replay does what its recording did. A kept press follows the guest's
// own pad and tasks, so a replay keeps it the same way.
#include "runtime.h"
#include "settings.h"

#include <cstdint>
#include <cstdio>

namespace {

auto& s_skip = settings::toggle("game.skip_cutscenes", "Game/Quality of life", "Skip cutscenes at once")
                   .help("A Start press during a movie or a scene ends it as soon as the game allows, even if "
                         "the game isn't ready for it yet: the game ignores a press while a movie is starting, "
                         "in its first half second, and while a scene is loading what comes next. Movies end "
                         "from their first frame.")
                   .flags(settings::AffectsGuest);

constexpr uint32_t kStart = 0x1000;  // the game's pad bits: Start

uint32_t g_frame;  // guest frames so far

void count_frame(uint32_t) { g_frame++; }

struct Init {
    Init() { settings::on_guest_frame(count_frame); }
} s_init;

// A press kept for one cutscene's task. The cutscene reports itself every frame it runs;
// a gap of a frame or more means it has ended, whatever task comes at that address next.
struct Kept {
    uint32_t task = 0, frame = 0;
    bool pressed = false;

    bool note(uint32_t t, bool now) {
        if (task != t || g_frame - frame > 1) {
            task = t;
            pressed = false;
        }
        frame = g_frame;
        if (now && !pressed) {
            pressed = true;
            fprintf(stderr, "[sfassault] skipping the cutscene as soon as it can end\n");
        }
        return pressed;
    }
};
Kept g_movie, g_scene;

}  // namespace

// The patch at 800C327C, the movie task's first instruction after its prologue, every
// frame it runs: `pad` is the pad's pressed bits.
extern "C" void sfa_movie_frame(uint32_t task, uint32_t pad) {
    if (s_skip.get()) g_movie.note(task, pad & kStart);
}

// The patch at 800C3374: the movie `task` tests Start, `pressed` this frame or 0.
extern "C" uint32_t sfa_movie_start(uint32_t task, uint32_t pressed) {
    if (!s_skip.get()) return pressed;
    return g_movie.task == task && g_movie.pressed ? kStart : pressed;
}

// The patch at 800C3398: how many frames a movie must have played before Start ends it.
extern "C" int32_t sfa_movie_skip_after(void) { return s_skip.get() ? 1 : 30; }

// The patch at 800ABE38: the scene `task` tests Start, `pressed` this frame or 0. It does
// so every frame it plays, so its kept press lasts until it ends.
extern "C" uint32_t sfa_scene_start(uint32_t task, uint32_t pressed) {
    if (!s_skip.get()) return pressed;
    return g_scene.note(task, pressed) ? kStart : 0;
}

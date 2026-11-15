// Skip the intro (issue #41): Game > Quality of life > Skip the intro, off by default.
//
// The game's top-level flow is an object whose state (+0x34) a function at 8000ECC8 steps
// every frame. At boot it starts a logo task (update 800110F0), which shows the health
// warning and the Nintendo, namco and Sofdec logos, waits for it to end, then (state 3)
// starts the opening movie (movie 0 of the table at 8025A4A8, through fn_800C2E18) and
// waits for that to end before loading the title (state 4). Two patches in
// recomp/patches.txt ask this file what to do:
//
// - "Logos": once the logo task has loaded its file and passed its first wait (the end of
//   its state 1), it goes to its last state (15) instead of the next, which removes the
//   task: what it does when the Sofdec logo has faded. Its file is loaded and freed
//   exactly as before, so the flow's resource stack is the same.
// - "Logos and movie": as well, the flow's state 3 doesn't start the movie the first time
//   after boot, so state 4 sees no movie running and loads the title at once, as it does
//   when a game returns to the title (state 0, which never plays it). The movie still
//   plays as the title's attract demo when the title is left alone.
//
// The setting is one the guest reads, so it is latched at a frame boundary and logged in
// the input log: a replay boots the way its recording did. It is read when the logos are
// on screen, so a change applies the next time the game starts (or Game > Reset).
#include "runtime.h"
#include "settings.h"

#include <cstdint>
#include <cstdio>

namespace {

enum { kOff, kLogos, kLogosAndMovie };

auto& s_skip = settings::choice("game.skip_intro", "Game/Quality of life", "Skip the intro",
                                {"Off", "Logos", "Logos and movie"}, kOff)
                   .help("Logos: starts without the health warning and the Nintendo, namco and Sofdec logos. "
                         "Logos and movie: without the opening movie too, straight to the title. It still "
                         "plays as the title's demo. Applies the next time the game starts (Game > Reset).")
                   .flags(settings::AffectsGuest);

// Set when the logo task ends its first wait, once per boot: the next movie the flow's
// state 3 starts is the opening one, not the title's demo.
bool g_boot_movie_next;

}  // namespace

// Called by the patch at 800111C0, the logo task's step from state 1 to `next` (2).
extern "C" uint32_t sfa_logos_next_state(uint32_t next) {
    g_boot_movie_next = true;
    if (s_skip.get() == kOff) return next;
    fprintf(stderr, "[sfassault] skipping the logos\n");
    return 15;
}

// Called by the patch at 8000F19C, where the flow's state 3 starts a movie: true to leave
// it out.
extern "C" int sfa_skip_boot_movie(void) {
    const bool boot = g_boot_movie_next;
    g_boot_movie_next = false;
    if (!boot || s_skip.get() != kLogosAndMovie) return 0;
    fprintf(stderr, "[sfassault] skipping the opening movie\n");
    return 1;
}

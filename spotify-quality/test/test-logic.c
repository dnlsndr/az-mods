/* SPDX-License-Identifier: GPL-3.0-only */
/* Host test: the selected quality replaces w2, Deck default leaves it alone,
 * and the other arguments and the return value pass through. */
#include <assert.h>
#include <stdio.h>

#include "../spotify-quality.c"

static int last_quality;
static int last_rc;
static int calls;

static int fake_add_track(void *dl, const char *uri, int quality, void *cb, void *progress,
                          void *user) {
    (void) dl;
    (void) uri;
    (void) cb;
    (void) progress;
    (void) user;
    calls++;
    last_quality = quality;
    return last_rc;
}

static int run(int in, int chosen, int rc) {
    calls = 0;
    last_quality = -1;
    last_rc = rc;
    __atomic_store_n(&g_quality, chosen, __ATOMIC_RELAXED);
    int got = add_track_hook((void *) 0x11, "spotify:track:x", in, (void *) 0x22, (void *) 0x33,
                             (void *) 0x44);
    assert(calls == 1);
    assert(got == rc);
    return last_quality;
}

int main(void) {
    orig_add_track = fake_add_track;

    assert(run(5, 8, 0) == 8);   /* Highest */
    assert(run(5, 0, 0) == 0);   /* Fastest */
    assert(run(8, 5, 0) == 5);   /* Default overrides a higher deck value */
    assert(run(5, -1, 0) == 5);  /* Deck default: untouched */
    assert(run(9, 8, 0) == 8);   /* Highest also lowers an above-table value */
    assert(run(5, 8, -3) == 8);  /* return value preserved */
    assert(g_levels[0] == -1 && g_levels[1] == 0 && g_levels[2] == 5 && g_levels[3] == 8);
    puts("PASS: selected quality replaces w2, Deck default leaves it, rc passes through");
    return 0;
}

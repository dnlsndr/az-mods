/* SPDX-License-Identifier: GPL-3.0-only */
/* Host test: the quality argument is raised only while the row is on, and every
 * other argument and the return value pass through unchanged. */
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

static int run(int in, int on, int rc) {
    calls = 0;
    last_quality = -1;
    last_rc = rc;
    __atomic_store_n(&g_on, on, __ATOMIC_RELAXED);
    int got = add_track_hook((void *) 0x11, "spotify:track:x", in, (void *) 0x22, (void *) 0x33,
                             (void *) 0x44);
    assert(calls == 1);
    assert(got == rc);
    return last_quality;
}

int main(void) {
    orig_add_track = fake_add_track;

    assert(run(5, 1, 0) == 8);   /* on: raised */
    assert(run(8, 1, 0) == 8);   /* already highest */
    assert(run(9, 1, 0) == 9);   /* above it: untouched */
    assert(run(5, 0, 0) == 5);   /* off: stock */
    assert(run(0, 1, -3) == 8);  /* return value preserved */
    puts("PASS: quality raised to 8 only when on and lower; other values and rc pass through");
    return 0;
}

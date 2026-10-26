/* SPDX-License-Identifier: GPL-3.0-only */
/* Host test for the two hooks' decision logic. It compiles the mod with AZ_HOST_TEST so the
 * static hooks are visible, and replaces EP147's functions with deterministic stand-ins. */
#include <assert.h>
#include <stdio.h>

#define AZ_HOST_TEST 1
#include "../record-streaming.c"

static int fake_rc;
static long fake_reason;
static int fake_calls;

static int fake_startable(void *self) {
    (void) self;
    fake_calls++;
    if (fake_reason)
        notify_hook(NULL, 24, 1, 0, fake_reason);
    return fake_rc;
}

static long fake_notify(void *self, long a1, long a2, long a3, long a4) {
    (void) self;
    (void) a1;
    (void) a2;
    (void) a3;
    (void) a4;
    return 0;
}

static int run(int rc, int reason, int on) {
    fake_rc = rc;
    fake_reason = reason;
    fake_calls = 0;
    __atomic_store_n(&g_on, on, __ATOMIC_RELAXED);
    int got = startable_hook(NULL);
    assert(fake_calls == 1);
    return got;
}

int main(void) {
    orig_startable = fake_startable;
    orig_notify = fake_notify;

    assert(run(1, 0, 1) == 1);  /* already startable: unchanged */
    assert(run(1, 0, 0) == 1);  /* off: unchanged */
    assert(run(0, 6, 1) == 1);  /* streaming, on: allowed */
    assert(run(0, 6, 0) == 0);  /* streaming, off: stock */
    assert(run(0, 5, 1) == 0);  /* another reason: never overridden */
    assert(run(0, 0, 1) == 0);  /* no reason notified: never overridden */
    assert(run(0, 6, 1) == 1);
    assert(run(0, 6, 1) == 1);

    /* A nested call must not clear the outer capture. */
    fake_rc = 0;
    fake_reason = 0;
    fake_calls = 0;
    __atomic_store_n(&g_on, 1, __ATOMIC_RELAXED);
    t_capture = 1;
    t_depth = 1;
    notify_hook(NULL, 0, 0, 0, 6);
    assert(t_reason == 6);
    t_capture = 0;
    t_depth = 0;

    puts("PASS: reason 6 overridden only when on; other reasons and off are stock");
    return 0;
}

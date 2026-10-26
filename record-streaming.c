/* SPDX-License-Identifier: GPL-3.0-only */
/**
 * @file record-streaming.c
 * @brief Keep MASTER REC running while tracks from a streaming service play.
 *
 * EP147's USB recorder asks usecase::usb_rec::UsbRecController whether recording may
 * start (0xd5cb78).  One of its checks looks at every loaded track's LoadTrackInfo
 * source field; a streaming track (source 6) makes it return 0 and tell its listeners
 * ReasonsForNotStart.  The same answer is used while recording, so loading a streaming
 * track stops the recording.
 *
 * The check has no single "is this a streaming track" function to hook.  Instead this
 * mod hooks the check itself and the notification sink it uses for the reason:
 *
 *   0xd65990  the reason notification (x4 = reason)
 *   0xd5cb78  the check (returns 1 = startable, 0 = not startable)
 *
 * The notification hook records the reason only while the check hook is running; the
 * check hook calls the original and changes a 0 with reason 6 (streaming) into 1.  Every
 * other reason (no recording device, a Bluetooth input, a track that is still loading,
 * ...) passes through unchanged.
 *
 * Both hooks are function entries, installed through claim_hook.  No code bytes are
 * patched.  The fingerprint guards refuse a build whose first 16 bytes differ.
 *
 * Off switch: the Mod Menu row.  Off leaves EP147 stock.
 */
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include "azmod-mod.h"

#define MOD_ID "record-streaming"
#define FW_TAG "ep147-9e9454e357389ece"
#define ADDR_STARTABLE ((uintptr_t) 0xd5cb78)
#define ADDR_NOTIFY ((uintptr_t) 0xd65990)
#define REASON_STREAMING 6

/* FNV-1a over the first 16 bytes of each hooked function.  Facts about the tested
 * build, not firmware bytes. */
#define FP_STARTABLE UINT64_C(0x55a7ccf8389336d6)
#define FP_NOTIFY UINT64_C(0x1694d1b2c38364f1)

static const az_core_v1 *C;
static int (*orig_startable)(void *self);
static long (*orig_notify)(void *self, long a1, long a2, long a3, long a4);
static int row_handle = -1;
static int g_on;

/* Per thread: the reason notification is only meaningful inside our startable call. */
static __thread int t_capture;
static __thread int t_depth;
static __thread int t_reason;
static int g_logged_override;
static int g_last_reason = -1;

static uint64_t code_fingerprint(uintptr_t address) {
    const unsigned char *p = (const unsigned char *) address;
    uint64_t h = UINT64_C(14695981039346656037);
    for (int i = 0; i < 16; i++)
        h = (h ^ p[i]) * UINT64_C(1099511628211);
    return h;
}

static int mod_on(void) { return __atomic_load_n(&g_on, __ATOMIC_RELAXED) != 0; }

/* 0xd65990(self, 24, 1, &value, reason): remember the reason while the check runs. */
static long notify_hook(void *self, long a1, long a2, long a3, long a4) {
    if (t_capture && t_depth == 1)
        t_reason = (int) a4;
    return orig_notify(self, a1, a2, a3, a4);
}

/* Tell the same listener list that recording is startable again.  The original check has
 * already sent "not startable, reason 6" through 0xd65990, and the UI greys the REC button
 * from that notification.  Slot +0x10 is notifyStartableRecording; the value pointer is only
 * there because the shared notifier reads one word from it. */
static void notify_startable(void *self) {
    if (!self || !orig_notify)
        return;
    long zero = 0;
    orig_notify((char *) self + 0x1a8, 0x10, 1, (long) (uintptr_t) &zero, 0);
}

/* 0xd5cb78(self): 1 = recording may start, 0 = it may not. */
static int startable_hook(void *self) {
    t_depth++;
    t_capture = 1;
    t_reason = 0;
    int rc = orig_startable(self);
    t_capture = 0;
    t_depth--;
    int last = __atomic_load_n(&g_last_reason, __ATOMIC_RELAXED);
    if (rc == 0 && t_reason != last) {
        __atomic_store_n(&g_last_reason, t_reason, __ATOMIC_RELAXED);
        if (C)
            C->log(MOD_ID, "recording refused, reason %d", t_reason);
    }
    if (rc == 0 && t_reason == REASON_STREAMING && mod_on()) {
        notify_startable(self);
        if (__atomic_exchange_n(&g_logged_override, 1, __ATOMIC_ACQ_REL) == 0 && C)
            C->log(MOD_ID, "streaming recording allowed (reason %d overridden, caller %p)",
                   t_reason, __builtin_return_address(0));
        return 1;
    }
    return rc;
}

static void setting_changed(int value, void *user) {
    (void) user;
    __atomic_store_n(&g_on, value != 0, __ATOMIC_RELEASE);
    if (C)
        C->log(MOD_ID, "recording while streaming: %s", value ? "on" : "off");
}

static int initialize(const az_core_v1 *api) {
    if (!api || api->abi != AZ_MOD_ABI)
        return 1;
    if (!AZ_HAS(api, fw_tag) || !AZ_HAS(api, claim_hook) || !AZ_HAS(api, register_row) ||
        !AZ_HAS(api, row_value))
        return 2;
    const char *tag = api->fw_tag();
    if (!tag || strcmp(tag, FW_TAG) != 0)
        return 3;
    if (code_fingerprint(ADDR_STARTABLE) != FP_STARTABLE || code_fingerprint(ADDR_NOTIFY) != FP_NOTIFY)
        return 4;
    C = api;

    const az_row_v1 row = {
        .size = sizeof(az_row_v1),
        .key = "allow",
        .label = "Recording while streaming",
        .kind = AZ_ROW_TOGGLE,
        .flags = AZ_ROW_DEFAULT_ON,
        .help = "Keeps MASTER REC running when a streaming service plays.\n"
                "Only the streaming block is removed; the other recording\n"
                "checks are left alone.",
        .on_change = setting_changed,
    };
    row_handle = api->register_row(MOD_ID, &row);
    if (row_handle < 0)
        return 5;
    int saved = api->row_value(row_handle);
    if (saved != 0 && saved != 1)
        return 6;
    setting_changed(saved, NULL);

    /* Claim the observer first: if the second claim is refused, the observer alone is
     * harmless and init returns non-zero, so the check is never altered. */
    if (api->claim_hook(MOD_ID, ADDR_NOTIFY, (void *) notify_hook, (void **) &orig_notify) != 0)
        return 7;
    if (api->claim_hook(MOD_ID, ADDR_STARTABLE, (void *) startable_hook,
                        (void **) &orig_startable) != 0)
        return 8;
    return 0;
}

static const az_mod_v1 mod = {
    .abi = AZ_MOD_ABI,
    .size = sizeof(az_mod_v1),
    .id = MOD_ID,
    .fw_tags = (const char *const[]) {FW_TAG, NULL},
    .init = initialize,
};

AZ_MOD_EXPORT const az_mod_v1 *az_mod_v1_entry(void) {
    return &mod;
}

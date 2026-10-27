/* SPDX-License-Identifier: GPL-3.0-only */
/**
 * @file spotify-quality.c
 * @brief Raise the Spotify DJ SDK's quality argument to its highest level.
 *
 * EP147's Spotify path calls the Spotify DJ SDK's DJDownload_add_track:
 *
 *   int DJDownload_add_track(DJDownload *dl, const char *uri,
 *                            DJAudioQuality quality,
 *                            AddTrackCallback cb, AddTrackProgressCallback progress,
 *                            void *user);
 *
 * The function is at 0x2827914 on EP147 2.00.  Its third argument is w2 and the
 * DJAudioQuality values are 0..8; 8 is the highest level the SDK exposes (the
 * "8 (Highest quality)" entry in its own quality table).  This mod hooks the
 * function entry, raises a lower value to 8, and calls the original with the
 * same five other arguments.  Values already at 8, or above it, pass through.
 *
 * This is the highest lossy level the deck's Spotify integration has.  It is
 * not Spotify Lossless/FLAC: the hardware eSDK's playback bitrate API has no
 * lossless setting (its Hifi24 value is track metadata only).
 *
 * Off switch: the Mod Menu row.  Off leaves the SDK call untouched.
 */
#include <stdint.h>
#include <string.h>
#include "azmod-mod.h"

#define MOD_ID "spotify-quality"
#define FW_TAG "ep147-9e9454e357389ece"
#define SP_ADD_TRACK ((uintptr_t) 0x2827914)
#define SP_MAX_QUALITY 8

/* FNV-1a over the first 16 bytes of DJDownload_add_track on EP147 2.00. */
#define SP_FINGERPRINT UINT64_C(0x00e943b25386ba34)

static const az_core_v1 *C;
static int (*orig_add_track)(void *dl, const char *uri, int quality, void *cb, void *progress,
                             void *user);
static int g_on;

static uint64_t code_fingerprint(uintptr_t address) {
    const unsigned char *p = (const unsigned char *) address;
    uint64_t h = UINT64_C(14695981039346656037);
    for (int i = 0; i < 16; i++)
        h = (h ^ p[i]) * UINT64_C(1099511628211);
    return h;
}

static int mod_on(void) { return __atomic_load_n(&g_on, __ATOMIC_RELAXED) != 0; }

/* DJDownload_add_track(dl, uri, quality, cb, progress, user): the SDK's own
 * function, with only w2 changed.  It returns DJDownloadError in w0. */
static int add_track_hook(void *dl, const char *uri, int quality, void *cb, void *progress,
                          void *user) {
    int before = quality;
    if (mod_on() && quality < SP_MAX_QUALITY)
        quality = SP_MAX_QUALITY;
    int rc = orig_add_track(dl, uri, quality, cb, progress, user);
    if (C && before != quality)
        C->log(MOD_ID, "DJAudioQuality %d -> %d", before, quality);
    return rc;
}

static void setting_changed(int value, void *user) {
    (void) user;
    __atomic_store_n(&g_on, value != 0, __ATOMIC_RELEASE);
    if (C)
        C->log(MOD_ID, "highest Spotify quality: %s", value ? "on" : "off");
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
    if (code_fingerprint(SP_ADD_TRACK) != SP_FINGERPRINT)
        return 4;
    C = api;

    const az_row_v1 row = {
        .size = sizeof(az_row_v1),
        .key = "highest",
        .label = "Highest Spotify quality",
        .kind = AZ_ROW_TOGGLE,
        .flags = AZ_ROW_DEFAULT_ON,
        .help = "Raises Spotify's DJ SDK quality to 8, the highest level\n"
                "the deck's integration exposes. Not Spotify Lossless/FLAC:\n"
                "the deck's hardware SDK has no lossless playback path.",
        .on_change = setting_changed,
    };
    int handle = api->register_row(MOD_ID, &row);
    if (handle < 0)
        return 5;
    int saved = api->row_value(handle);
    if (saved != 0 && saved != 1)
        return 6;
    setting_changed(saved, NULL);
    if (api->claim_hook(MOD_ID, SP_ADD_TRACK, (void *) add_track_hook,
                        (void **) &orig_add_track) != 0)
        return 7;
    return 0;
}

static const az_mod_v1 mod = {
    .abi = AZ_MOD_ABI,
    .size = sizeof(az_mod_v1),
    .id = MOD_ID,
    .fw_tags = (const char *const[]) { FW_TAG, NULL },
    .init = initialize,
};

AZ_MOD_EXPORT const az_mod_v1 *az_mod_v1_entry(void) {
    return &mod;
}

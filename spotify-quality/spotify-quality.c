/* SPDX-License-Identifier: GPL-3.0-only */
/**
 * @file spotify-quality.c
 * @brief Select the Spotify DJ SDK's quality level.
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
 * function entry and replaces a lower value with the level selected in the Mod
 * Menu.  "Deck default" leaves EP147's own value untouched.
 *
 * The choices are 0 (Fastest), 5 (Default) and 8 (Highest).  The intermediate
 * values 1-4 and 6-7 are not named by the SDK's table and are not exposed here.
 * This is still the highest lossy level: the hardware eSDK's playback bitrate
 * API has no lossless setting (its Hifi24 value is track metadata only).
 */
#include <stdint.h>
#include <string.h>
#include "azmod-mod.h"

#define MOD_ID "spotify-quality"
#define FW_TAG "ep147-9e9454e357389ece"
#define SP_ADD_TRACK ((uintptr_t) 0x2827914)

/* FNV-1a over the first 16 bytes of DJDownload_add_track on EP147 2.00. */
#define SP_FINGERPRINT UINT64_C(0x00e943b25386ba34)

/* The row's choice index -> the DJAudioQuality value.  -1 = leave EP147's value. */
static const int g_levels[] = {-1, 0, 5, 8};
static const char *const g_choices[] = {"Deck default", "Fastest (0)", "Default (5)",
                                        "Highest (8)"};

static const az_core_v1 *C;
static int (*orig_add_track)(void *dl, const char *uri, int quality, void *cb, void *progress,
                             void *user);
static int g_quality = 8; /* atomic: the DJAudioQuality to force, -1 for none */

static uint64_t code_fingerprint(uintptr_t address) {
    const unsigned char *p = (const unsigned char *) address;
    uint64_t h = UINT64_C(14695981039346656037);
    for (int i = 0; i < 16; i++)
        h = (h ^ p[i]) * UINT64_C(1099511628211);
    return h;
}

/* DJDownload_add_track(dl, uri, quality, cb, progress, user): the SDK's own
 * function, with only w2 changed.  It returns DJDownloadError in w0. */
static int add_track_hook(void *dl, const char *uri, int quality, void *cb, void *progress,
                          void *user) {
    int before = quality;
    int chosen = __atomic_load_n(&g_quality, __ATOMIC_RELAXED);
    if (chosen >= 0 && quality != chosen)
        quality = chosen;
    int rc = orig_add_track(dl, uri, quality, cb, progress, user);
    if (C && before != quality)
        C->log(MOD_ID, "DJAudioQuality %d -> %d", before, quality);
    return rc;
}

static void setting_changed(int value, void *user) {
    (void) user;
    if (value < 0 || value >= (int) (sizeof g_levels / sizeof g_levels[0]))
        return;
    __atomic_store_n(&g_quality, g_levels[value], __ATOMIC_RELEASE);
    if (C)
        C->log(MOD_ID, "Spotify quality: %s", g_choices[value]);
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
        .key = "quality",
        .label = "Spotify quality",
        .kind = AZ_ROW_CHOICE,
        .choices = g_choices,
        .n_choices = (int) (sizeof g_choices / sizeof g_choices[0]),
        .default_choice = 3, /* Highest (8) */
        .help = "Sets the Spotify DJ SDK's DJAudioQuality for the next track\n"
                "it adds. Highest (8) is the highest level the deck exposes.\n"
                "It is not Spotify Lossless/FLAC; the SDK has no lossless\n"
                "playback path. Deck default leaves EP147's value alone.",
        .on_change = setting_changed,
    };
    int handle = api->register_row(MOD_ID, &row);
    if (handle < 0)
        return 5;
    int saved = api->row_value(handle);
    if (saved < 0 || saved >= (int) (sizeof g_levels / sizeof g_levels[0]))
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

/* SPDX-License-Identifier: GPL-3.0-only */
/**
 * @file hide-beatgrid.c
 * @brief Hide the waveform beat rulers for beatmatching practice.
 *
 * EP147's gridMaker.makeGridImage (0x1dffe10) rasterizes the 1220x15 ARGB beat
 * rulers above and below each waveform.  The core's Bright Grid/Day Mode owns
 * that hook when active, so this mod first writes settings/azmod-gridaa.off and
 * then claims the hook.  On the first install the core still owns it: Reload
 * once, and the core skips makeGridImage because of the kill switch.  If Day
 * Mode or Bright Grid is wanted, this mod cannot coexist with it.
 *
 * The hook calls the original, then clears the ruler image.  The waveform,
 * BPM, cues and audio are untouched.  Off leaves the rulers stock.
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdatomic.h>
#include "azmod-mod.h"

#define MOD_ID "hide-beatgrid"
#define FW_TAG "ep147-9e9454e357389ece"
#define ADDR_GRID ((uintptr_t) 0x1dffe10) /* gridMaker.makeGridImage */
#define FP_GRID UINT64_C(0x16f0c8b5617a213a)
#define KILL_PATH "/home/root/settings/azmod-gridaa.off"

static const az_core_v1 *C;
static int *(*orig_grid)(float, int *, long *, int, int, int, int);
static int g_on;

static uint64_t code_fingerprint(uintptr_t address) {
    const unsigned char *p = (const unsigned char *) address;
    uint64_t h = UINT64_C(14695981039346656037);
    for (int i = 0; i < 16; i++)
        h = (h ^ p[i]) * UINT64_C(1099511628211);
    return h;
}

static int mod_on(void) { return __atomic_load_n(&g_on, __ATOMIC_RELAXED) != 0; }

/* gm+0x30 is juce::Image::BitmapData: data, format, lineStride, pixelStride,
 * width, height. ARGB premultiplied; the ruler is a separate image over the
 * waveform. */
static void blank_ruler(int *gm) {
    const char *b = (const char *) gm;
    uint8_t *data = *(uint8_t *const *) (b + 0x30);
    int fmt = *(const int *) (b + 0x38), ls = *(const int *) (b + 0x3c),
        ps = *(const int *) (b + 0x40);
    int w = *(const int *) (b + 0x44), h = *(const int *) (b + 0x48);
    if (!data || fmt != 2 || ps != 4 || w < 2 || w > 8192 || h < 1 || h > 4096 ||
        ls < w * 4)
        return;
    for (int y = 0; y < h; y++)
        memset(data + (long) y * ls, 0, (size_t) w * 4);
}

static int *grid_hook(float zoom, int *gm, long *content, int off, int h, int a6, int a7) {
    int *r = orig_grid(zoom, gm, content, off, h, a6, a7);
    if (mod_on())
        blank_ruler(gm);
    return r;
}

static void setting_changed(int value, void *user) {
    (void) user;
    __atomic_store_n(&g_on, value != 0, __ATOMIC_RELEASE);
    if (C)
        C->log(MOD_ID, "hide beat grid: %s", value ? "on" : "off");
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
    if (code_fingerprint(ADDR_GRID) != FP_GRID)
        return 4;
    C = api;

    /* The core installs its Bright Grid/Day Mode hook at EP147 start.  The kill
     * switch makes it skip that hook on the next start, freeing makeGridImage
     * for this mod. */
    if (access(KILL_PATH, F_OK) != 0) {
        FILE *f = fopen(KILL_PATH, "we");
        if (f)
            fclose(f);
        C->log(MOD_ID, "wrote azmod-gridaa.off; Reload once to free makeGridImage");
    }

    const az_row_v1 row = {
        .size = sizeof(az_row_v1),
        .key = "hide",
        .label = "Hide Beat Grid",
        .kind = AZ_ROW_TOGGLE,
        .flags = AZ_ROW_DEFAULT_ON,
        .help = "Hides the beat rulers above and below the waveforms, so\n"
                "you can practice matching by ear. BPM and waveform stay.\n"
                "Reload once after installing from off. Bright Grid and\n"
                "Day Mode cannot be used together with this mod.",
        .on_change = setting_changed,
    };
    int handle = api->register_row(MOD_ID, &row);
    if (handle < 0)
        return 5;
    int saved = api->row_value(handle);
    if (saved != 0 && saved != 1)
        return 6;
    setting_changed(saved, NULL);
    if (api->claim_hook(MOD_ID, ADDR_GRID, (void *) grid_hook, (void **) &orig_grid) != 0) {
        C->log(MOD_ID, "makeGridImage is owned by the core; Reload once after the kill switch");
        return 0; /* row is usable after the Reload; do not make the mod fail */
    }
    C->log(MOD_ID, "makeGridImage hook claimed");
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

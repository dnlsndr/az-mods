/* SPDX-License-Identifier: GPL-3.0-only */
/* Host test of the image blanking rule, no deck needed. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define main hide_beatgrid_main
#include "../hide-beatgrid.c"
#undef main

int main(void) {
    unsigned char mem[0x60 + 3 * 16];
    memset(mem, 0x5a, sizeof mem);
    unsigned char *data = mem + 0x20;
    int gm[0x50 / 4] = {0};
    *(uint8_t **) ((char *) gm + 0x30) = data;
    *(int *) ((char *) gm + 0x38) = 2;
    *(int *) ((char *) gm + 0x3c) = 3 * 4;
    *(int *) ((char *) gm + 0x40) = 4;
    *(int *) ((char *) gm + 0x44) = 3;
    *(int *) ((char *) gm + 0x48) = 3;
    memset(data, 0x11, 3 * 4 * 3);
    blank_ruler(gm);
    for (int i = 0; i < 3 * 4 * 3; i++)
        assert(data[i] == 0);
    puts("PASS: ruler image rows cleared");
    return 0;
}

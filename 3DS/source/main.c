/*
 *  ctroller -- use your 3DS as a gamepad on your PC
 *  Copyright (C) 2016  Philipp Joram (phijor)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ctroller.h"

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <3ds/result.h>
#include <3ds/console.h>
#include <3ds/env.h>

#include <3ds/services/apt.h>
#include <3ds/services/hid.h>
#include <3ds/services/soc.h>
#include <3ds/services/ac.h>
#include <3ds/services/irrst.h>

#ifdef DEBUG
#define EXIT_KEYS (KEY_START | KEY_SELECT)
#define EXIT_DESC "START+SELECT"
#else
#define EXIT_KEYS (KEY_START | KEY_DUP | KEY_L | KEY_B)
#define EXIT_DESC "START+UP+L+B"
#endif

#define SOCU_BUFSZ 0x100000
#define SOCU_ALIGN 0x1000

static u32 *sock_ctx = NULL;

int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    Result res = MAKERESULT(RL_SUCCESS, RS_SUCCESS, 0, RD_SUCCESS);

    gfxInitDefault();

    PrintConsole top;
    consoleInit(GFX_TOP, &top);

    util_debug_init();
    consoleSelect(&top);

    if (R_FAILED(res = acInit())) {
        util_presult("acInit failed", res);
        goto ac_failure;
    }

    if ((sock_ctx = memalign(SOCU_BUFSZ, SOCU_ALIGN)) == NULL) {
        util_perror("Allocating SOC buffer");
        res = MAKERESULT(
            RL_PERMANENT, RS_OUTOFRESOURCE, RM_SOC, RD_OUT_OF_MEMORY);
        goto soc_alloc_failure;
    }

    if (R_FAILED(res = socInit(sock_ctx, SOCU_BUFSZ))) {
        util_presult("socInit failed", res);
        goto soc_failure;
    }

    u32 wifi = 0;
    if (R_FAILED(res = ACU_GetWifiStatus(&wifi))) {
        util_presult("ACU_GetWifiStatus failed", res);
        fprintf(stderr, "Did you enable Wifi?\n");
        goto failure;
    }
    if (!wifi) {
        fprintf(stderr, "Wifi disabled.\n");
        goto failure;
    }

    u8 isNew3DS = 0;
    APT_CheckNew3DS(&isNew3DS);
    util_debug_printf("Running on %s3DS.\n", isNew3DS ? "New" : "Old");

    if (isNew3DS && R_FAILED(res = irrstInit())) {
        util_presult("irrstInit failed", res);
        goto irsst_failure;
    }

    if (R_FAILED(res = ctrollerInit())) {
        fprintf(stderr, "Do you have a valid IP in\n '" CFG_FILE "'?");
        goto failure;
    }

    bool isHomebrew = envIsHomebrew();
    printf("Press %s to exit.\n", isHomebrew ? EXIT_DESC : "HOME");
    fflush(stdout);

    while (aptMainLoop()) {

        if (isHomebrew) {
            if (hidKeysHeld() == EXIT_KEYS) {
                res = RL_SUCCESS;
                break;
            }
        }

        if (ctrollerSendHIDInfo()) {
            util_perror("Sending HID info");
        }

        gspWaitForVBlank();

        gfxFlushBuffers();
        gfxSwapBuffers();
    }

    puts("Exiting...");
failure:
    irrstExit();
irsst_failure:
    socExit();
soc_failure:
    free(sock_ctx);
soc_alloc_failure:
    acExit();
ac_failure:
    gfxExit();
    if (R_FAILED(res)) {
        util_hang(res);
    }
    return res;
}

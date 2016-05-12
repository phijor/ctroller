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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "ctroller.h"
#include "hid.h"

void on_terminate(int signum)
{
    (void) signum;
    puts("Exiting...");
    ctroller_exit();
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    (void) argc, (void) argv;
    int res = EXIT_SUCCESS;

    if (ctroller_init() == -1) {
        perror("Error initializing ctroller");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, on_terminate) == SIG_ERR) {
        fprintf(stderr, "Failed to register SIGINT handler.\n");
    }

    printf("Waiting for incoming packets...\n");

    int connected      = 0;
    struct hidinfo hid = {};
    while (1) {
        res = ctroller_poll_hid_info(&hid);
        if (res < 0) {
            fprintf(stderr, "An error occured (%d). Exiting...", res);
            fflush(stderr);
            res = EXIT_FAILURE;
            break;
        }

        if (res != 0) {
            if (!connected) {
                printf("Nintendo 3DS connected. (ctroller version "
                       "%01d.%01d.%01d)\n",
                       (hid.version & 0x0f00) >> 8,
                       (hid.version & 0x00f0) >> 4,
                       (hid.version & 0x000f) >> 0);
                connected = 1;
            }
            ctroller_write_hid_info(&hid);
        } else {
            connected = 0;
            memset(&hid, 0, sizeof(hid));
            puts("Timeout waiting for 3DS. Retrying...");
        }
    }

    ctroller_exit();

    return res;
}

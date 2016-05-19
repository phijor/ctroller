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

#define _GNU_SOURCE
#include <features.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <errno.h>
#include <getopt.h>
#include <unistd.h>

#include "ctroller.h"
#include "hid.h"

void on_terminate(int signum)
{
    (void) signum;
    puts("Exiting...");
    ctroller_exit();
    exit(EXIT_SUCCESS);
}

void print_usage(void)
{
    printf("Usage:\n");
    printf("  %s [<switches>]\n", program_invocation_short_name);
    printf("\n");

    printf("<switches>:\n");
#define print_opt(shortopt, longopt, desc)                                     \
    printf("  -%-1s  --%-22s " desc, shortopt, longopt)

    print_opt("d", "daemonize", "execute in background\n");
    print_opt("h", "help", "print this help text\n");
    print_opt("p",
              "port=<num>",
              "listen on port 'num' (defaults to " PORT_DEFAULT ")\n");
    print_opt("u",
              "uinput-device=<path>",
              "uinput character "
              "device (defaults to " UINPUT_DEFAULT_DEVICE ")\n");
#undef print_opt
}

int main(int argc, char *argv[])
{
    int res = EXIT_SUCCESS;

    // clang-format off
    struct options {
        char *uinput_device;
        char *port;
        int daemonize;
    } options = {
        .uinput_device = NULL,
        .port          = NULL,
        .daemonize     = 0,
    };

    static const struct option optstrings[] = {
        {"daemonize",       no_argument,       NULL, 'd'},
        {"help",            no_argument,       NULL, 'h'},
        {"port",            required_argument, NULL, 'p'},
        {"uinput-device",   required_argument, NULL, 'u'},
        {NULL,              0,                 NULL, 0},
    };
    // clang-format on

    int index = 0;
    int curopt;
    while ((curopt = getopt_long(argc, argv, "dhp:u:", optstrings, &index)) !=
           -1) {
        switch (curopt) {
        case 0:
            break;
        case 'd':
            options.daemonize = 1;
            break;
        case 'h':
            print_usage();
            return EXIT_SUCCESS;
        case 'p':
            options.port = optarg;
            break;
        case 'u':
            options.uinput_device = optarg;
            printf("uinput device: %s\n", optarg);
            break;
        case '?':
            print_usage();
            return EXIT_FAILURE;
        }
    }

    if (options.daemonize) {
        printf("Daemonizing %s...\n", program_invocation_short_name);
        res = daemon(1, 1);
        if (res == -1) {
            perror("Failed to daemonize process");
        }
    }

    if (ctroller_init(options.uinput_device, options.port) == -1) {
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
            if (!options.daemonize) {
                puts("Timeout waiting for 3DS. Retrying...");
            }
        }
    }

    ctroller_exit();

    return res;
}

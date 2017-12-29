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
#include "devices.h"

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
    printf("  -%-1s  --%-34s " desc, shortopt, longopt)

    print_opt("d", "daemonize", "execute in background\n");
    print_opt("h", "help", "print this help text\n");
    print_opt("p",
              "port=<num>",
              "listen on port 'num' (defaults to " PORT_DEFAULT ")\n");
    print_opt("u",
              "uinput-device=<path>",
              "uinput character "
              "device (defaults to " UINPUT_DEFAULT_DEVICE ")\n");
    print_opt("x",
              "exclude=<device1>[,<device2>,...]",
              "3DS devices that will not be provided to the system"
              " (possible values are: gamepad, touchscreen, gyroscope or "
              "accelerometer)\n");
#undef print_opt
}

static const struct device_name_to_id {
    const char *name;
    enum DEVICE_ID id;
} dev_to_id[] = {
    {"gamepad", DEVICE_GAMEPAD},
    {"touchscreen", DEVICE_TOUCHSCREEN},
    {"gyroscope", DEVICE_GYROSCOPE},
    {"accelerometer", DEVICE_ACCELEROMETER},
};

static device_mask_t parse_device_mask(const char *device_list)
{
    device_mask_t mask  = 0;
    const char *cur_dev = device_list;
    const char *end;

    do {
        end = strchrnul(cur_dev, ',');

        fprintf(
            stderr, "parsing dev mask: %.*s\n", (int) (end - cur_dev), cur_dev);

        for (size_t i = 0; i < arrsize(dev_to_id); i++) {
            if (strncmp(dev_to_id[i].name, cur_dev, end - cur_dev) == 0) {
                mask |= (1 << dev_to_id[i].id);
            }
        }
        cur_dev = end + 1;
    } while (*end != '\0');

    return mask;
}

int main(int argc, char *argv[])
{
    int exit_code = EXIT_SUCCESS;

    // clang-format off
    struct options {
        char *uinput_device;
        char *port;
        int daemonize;
        unsigned device_exclude_mask;
    } options = {
        .uinput_device       = NULL,
        .port                = NULL,
        .daemonize           = 0,
        .device_exclude_mask = 0,
    };

    static const struct option optstrings[] = {
        {"daemonize",       no_argument,       NULL, 'd'},
        {"help",            no_argument,       NULL, 'h'},
        {"port",            required_argument, NULL, 'p'},
        {"uinput-device",   required_argument, NULL, 'u'},
        {"exclude",         required_argument, NULL, 'x'},
        {NULL,              0,                 NULL, 0},
    };
    // clang-format on

    int index = 0;
    int curopt;
    while ((curopt = getopt_long(argc, argv, "dhp:u:x:", optstrings, &index)) !=
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
        case 'x':
            options.device_exclude_mask = parse_device_mask(optarg);
            break;
        case '?':
            print_usage();
            return EXIT_FAILURE;
        }
    }

    fprintf(stderr,
            "options: deamonize=%d, port=%s, uinput_device=%s, exclude=0%o\n",
            options.daemonize,
            options.port,
            options.uinput_device,
            options.device_exclude_mask);

    if (options.daemonize) {
        printf("Daemonizing %s...\n", program_invocation_short_name);
        exit_code = daemon(1, 1);
        if (exit_code == -1) {
            perror("Failed to daemonize process");
        }
    }

    if (ctroller_init(options.uinput_device,
                      options.port,
                      ~options.device_exclude_mask) == -1) {
        perror("Error initializing ctroller");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, on_terminate) == SIG_ERR) {
        fprintf(stderr, "Failed to register SIGINT handler.\n");
    }

    enum STATE {
        BROADCASTING,
        CONNECTING,
        RECEIVING,
    } state = BROADCASTING;

    struct hidinfo hid = {};
    while (1) {
        switch (state) {
        case BROADCASTING: {
            if (!options.daemonize) {
                printf("Advertising server...\n");
            }

            if (ctroller_broadcast()) {
                perror("ctroller_broadcast");
            }

            state = CONNECTING;
            break;
        }
        case CONNECTING: {
            int ret = ctroller_poll_hid_info(&hid, 3000);

            if (ret < 0) {
                fprintf(stderr, "Failed to read HID info: ret=%d\n", ret);
                exit_code = EXIT_FAILURE;
                break;
            }

            if (ret == 0) {
                state = BROADCASTING;
                if (!options.daemonize) {
                    puts("Timeout waiting for 3DS. Retrying...");
                }
            } else {
                state = RECEIVING;
                printf("Nintendo 3DS connected. (ctroller version "
                       "%01d.%01d.%01d)\n",
                       (hid.version & 0x0f00) >> 8,
                       (hid.version & 0x00f0) >> 4,
                       (hid.version & 0x000f) >> 0);
                if (hid.version != CTROLLER_VERSION) {
                    fprintf(stderr,
                            "Server version (%#04x) and client version "
                            "(%#04x) differ.\n",
                            CTROLLER_VERSION,
                            hid.version);
                }
                ctroller_write_hid_info(&hid);
            }
            exit_code = EXIT_SUCCESS;
            break;
        }
        case RECEIVING: {
            int ret = ctroller_poll_hid_info(&hid, -1);
            if (ret < 0) {
                fprintf(stderr, "Failed to read HID info: ret=%d\n", ret);
                exit_code = EXIT_FAILURE;
            }
            if (ret == 0) {
                printf("Nintendo 3DS disconnected. Trying to reconnect...\n");
                state = BROADCASTING;
                break;
            }
            ctroller_write_hid_info(&hid);
            break;
        }
        }

        if (exit_code != EXIT_SUCCESS) {
            fprintf(stderr, "An error occured (%d). Exiting...", exit_code);
            fflush(stderr);
            break;
        }
    }

    ctroller_exit();

    return exit_code;
}

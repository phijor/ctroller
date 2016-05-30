#include "devices.h"
#include "hid.h"

#include <stdio.h>

#include <unistd.h>

#include <linux/uinput.h>

static const struct uinput_user_dev touchscreen = {
    .name = "Nintendo 3DS Touchscreen",
    .id =
        {
            .vendor  = 0x057e,
            .product = 0x0402,
            .version = 1,
            .bustype = BUS_VIRTUAL,
        },
    .absmin[ABS_X]  = 0,
    .absmax[ABS_X]  = 320,
    .absflat[ABS_X] = 0,
    .absfuzz[ABS_X] = 0,

    .absmin[ABS_Y]  = 0,
    .absmax[ABS_Y]  = 240,
    .absflat[ABS_Y] = 0,
    .absfuzz[ABS_Y] = 0,
};

static const uint16_t keys[] = {
    BTN_TOUCH,
};

static const uint16_t axis[] = {
    ABS_X, ABS_Y,
};

#define NUMEVENTS (arrsize(keys) + arrsize(axis) + 1)

int touchscreen_create(const char *uinput_device)
{
    int uinputfd = device_open(uinput_device);
    if (uinputfd < 0) {
        goto failure_noclose;
    }

    int res;
    res = device_register_keys(uinputfd, keys, arrsize(keys));
    if (res != 1) {
        goto failure;
    }

    res = device_register_absaxis(uinputfd, axis, arrsize(axis));
    if (res != 2) {
        goto failure;
    }

    res = device_create(uinputfd, &touchscreen);
    if (res < 0) {
        goto failure;
    }

    return uinputfd;

failure:
    close(uinputfd);
failure_noclose:
    fprintf(stderr, "Failed to initialize touchscreen.\n");
    return -1;
}

int touchscreen_write(int uinputfd, struct hidinfo *hid)
{
    int res;
    static struct input_event events[NUMEVENTS];

    size_t i       = 0;
    events[i].type = EV_KEY;
    events[i].code = keys[i];
    events[i].value =
        HID_HAS_KEY(hid->keys.held | hid->keys.down, HID_KEY_TOUCH);
    i++;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_X;
    events[i].value = hid->touchscreen.px;
    i++;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_Y;
    events[i].value = hid->touchscreen.py;
    i++;

    events[i].type  = EV_SYN;
    events[i].code  = SYN_REPORT;
    events[i].value = 0;
    i++;

    res = write(uinputfd, events, i * sizeof(struct input_event));
    if (res < 0) {
        perror("Error writing touchscreen events");
    }
    return res;
}

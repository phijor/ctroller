#include "devices.h"
#include "hid.h"

#include <stdio.h>
#include <unistd.h>

#include <linux/uinput.h>

static const struct uinput_user_dev gamepad = {
    .name = "Nintendo 3DS",
    .id =
        {
            .vendor  = 0x057e,
            .product = 0x0401,
            .version = 1,
            .bustype = BUS_VIRTUAL,
        },

    // Circlepad
    .absmin[ABS_X]  = -0x9c,
    .absmax[ABS_X]  = 0x9c,
    .absflat[ABS_X] = 10,
    .absfuzz[ABS_X] = 3,

    .absmin[ABS_Y]  = -0x9c,
    .absmax[ABS_Y]  = 0x9c,
    .absflat[ABS_Y] = 10,
    .absfuzz[ABS_Y] = 3,

    // C-Stick
    .absmin[ABS_RX]  = -0x9c,
    .absmax[ABS_RX]  = 0x9c,
    .absflat[ABS_RX] = 10,
    .absfuzz[ABS_RX] = 3,

    .absmin[ABS_RY]  = -0x9c,
    .absmax[ABS_RY]  = 0x9c,
    .absflat[ABS_RY] = 10,
    .absfuzz[ABS_RY] = 3,
};

static const uint16_t keys[] = {
    BTN_EAST,
    BTN_SOUTH,
    BTN_NORTH,
    BTN_WEST,

    BTN_START,
    BTN_SELECT,

    BTN_TL,
    BTN_TR,

    BTN_TL2,
    BTN_TR2,

    BTN_DPAD_UP,
    BTN_DPAD_DOWN,
    BTN_DPAD_LEFT,
    BTN_DPAD_RIGHT,
};

static const uint32_t keymasks[] = {
    HID_KEY_A,
    HID_KEY_B,
    HID_KEY_X,
    HID_KEY_Y,

    HID_KEY_START,
    HID_KEY_SELECT,

    HID_KEY_L,
    HID_KEY_R,

    HID_KEY_ZL,
    HID_KEY_ZR,

    HID_KEY_DUP,
    HID_KEY_DDOWN,
    HID_KEY_DLEFT,
    HID_KEY_DRIGHT,
};

static const uint16_t axis[] = {
    // Circlepad
    ABS_X,
    ABS_Y,

    // C-Stick
    ABS_RX,
    ABS_RY,
};

#define NUMEVENTS (arrsize(keys) + arrsize(axis) + 1)

int gamepad_create(const char *uinput_device)
{
    int uinputfd = device_open(uinput_device);
    if (uinputfd < 0) {
        goto failure_noclose;
    }

    int res;
    res = device_register_keys(uinputfd, keys, arrsize(keys));
    if (res != arrsize(keys)) {
        goto failure;
    }

    res = device_register_absaxis(uinputfd, axis, arrsize(axis));
    if (res != arrsize(axis)) {
        goto failure;
    }

    res = device_create(uinputfd, &gamepad);
    if (res < 0) {
        goto failure;
    }

    return uinputfd;

failure:
    close(uinputfd);
failure_noclose:
    fprintf(stderr, "Failed to initialize Gamepad.\n");
    return -1;
}

int gamepad_write(int uinputfd, struct hidinfo *hid)
{
    int res;
    static struct input_event events[NUMEVENTS];

    size_t i = 0;
    for (; i < arrsize(keys); i++) {
        events[i].type = EV_KEY;
        events[i].code = keys[i];
        events[i].value =
            HID_HAS_KEY(hid->keys.held | hid->keys.down, keymasks[i]);
    }

    events[i].type  = EV_ABS;
    events[i].code  = ABS_X;
    events[i].value = hid->circlepad.dx;
    i++;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_Y;
    events[i].value = -hid->circlepad.dy;
    i++;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_RX;
    events[i].value = hid->cstick.dx;
    i++;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_RY;
    events[i].value = -hid->cstick.dy;
    i++;

    events[i].type  = EV_SYN;
    events[i].code  = SYN_REPORT;
    events[i].value = 0;
    i++;

    res = write(uinputfd, events, i * sizeof(struct input_event));
    if (res < 0) {
        perror("Error writing key events");
    }
    return res;
}

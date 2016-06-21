#include "devices.h"
#include "hid.h"

#include <stdio.h>

#include <unistd.h>

#include <linux/uinput.h>

static const struct uinput_user_dev accelerometer = {
    .name = "Nintendo 3DS Accelerometer",
    .id =
        {
            .vendor  = 0x057e,
            .product = 0x0404,
            .version = 1,
            .bustype = BUS_VIRTUAL,
        },
};

static const uint16_t axis[] = {
    ABS_X, ABS_Y, ABS_Z,
};

#define NUMEVENTS (arrsize(axis) + 1)

struct device_context device_accelerometer = {
    -1,
    accelerometer_write,
    accelerometer_create,
};

int accelerometer_create(const char *uinput_device)
{
    int uinputfd = device_open(uinput_device);
    if (uinputfd < 0) {
        goto failure_noclose;
    }

    int res;
    res = device_register_absaxis(uinputfd, axis, arrsize(axis));
    if (res != arrsize(axis)) {
        goto failure;
    }

    res = device_create(uinputfd, &accelerometer);
    if (res < 0) {
        goto failure;
    }

    return uinputfd;

failure:
    close(uinputfd);
failure_noclose:
    fprintf(stderr, "Failed to initialize accelerometer.\n");
    return -1;
}

int accelerometer_write(int uinputfd, struct hidinfo *hid)
{
    int res;
    static struct input_event events[NUMEVENTS];
    size_t i = 0;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_X;
    events[i].value = hid->accel.x;
    i++;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_Y;
    events[i].value = hid->accel.y;
    i++;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_Z;
    events[i].value = hid->accel.z;
    i++;

    events[i].type  = EV_SYN;
    events[i].code  = SYN_REPORT;
    events[i].value = 0;
    i++;

    res = write(uinputfd, events, i * sizeof(struct input_event));
    if (res < 0) {
        perror("Error writing accelerometer events");
    }
    return res;
}

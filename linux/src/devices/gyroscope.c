#include "devices.h"
#include "hid.h"

#include <stdio.h>

#include <unistd.h>

#include <linux/uinput.h>

#define GYRO_MAX_VAL 12000

static const struct uinput_user_dev gyroscope = {
    .name = "Nintendo 3DS Gyroscope",
    .id =
        {
            .vendor  = 0x057e,
            .product = 0x0403,
            .version = 1,
            .bustype = BUS_VIRTUAL,
        },

    .absmin[ABS_X]  = -GYRO_MAX_VAL,
    .absmax[ABS_X]  = GYRO_MAX_VAL,
    .absflat[ABS_X] = 0,
    .absfuzz[ABS_X] = 5,

    .absmin[ABS_Y]  = -GYRO_MAX_VAL,
    .absmax[ABS_Y]  = GYRO_MAX_VAL,
    .absflat[ABS_Y] = 0,
    .absfuzz[ABS_Y] = 5,

    .absmin[ABS_Z]  = -GYRO_MAX_VAL,
    .absmax[ABS_Z]  = GYRO_MAX_VAL,
    .absflat[ABS_Z] = 0,
    .absfuzz[ABS_Z] = 5,
};

static const uint16_t axis[] = {
    ABS_X, ABS_Y, ABS_Z,
};

#define NUMEVENTS (arrsize(axis) + 1)

int gyroscope_create(const char *uinput_device)
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

    res = device_create(uinputfd, &gyroscope);
    if (res < 0) {
        goto failure;
    }

    return uinputfd;

failure:
    close(uinputfd);
failure_noclose:
    fprintf(stderr, "Failed to initialize gyroscope.\n");
    return -1;
}

int gyroscope_write(int uinputfd, struct hidinfo *hid)
{
    int res;
    static struct input_event events[NUMEVENTS];
    size_t i = 0;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_X;
    events[i].value = hid->gyro.x;
    i++;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_Y;
    events[i].value = hid->gyro.y;
    i++;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_Z;
    events[i].value = hid->gyro.z;
    i++;

    events[i].type  = EV_SYN;
    events[i].code  = SYN_REPORT;
    events[i].value = 0;
    i++;

    res = write(uinputfd, events, i * sizeof(struct input_event));
    if (res < 0) {
        perror("Error writing gyroscope events");
    }
    return res;
}

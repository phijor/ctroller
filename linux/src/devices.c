#include "devices.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <linux/input.h>
#include <linux/uinput.h>

int device_open(const char *uinput_device)
{
    int uinputfd;
    if (uinput_device == NULL) {
        errno    = EINVAL;
        uinputfd = -1;
        goto error;
    }
    uinputfd = open(uinput_device, O_WRONLY | O_NONBLOCK);
    if (uinputfd < 0) {
        goto error;
    }
    return uinputfd;

error:
    fprintf(stderr,
            "Error opening uinput device at '%s': %s\n",
            uinput_device,
            strerror(errno));
    return uinputfd;
}

ssize_t
device_register_keys(const int uinputfd, const uint16_t *keycodes, size_t len)
{
    ssize_t res;
    res = ioctl(uinputfd, UI_SET_EVBIT, EV_KEY);
    if (res < 0) {
        perror("Failed to register event type for keys");
        return -1;
    }
    for (size_t i = 0; i < len; i++) {
        res = ioctl(uinputfd, UI_SET_KEYBIT, keycodes[i]);
        if (res < 0) {
            perror("Failed to register key");
            return i;
        }
    }
    return len;
}

ssize_t device_register_absaxis(const int uinputfd,
                                const uint16_t *axiscodes,
                                size_t len)
{
    ssize_t res;
    res = ioctl(uinputfd, UI_SET_EVBIT, EV_ABS);
    if (res < 0) {
        perror("Failed to register event type for absolute axis");
        return -1;
    }
    for (size_t i = 0; i < len; i++) {
        res = ioctl(uinputfd, UI_SET_ABSBIT, axiscodes[i]);
        if (res < 0) {
            perror("Failed to register absolute axis");
            return i;
        }
    }
    return len;
}

int device_create(int uinputfd, const struct uinput_user_dev *dev)
{
    int res;
    res = write(uinputfd, dev, sizeof(struct uinput_user_dev));
    if (res < 0) {
        perror("Failed to register virtual device");
        return -1;
    }

    res = ioctl(uinputfd, UI_DEV_CREATE);
    if (res < 0) {
        perror("Unable to create virtual device");
        return -1;
    }
    return uinputfd;
}

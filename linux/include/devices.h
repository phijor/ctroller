#ifndef DEVICES_H
#define DEVICES_H

#include <devices/gamepad.h>
#include <devices/touchscreen.h>
#include <devices/gyroscope.h>

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#define arrsize(a) (sizeof(a) / sizeof(a[0]))

int device_open(const char *uinput_device);

ssize_t
device_register_keys(const int uinputfd, const uint16_t *keycodes, size_t len);

ssize_t device_register_absaxis(const int uinputfd,
                                const uint16_t *axiscodes,
                                size_t len);

struct uinput_user_dev;
int device_create(int uinputfd, const struct uinput_user_dev *dev);

#endif /* ----- #ifndef DEVICES_H  ----- */

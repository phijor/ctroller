#ifndef HID_H
#define HID_H

#include <stdint.h>

#include <3ds/types.h>
#include <3ds/services/hid.h>

struct hidInfo {
    struct {
        u32 up;
        u32 down;
        u32 held;
    } keys;
    circlePosition circlepad;
    circlePosition cstick;
    touchPosition touchscreen;
    angularRate gyro;
};

int hidCollectData(struct hidInfo*);

#endif /* ----- #ifndef HID_H  ----- */

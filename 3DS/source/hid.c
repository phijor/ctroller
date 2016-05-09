#include "hid.h"

#include <3ds/result.h>
#include <3ds/services/hid.h>
#include <3ds/services/irrst.h>

int hidCollectData(struct hidInfo *info)
{
    info->keys.up   = hidKeysUp();
    info->keys.down = hidKeysDown();
    info->keys.held = hidKeysHeld();

    hidTouchRead(&info->touchscreen);
    hidCircleRead(&info->circlepad);
    hidGyroRead(&info->gyro);
    hidCstickRead(&info->cstick);

    return 0;
}

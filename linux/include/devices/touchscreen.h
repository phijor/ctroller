#ifndef TOUCHSCREEN_H
#define TOUCHSCREEN_H

int touchscreen_create(const char* uinput_device);

struct hidinfo;
int touchscreen_write(int uinputfd, struct hidinfo *hid);

#endif /* ----- #ifndef TOUCHSCREEN_H  ----- */

#ifndef GAMEPAD_H
#define GAMEPAD_H

int gamepad_create(const char *uinput_device);

struct hidinfo;
int gamepad_write(int uinputfd, struct hidinfo *hid);

#endif /* ----- #ifndef GAMEPAD_H  ----- */

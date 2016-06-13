#ifndef GYROSCOPE_H
#define GYROSCOPE_H

int gyroscope_create(const char *uinput_device);

struct hidinfo;
int gyroscope_write(int uinputfd, struct hidinfo *hid);

#endif /* ----- #ifndef GYROSCOPE_H  ----- */

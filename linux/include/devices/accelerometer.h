#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

int accelerometer_create(const char *uinput_device);

struct hidinfo;
int accelerometer_write(int uinputfd, struct hidinfo *hid);

#endif /* ----- #ifndef ACCELEROMETER_H  ----- */

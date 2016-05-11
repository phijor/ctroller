#include "ctroller.h"

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/poll.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/uinput.h>
#include <linux/input.h>

#include "hid.h"

#define PORT "15708"

static int ctroller_socket        = 0;
static int ctroller_uinput_device = 0;

struct sockaddr listen_addr;
socklen_t listen_addr_len;

static const struct keymap {
    uint16_t keycode;
    uint32_t keymask;
    const char *desc;
} keymap[] = {
    {BTN_TOUCH, HID_KEY_TOUCH, "Touch event"},

    {BTN_EAST, HID_KEY_A, "A Button"},
    {BTN_SOUTH, HID_KEY_B, "B Button"},
    {BTN_NORTH, HID_KEY_X, "X Button"},
    {BTN_WEST, HID_KEY_Y, "Y Button"},

    {BTN_START, HID_KEY_START, "START Button"},
    {BTN_SELECT, HID_KEY_SELECT, "SELECT Button"},

    {BTN_TL, HID_KEY_L, "L Button"},
    {BTN_TR, HID_KEY_R, "R Button"},

    {BTN_TL2, HID_KEY_ZL, "ZL Button"},
    {BTN_TR2, HID_KEY_ZR, "ZR Button"},

    {BTN_DPAD_UP, HID_KEY_DUP, "D-Pad Up"},
    {BTN_DPAD_DOWN, HID_KEY_DDOWN, "D-Pad Down"},
    {BTN_DPAD_LEFT, HID_KEY_DLEFT, "D-Pad Left"},
    {BTN_DPAD_RIGHT, HID_KEY_DRIGHT, "D-Pad Right"},
};
#define NUMKEYS (sizeof(keymap) / sizeof(keymap[0]))

static const struct axis {
    uint16_t axiscode;
    const char *desc;
} axis[] = {
    {ABS_X, "Circlepad X-Axis"},
    {ABS_Y, "Circlepad Y-Axis"},
    {ABS_RX, "C-Stick X-Axis"},
    {ABS_RY, "C-Stick Y-Axis"},
};
#define NUMAXIS (sizeof(axis) / sizeof(axis[0]))
#define NUMEVENTS (NUMKEYS + NUMAXIS + 1)

int ctroller_init()
{
    int res;
    if ((res = ctroller_listener_init()) < 0) {
        fprintf(stderr, "Failed to initialize listener.\n");
        return res;
    }

    if ((res = ctroller_uinput_init()) < 0) {
        fprintf(stderr, "Failed to create virtual device.\n");
        ctroller_exit();
        return res;
    }
    return 0;
}

int ctroller_listener_init()
{
    int res;

    struct addrinfo hints = {};

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;

    struct addrinfo *ctroller_info;
    if ((res = getaddrinfo(NULL, PORT, &hints, &ctroller_info))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
        return -1;
    }

    struct addrinfo *addr_info;
    ctroller_socket = -1;
    for (addr_info = ctroller_info; addr_info != NULL;
         addr_info = addr_info->ai_next) {
        if ((ctroller_socket = socket(addr_info->ai_family,
                                      addr_info->ai_socktype,
                                      addr_info->ai_protocol)) < 0) {
            perror("socket");
            continue;
        }

        if (bind(ctroller_socket, addr_info->ai_addr, addr_info->ai_addrlen) <
            0) {
            close(ctroller_socket);
            perror("bind");
            continue;
        }

        break;
    }

    if (addr_info == NULL) {
        errno = EADDRNOTAVAIL;
        return -1;
    }

    listen_addr     = *addr_info->ai_addr;
    listen_addr_len = addr_info->ai_addrlen;

    freeaddrinfo(ctroller_info);

    puts("Listening on port " PORT ".");
    return 0;
}

int ctroller_uinput_init()
{
    int res      = 0;
    int uinputfd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (uinputfd < 0) {
        perror("Error opening uinput device");
        return -1;
    }

    res = ioctl(uinputfd, UI_SET_EVBIT, EV_KEY);
    if (res < 0) {
        perror("Failed to register event type for keys");
        goto failure;
    }
    for (size_t i = 0; i < NUMKEYS; i++) {
        if ((res = ioctl(uinputfd, UI_SET_KEYBIT, keymap[i].keycode)) < 0) {
            fprintf(stderr,
                    "Failed to register %s: %s",
                    keymap[i].desc,
                    strerror(errno));
            goto failure;
        }
    }

    res = ioctl(uinputfd, UI_SET_EVBIT, EV_ABS);
    if (res < 0) {
        perror("Failed to register event type for absolute axis "
               "(Circlepad/C-Stick/Touchscreen)");
        goto failure;
    }

    for (size_t i = 0; i < NUMAXIS; i++) {
        if ((res = ioctl(uinputfd, UI_SET_ABSBIT, axis[i].axiscode)) < 0) {
            fprintf(stderr,
                    "Failed to register %s: %s",
                    axis[i].desc,
                    strerror(errno));
            goto failure;
        }
    }

    static const struct uinput_user_dev dev3ds = {
        .name = "Nintendo 3DS",
        .id =
            {
                .vendor  = 0x057e,
                .product = 0x0401,
                .version = 1,
                .bustype = BUS_VIRTUAL,
            },

        // Circlepad
        .absmin[ABS_X]  = -0x9c,
        .absmax[ABS_X]  = 0x9c,
        .absflat[ABS_X] = 10,
        .absfuzz[ABS_X] = 3,

        .absmin[ABS_Y]  = -0x9c,
        .absmax[ABS_Y]  = 0x9c,
        .absflat[ABS_Y] = 10,
        .absfuzz[ABS_Y] = 3,

        // C-Stick
        .absmin[ABS_RX]  = -0x9c,
        .absmax[ABS_RX]  = 0x9c,
        .absflat[ABS_RX] = 10,
        .absfuzz[ABS_RX] = 3,

        .absmin[ABS_RY]  = -0x9c,
        .absmax[ABS_RY]  = 0x9c,
        .absflat[ABS_RY] = 10,
        .absfuzz[ABS_RY] = 3,
    };

    res = write(uinputfd, &dev3ds, sizeof(dev3ds));
    if (res < 0) {
        perror("Failed to register virtual device");
        goto failure;
    }

    res = ioctl(uinputfd, UI_DEV_CREATE);
    if (res < 0) {
        perror("Unable to create virtual device");
        goto failure;
    }
    return ctroller_uinput_device = uinputfd;

failure:
    close(uinputfd);
    return -1;
}

int ctroller_recv(void *buf, size_t len)
{
    return recvfrom(
        ctroller_socket, buf, len, 0, &listen_addr, &listen_addr_len);
}

int ctroller_poll_hid_info(struct hidinfo *hid)
{
    int res = 0;
    packet_hid_t packet __attribute__((aligned(sizeof(uint32_t))));
    struct pollfd ufds;

    ufds.fd     = ctroller_socket;
    ufds.events = POLLIN;

    res = poll(&ufds, 1, 3 * 1000);
    if (res < 0) {
        perror("Error polling 3DS");
        return -1;
    } else if (res == 0) {
        // timeout
        return 0;
    }

    if (ufds.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        fprintf(stderr, "Polling 3DS: revents indicate error\n");
        return -1;
    }

    if (!(ufds.revents & POLLIN)) {
        return 0;
    }
    res = ctroller_recv(packet, PACKET_SIZE);
    if (res < 0) {
        perror("Error receiving packet");
        return -1;
    }

    res = ctroller_unpack_hid_info(packet, hid);
    return res;
}

inline void *ctroller_unpack_int16_t(unsigned char *buf, int16_t *val)
{
    *val = (int16_t) ntohs(*(uint16_t *) buf);
    return buf + sizeof(int16_t);
}

inline void *ctroller_unpack_uint16_t(unsigned char *buf, uint16_t *val)
{
    *val = ntohs(*(uint16_t *) buf);
    return buf + sizeof(uint16_t);
}

inline void *ctroller_unpack_uint32_t(unsigned char *buf, uint32_t *val)
{
    *val = ntohl(*(uint32_t *) buf);
    return buf + sizeof(uint32_t);
}

inline int ctroller_unpack_hid_info(unsigned char *sendbuf, struct hidinfo *hid)
{
    uint16_t magic;
    unsigned char *unpack = sendbuf;
    unpack = ctroller_unpack_uint16_t(unpack, &magic);
    if (magic != PACKET_MAGIC) {
        fprintf(stderr, "Invalid package header (%#08x).\n", magic);
        return -1;
    }

    unpack = ctroller_unpack_uint16_t(unpack, &hid->version);

    unpack = ctroller_unpack_uint32_t(unpack, &hid->keys.up);
    unpack = ctroller_unpack_uint32_t(unpack, &hid->keys.down);
    unpack = ctroller_unpack_uint32_t(unpack, &hid->keys.held);

    unpack = ctroller_unpack_uint16_t(unpack, &hid->touchscreen.px);
    unpack = ctroller_unpack_uint16_t(unpack, &hid->touchscreen.py);

    unpack = ctroller_unpack_int16_t(unpack, &hid->circlepad.dx);
    unpack = ctroller_unpack_int16_t(unpack, &hid->circlepad.dy);

    unpack = ctroller_unpack_int16_t(unpack, &hid->cstick.dx);
    unpack = ctroller_unpack_int16_t(unpack, &hid->cstick.dy);

#ifdef USE_GYRO
    unpack = ctroller_unpack_int16_t(unpack, &hid->gyro.x);
    unpack = ctroller_unpack_int16_t(unpack, &hid->gyro.y);
    unpack = ctroller_unpack_int16_t(unpack, &hid->gyro.z);
#endif // USE_GYRO
    return unpack - sendbuf;
}

int ctroller_write_hid_info(struct hidinfo *hid)
{
    int res;
    static struct input_event events[NUMEVENTS];

    size_t i = 0;
    for (; i < NUMKEYS; i++) {
        events[i].type = EV_KEY;
        events[i].code = keymap[i].keycode;
        events[i].value =
            HID_HAS_KEY(hid->keys.held | hid->keys.down, keymap[i].keymask);
    }

#define CTROLLER_WRITE_AXIS(ev, idx, axis, src)                                \
    do {                                                                       \
        ev[idx].type  = EV_ABS;                                                \
        ev[idx].code  = axis;                                                  \
        ev[idx].value = src;                                                   \
        idx++;                                                                 \
    } while (0)

    CTROLLER_WRITE_AXIS(events, i, ABS_X, hid->circlepad.dx);
    CTROLLER_WRITE_AXIS(events, i, ABS_Y, -hid->circlepad.dy);

    CTROLLER_WRITE_AXIS(events, i, ABS_RX, hid->cstick.dx);
    CTROLLER_WRITE_AXIS(events, i, ABS_RY, -hid->cstick.dy);

#undef CTROLLER_WRITE_AXIS

    events[i].type  = EV_SYN;
    events[i].code  = SYN_REPORT;
    events[i].value = 0;

    res = write(ctroller_uinput_device, events, sizeof(events));
    if (res < 0) {
        perror("Error writing key events");
    }
    return res;
}

void ctroller_exit()
{
    close(ctroller_socket);
    ctroller_socket = 0;
    ioctl(ctroller_uinput_device, UI_DEV_DESTROY);
    ctroller_uinput_device = 0;
    return;
}

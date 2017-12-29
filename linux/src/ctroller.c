#include "ctroller.h"
#include "devices.h"

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

static struct {
    int socket;
    struct device_context *devices[DEVICES_COUNT];
} ctroller = {
    .socket = -1,
    .devices =
        {
            [DEVICE_GAMEPAD]       = &device_gamepad,
            [DEVICE_TOUCHSCREEN]   = &device_touchscreen,
            [DEVICE_GYROSCOPE]     = &device_gyroscope,
            [DEVICE_ACCELEROMETER] = &device_accelerometer,
        },
};

struct sockaddr listen_addr;
socklen_t listen_addr_len;

int ctroller_init(const char *uinput_device,
                  const char *port,
                  device_mask_t device_mask)
{
    puts("Initializing ctroller version " CTROLLER_VERSION_STRING ".");
    int res;
    if ((res = ctroller_listener_init(port)) < 0) {
        fprintf(stderr, "Failed to initialize listener.\n");
        return res;
    }

    if ((res = ctroller_uinput_init(uinput_device, device_mask)) < 0) {
        fprintf(stderr, "Failed to create virtual device.\n");
        ctroller_exit();
        return res;
    }
    return 0;
}

int ctroller_listener_init(const char *port)
{
    int res;

    struct addrinfo hints = {};

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;

    if (port == NULL) {
        port = PORT_DEFAULT;
    }

    struct addrinfo *ctroller_info;
    if ((res = getaddrinfo(NULL, port, &hints, &ctroller_info))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
        return -1;
    }

    struct addrinfo *addr_info;
    ctroller.socket = -1;
    for (addr_info = ctroller_info; addr_info != NULL;
         addr_info = addr_info->ai_next) {
        if ((ctroller.socket = socket(addr_info->ai_family,
                                      addr_info->ai_socktype,
                                      addr_info->ai_protocol)) < 0) {
            perror("socket");
            continue;
        }

        if (bind(ctroller.socket, addr_info->ai_addr, addr_info->ai_addrlen) <
            0) {
            close(ctroller.socket);
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

    printf("Listening on port %s.\n", port);
    return 0;
}

int ctroller_uinput_init(const char *uinput_device, device_mask_t device_mask)
{
    if (uinput_device == NULL) {
        uinput_device = UINPUT_DEFAULT_DEVICE;
    }

    for (size_t i = 0; i < arrsize(ctroller.devices); i++) {
        if (device_mask & (1 << i)) {

            fprintf(stderr, "initializing device DEVICE_ID=%zu...\n", i);

            int fd = ctroller.devices[i]->create(uinput_device);
            if (fd < 0) {
                for (size_t j = 0; j < i; j++) {
                    close(ctroller.devices[j]->fd);
                    ctroller.devices[j]->fd = -1;
                }
                return -1;
            }
            ctroller.devices[i]->fd = fd;
        }
    }

    return 0;
}

#define sizeof_member(s, m) (sizeof(((s *) 0)->m))

struct broadcast_info {
    int sockfd;
    char info[sizeof(uint16_t) + sizeof(uint32_t) +
              sizeof_member(struct addrinfo, ai_addr->sa_data)];
    struct sockaddr_in addr;
} broadcast = {-1, {0}, {}};

int ctroller_broadcast_init()
{
    struct addrinfo hints;
    memset(&hints, '\0', sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;

    struct addrinfo *hostinfo, *info;
    if (getaddrinfo(NULL, PORT_DEFAULT, &hints, &hostinfo)) {
        perror("broadcast getaddrinfo");
        return 1;
    }

    broadcast.sockfd = -1;
    for (info = hostinfo; info != NULL; info = info->ai_next) {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &info->ai_addr, ip_str, INET_ADDRSTRLEN);
        fprintf(stderr, "host ip: %s\n", ip_str);
        if ((broadcast.sockfd = socket(info->ai_family,
                                       info->ai_socktype,
                                       info->ai_protocol)) == -1) {
            perror("ctroller_broadcast_init: socket");
            continue;
        }

        break;
    }

    if (broadcast.sockfd == -1) {
        freeaddrinfo(hostinfo);
        perror("broadcast socket");
        return 1;
    }

    if (setsockopt(broadcast.sockfd,
                   SOL_SOCKET,
                   SO_BROADCAST,
                   &(int){1},
                   sizeof(int))) {
        freeaddrinfo(hostinfo);
        perror("broadcast setsockopt(SO_BROADCAST)");
        return 1;
    }

    if (setsockopt(broadcast.sockfd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &(int){1},
                   sizeof(int))) {
        perror("broadcast setsockopt(SO_REUSEADDR)");
    }

    // if (bind(broadcast.sockfd, info->ai_addr, info->ai_addrlen) == -1) {
    //     freeaddrinfo(hostinfo);
    //     perror("ctroller_broadcast_init: bind");
    //     return 1;
    // }

    memset(&broadcast.addr, '\0', sizeof(broadcast.addr));
    broadcast.addr.sin_family      = AF_INET;
    broadcast.addr.sin_port        = htons(PORT_BC_NUM_DEFAULT);
    broadcast.addr.sin_addr.s_addr = INADDR_BROADCAST;

    *((uint16_t *) &broadcast.info[0]) = htons(PACKET_MAGIC);
    *((uint32_t *) &broadcast.info[2]) = htonl(info->ai_addrlen);
    memcpy(&broadcast.info[6],
           info->ai_addr->sa_data,
           sizeof(info->ai_addr->sa_data));

    freeaddrinfo(hostinfo);
    return 0;
}

int ctroller_broadcast()
{
    if (broadcast.sockfd == -1) {
        ctroller_broadcast_init();
    }
    int bytes_sent;
    if ((bytes_sent = sendto(broadcast.sockfd,
                             broadcast.info,
                             sizeof(broadcast.info),
                             0,
                             (struct sockaddr *) &broadcast.addr,
                             sizeof(broadcast.addr))) == -1) {
        perror("broadcast: sendto");
        return 1;
    }

    return 0;
}

int ctroller_broadcast_exit()
{
    return close(broadcast.sockfd);
}

int ctroller_recv(void *buf, size_t len)
{
    return recvfrom(
        ctroller.socket, buf, len, 0, &listen_addr, &listen_addr_len);
}

int ctroller_poll_hid_info(struct hidinfo *hid, int timeout)
{
    int res                                                        = 0;
    packet_hid_t packet __attribute__((aligned(sizeof(uint32_t)))) = {};
    struct pollfd ufds;

    ufds.fd     = ctroller.socket;
    ufds.events = POLLIN;

    res = poll(&ufds, 1, timeout);
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
    unpack                = ctroller_unpack_uint16_t(unpack, &magic);
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

    unpack = ctroller_unpack_int16_t(unpack, &hid->gyro.x);
    unpack = ctroller_unpack_int16_t(unpack, &hid->gyro.y);
    unpack = ctroller_unpack_int16_t(unpack, &hid->gyro.z);

    unpack = ctroller_unpack_int16_t(unpack, &hid->accel.x);
    unpack = ctroller_unpack_int16_t(unpack, &hid->accel.y);
    unpack = ctroller_unpack_int16_t(unpack, &hid->accel.z);

    return unpack - sendbuf;
}

int ctroller_write_hid_info(struct hidinfo *hid)
{
    for (size_t i = 0; i < arrsize(ctroller.devices); i++) {
        int devfd = ctroller.devices[i]->fd;
        if (devfd != -1) {
            ctroller.devices[i]->write(devfd, hid);
        }
    }
    return 0;
}

void ctroller_exit()
{
    close(broadcast.sockfd);
    close(ctroller.socket);
    ctroller.socket = -1;

    for (size_t i = 0; i < arrsize(ctroller.devices); i++) {
        ioctl(ctroller.devices[i]->fd, UI_DEV_DESTROY);
        ctroller.devices[i]->fd = -1;
    }

    return;
}

#include "ctroller.h"

#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "util.h"
#include "hid.h"

struct peer {
    int socket;
    struct addrinfo *addr_list;
    struct addrinfo *addr;
};

static struct peer SERVER = {
    .socket = -1, .addr_list = NULL, .addr = NULL,
};

// static int isNew3DS = 0;

Result ctrollerInit(void)
{
    int res = 0;

    struct sockaddr_in ctroller_addr = {};
    ctroller_addr.sin_family         = AF_INET;
    ctroller_addr.sin_addr.s_addr    = gethostid();

    struct addrinfo hints = {};
    hints.ai_family       = AF_INET;
    hints.ai_socktype     = SOCK_DGRAM;
    hints.ai_flags        = 0;

    char ctroller_ip[INET_ADDRSTRLEN];
    char server_ip[INET_ADDRSTRLEN];
    if (ctrollerReadServerIP(server_ip, INET_ADDRSTRLEN, CFG_FILE) < 0) {
        util_perror("Reading server IP");
        return MAKERESULT(RL_USAGE, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
    }

    util_debug_printf("Initializing ctroller...\n"
                      "- Version: " VERSION_BUILD " (" __DATE__ ")\n"
                      "- Config:  " CFG_FILE "\n"
                      "- Client:  %s\n"
                      "- Server:  %s\n"
                      "- Port:    " CFG_PORT "\n",
                      inet_ntop(ctroller_addr.sin_family,
                                &ctroller_addr.sin_addr,
                                ctroller_ip,
                                INET_ADDRSTRLEN),
                      server_ip);

    if ((res = getaddrinfo(server_ip, CFG_PORT, &hints, &SERVER.addr_list))) {
        util_debug_printf("getaddrinfo: %s\n", gai_strerror(res));
        errno = EADDRNOTAVAIL;
        return MAKERESULT(
            RL_PERMANENT, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
    }

    struct addrinfo *inf = SERVER.addr_list;
    for (; inf != NULL; inf = inf->ai_next) {
        SERVER.socket =
            socket(inf->ai_family, inf->ai_socktype, inf->ai_protocol);
        if (SERVER.socket < 0) {
            util_perror("socket");
            continue;
        }

        break;
    }

    if (inf == NULL || SERVER.socket < 0) {
        freeaddrinfo(SERVER.addr_list);
        SERVER.addr_list = NULL;
        errno = EADDRNOTAVAIL;
        return MAKERESULT(
            RL_PERMANENT, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
    }
    SERVER.addr = inf;

    return SERVER.socket;
}

int ctrollerReadServerIP(char *ipstr, size_t len, const char *path)
{
    FILE *fp;
    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }

    int res = 0;
    if (fgets(ipstr, len, fp) == NULL) {
        res = -1;
    }
    fclose(fp);

    /* Trim trailing newline character */
    char *lf = strchr(ipstr, '\n');
    if (lf != NULL) {
        *lf = '\0';
    }

    return res;
}

int ctrollerSend(const void *buf, size_t len)
{
    return sendto(SERVER.socket,
                  buf,
                  len,
                  0,
                  SERVER.addr->ai_addr,
                  SERVER.addr->ai_addrlen);
}

int ctrollerSendHIDInfo(void)
{
    int res = 0;
    struct hidInfo hid;
    res = hidCollectData(&hid);
    if (res) {
        util_debug_printf("Error reading HID peerinfo.\n");
        return res;
    }

    packet_hid_t packet;
    ctrollerPackHIDInfo(packet, &hid);

    res = ctrollerSend(packet, PACKET_SIZE);
    if (res == -1) {
        util_perror("HID send");
    }
    if (res > 0) {
        res = 0;
    }
    return res;
}

#define CTROLLER_PACK_DEFINE(type, packer)                                     \
    static inline uint8_t *pack_##type(uint8_t *buf, type val)                 \
    {                                                                          \
        type *target = (type *) buf;                                           \
        *target = packer(val);                                                 \
        return buf + sizeof(type);                                             \
    }

CTROLLER_PACK_DEFINE(int16_t, htons);
CTROLLER_PACK_DEFINE(uint16_t, htons);
CTROLLER_PACK_DEFINE(uint32_t, htonl);

#undef CTROLLER_PACK_DEFINE

int ctrollerPackHIDInfo(packet_hid_t packet, const struct hidInfo *hid)
{
    uint8_t *bufptr = packet;

    bufptr = pack_uint16_t(bufptr, PACKET_MAGIC);
    bufptr = pack_uint16_t(bufptr, PACKET_VERSION);

    bufptr = pack_uint32_t(bufptr, hid->keys.up);
    bufptr = pack_uint32_t(bufptr, hid->keys.down);
    bufptr = pack_uint32_t(bufptr, hid->keys.held);

    bufptr = pack_uint16_t(bufptr, hid->touchscreen.px);
    bufptr = pack_uint16_t(bufptr, hid->touchscreen.py);

    bufptr = pack_int16_t(bufptr, hid->circlepad.dx);
    bufptr = pack_int16_t(bufptr, hid->circlepad.dy);

    bufptr = pack_int16_t(bufptr, hid->cstick.dx);
    bufptr = pack_int16_t(bufptr, hid->cstick.dy);

    bufptr = pack_int16_t(bufptr, hid->gyro.x);
    bufptr = pack_int16_t(bufptr, hid->gyro.y);
    bufptr = pack_int16_t(bufptr, hid->gyro.z);

    bufptr = pack_int16_t(bufptr, hid->accel.x);
    bufptr = pack_int16_t(bufptr, hid->accel.y);
    bufptr = pack_int16_t(bufptr, hid->accel.z);

    return bufptr - packet;
}

void ctrollerExit(void)
{
    freeaddrinfo(SERVER.addr_list);
    SERVER.addr_list = NULL;
    SERVER.addr      = NULL;

    close(SERVER.socket);
    SERVER.socket = -1;

    return;
}

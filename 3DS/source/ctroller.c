#include "ctroller.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "hid.h"
#include "util.h"

struct peer {
    int socket;
    struct addrinfo *addr_list;
    struct addrinfo *addr;
};

static struct peer SERVER = {
    .socket    = -1,
    .addr_list = NULL,
    .addr      = NULL,
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
    uint32_t server_ip_num;

    const char *cfg = "dynamic";

    if (ctrollerDiscoverHosts(&server_ip_num) < 0) {
        util_perror("failed to discover host, falling back to config file\n");

        cfg = CFG_FILE;
        if (ctrollerReadServerIP(server_ip, INET_ADDRSTRLEN, CFG_FILE) < 0) {
            util_perror("Reading server IP");
            return MAKERESULT(
                RL_USAGE, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
        }

    } else {
        inet_ntop(AF_INET, &server_ip_num, server_ip, INET_ADDRSTRLEN);
    }

    util_debug_printf("Initializing ctroller...\n"
                      "- Version: " VERSION_BUILD " (" __DATE__ ")\n"
                      "- Config:  %s\n"
                      "- Client:  %s\n"
                      "- Server:  %s\n"
                      "- Port:    " CFG_PORT "\n",
                      cfg,
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
        errno            = EADDRNOTAVAIL;
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

int ctrollerDiscoverHosts(uint32_t *host_addr)
{
    int ret;
    int sockfd;
    struct addrinfo hints = {}, *hostinfo, *info = NULL;

    hints.ai_flags    = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;

    if ((ret = getaddrinfo(NULL, CFG_PORT_BC, &hints, &hostinfo))) {
        fprintf(stderr,
                "ctrollerDiscoverHosts: getaddrinfo: %s\n",
                gai_strerror(ret));
        return -1;
    }

    for (info = hostinfo; info != NULL; info = info->ai_next) {
        sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (sockfd < 0) {
            perror("ctrollerDiscoverHosts: socket");
            continue;
        }

        ret = bind(sockfd, info->ai_addr, info->ai_addrlen);
        if (ret == -1) {
            perror("ctrollerDiscoverHosts: bind");
            close(sockfd);
            continue;
        }

        freeaddrinfo(hostinfo);
        break;
    }

    if (info == NULL) {
        fprintf(stderr, "ctrollerDiscoverHosts: failed to bind socket\n");
        return -1;
    }

    uint16_t brd_package[2];

#define BRDI_MAGIC(p) (ntohs(p[0]))
#define BRDI_VERSION(p) (ntohs(p[1]))

    printf("Waiting for host advertisement...\n");

    int nbytes;
    struct sockaddr_in host_saddr;
    if ((nbytes = recvfrom(sockfd,
                           brd_package,
                           sizeof(brd_package),
                           0,
                           (struct sockaddr *) &host_saddr,
                           &((socklen_t){sizeof(host_saddr)}))) == -1) {
        perror("ctrollerDiscoverHosts: recvfrom");
        return -1;
    }

    char ip_str[INET_ADDRSTRLEN];
    fprintf(stderr,
            "received advertisement from %s\n",
            inet_ntop(AF_INET, &host_saddr.sin_addr, ip_str, INET_ADDRSTRLEN));

    if (nbytes < (int) sizeof(brd_package)) {
        fprintf(
            stderr, "ctrollerDiscoverHosts: package too short (%db)\n", nbytes);
        return -1;
    }

    if (BRDI_MAGIC(brd_package) != PACKET_MAGIC) {
        fprintf(stderr, "ctrollerDiscoverHosts: invalid package header\n");
        return -1;
    }

    uint16_t ver = BRDI_VERSION(brd_package);
    fprintf(stderr,
            "host version: %d.%d.%d)\n",
            (ver >> 8) & 0xf,
            (ver >> 4) & 0xf,
            ver & 0xf);
    if (ver != VERSION_BCD) {
        fputs("ctrollerDiscoverHosts: WARNING: version mismatch (client "
              "version: " VERSION "\n",
              stderr);
    }

    *host_addr = host_saddr.sin_addr.s_addr;

    return 0;
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
    // ctrollerSend returns a negative value on error
    return (res > 0) ? 0 : res;
}

#define CTROLLER_PACK_DEFINE(type, packer)                                     \
    static inline uint8_t *pack_##type(uint8_t *buf, type val)                 \
    {                                                                          \
        type *target = (type *) buf;                                           \
        *target      = packer(val);                                            \
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

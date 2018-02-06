#include "net.h"

#include <stdlib.h>    // for NULL
#include <errno.h>     // for errno
#include <string.h>    // for strcmp
#include <arpa/inet.h> // for struct sockaddr_in
#include <ifaddrs.h>   // for getifaddrs
#include <net/if.h>    // for IFF_BROADCAST

int net_get_host_address(const char *ifname,
                         struct sockaddr_in *addr,
                         struct sockaddr_in *brd_addr)
{
    struct ifaddrs *ifaddr;

    if (getifaddrs(&ifaddr) == -1) {
        return -1;
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {

        /* ignore any non IPv4 interface (the 3DS only does IPv4) */
        if (ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        /* if ifname was provided, only match interfaces with this name */
        if (ifname != NULL && strcmp(ifa->ifa_name, ifname) != 0) {
            continue;
        }

        /* if a broadcast address was requested, only match the interface if it
         * provides a broadcast address in ifa */
        if (brd_addr != NULL && !(ifa->ifa_flags & IFF_BROADCAST)) {
            continue;
        }

        /* found a matching interface; copy its address */
        if (addr != NULL) {
            memcpy(addr, ifa->ifa_addr, sizeof(struct sockaddr_in));
        }

        /* copy broadcast address */
        if (brd_addr != NULL) {
            memcpy(brd_addr, ifa->ifa_addr, sizeof(struct sockaddr_in));
        }

        freeifaddrs(ifaddr);
        return 0;
    }

    freeifaddrs(ifaddr);
    errno = EADDRNOTAVAIL;
    return -1;
}

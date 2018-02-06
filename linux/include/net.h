#ifndef NET_H
#define NET_H

#include <arpa/inet.h> // for struct sockaddr_in

int net_get_host_address(const char *ifname,
                         struct sockaddr_in *addr,
                         struct sockaddr_in *brd_addr);

#endif // ----- #ifndef NET_H -----

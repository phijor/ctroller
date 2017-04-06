#ifndef CTROLLER_H
#define CTROLLER_H

#include <stddef.h>

#include "hid.h"

#define _STRINGIFY(a) #a
#define STRINGIFY(a) _STRINGIFY(a)

#if !defined(VERSION_MAJOR) || !defined(VERSION_MINOR) ||                      \
    !defined(VERSION_PATCH)
#warning "VERSION_[MAJOR|MINOR|PATCH] not defined. Using defaults."

#ifndef VERSION_MAJOR
#define VERSION_MAJOR 0
#endif

#ifndef VERSION_MINOR
#define VERSION_MINOR 0
#endif

#ifndef VERSION_PATCH
#define VERSION_PATCH 0
#endif

#endif /* ----- #ifndef VERSION_[MAJOR|MINOR|PATCH] ----- */

#define VERSION                                                                \
    STRINGIFY(VERSION_MAJOR)                                                   \
    "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH)

#ifdef DEBUG
#define VERSION_BUILD VERSION " debug"
#else
#define VERSION_BUILD VERSION
#endif /* ----- #ifdef DEBUG  ----- */

#define MAKEBCDVER(major, minor, patch)                                        \
    ((major & 0xf) << 8 | (minor & 0xf) << 4 | (patch & 0xf))

/** Location of the config directory on the SD card
 **/
#define CFG_DIR "/ctroller/"

/** Location of the config file on the SD card
 **/
#define CFG_FILE CFG_DIR "ctroller.cfg"

/** Port of the server to connect to.
 **/
#define CFG_PORT "15708"

/** Initialize the ctroller client
 **/
Result ctrollerInit(void);

/** Exit the ctroller client and clean up ressources
 **/
void ctrollerExit(void);

/** Read the server IP from a file
 *
 * @param ipstr Pointer to array of chars to be filled with the server IP
 * @param len   Max. ammmount of bytes to be read from the file
 * @param path  Location of the file to read from on the SD
 *
 * @returns 0 on success
 * @returns < 0 on failure
 **/
int ctrollerReadServerIP(char *ipstr, size_t len, const char *path);

/** Magic constant identifying a ctroller packet
 **/
#define PACKET_MAGIC 0x3d5c

/** Constant identifying a packet version
 *
 * This value is represented in a BCD (binary-coded decimal) format.
 * I.e. '0x0123' stands for version 1.2.3
 **/
#define PACKET_VERSION MAKEBCDVER(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

/** Minimum number of bytes per package
 *
 * A package consists of metadata (magic value + version info) and the
 * contents of a hidinfo structure encoded in network byte order.
 **/
#define PACKET_SIZE (2 * sizeof(uint16_t) + sizeof(struct hidInfo))

/** A network packet that can hold all HID information collected
 **/
typedef uint8_t packet_hid_t[PACKET_SIZE];

struct hidInfo;
/** Write HID info into a packet ready to be sent
 *
 * @param packet Buffer to pack HID info into
 * @param hidinfo HID info collected from the system
 *
 * @returns Number of bytes written to packet
 **/
int ctrollerPackHIDInfo(packet_hid_t packet, const struct hidInfo *hid);

/** Send data to the server
 *
 * @param buf Pointer to data to be sent
 * @param len Number of bytes to be sent
 **/
int ctrollerSend(const void *buf, size_t len);

/** Collect and send HID information to the server
 **/
int ctrollerSendHIDInfo(void);

#endif /* ----- #ifndef CTROLLER_H  ----- */

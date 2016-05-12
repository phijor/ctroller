#include "util.h"

#include <3ds/services/hid.h>
#include <3ds/console.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <string.h>
#include <errno.h>

Result util_debug_result;

PrintConsole debug;

void util_hang(Result res)
{
    fprintf(stderr, "Press START to exit.\n");
    util_debug_printf("Will exit with status 0x%08x.\n", (unsigned int) res);
    while (1) {
        hidScanInput();

        if (hidKeysDown() & KEY_START)
            break;

        gspWaitForVBlank();

        gfxFlushBuffers();
        gfxSwapBuffers();
    }
}

void util_debug_init()
{
#ifdef DEBUG
    util_debug_result = 0;
#endif

    consoleInit(GFX_BOTTOM, &debug);
    consoleSetWindow(&debug, 0, 0, 40, 30);
    consoleDebugInit(debugDevice_CONSOLE);
}

__attribute__((format(printf, 1, 2))) void
util_debug_printf(const char *restrict fmt, ...)
{
    PrintConsole *tmp = consoleSelect(&debug);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stderr);
    consoleSelect(tmp);
}

void util_perror(const char *msg)
{
    char *delim       = (msg == NULL) ? "" : ": ";
    char *color       = (errno == 0) ? CONSOLE_GREEN : CONSOLE_RED;
    PrintConsole *tmp = consoleSelect(&debug);
    fprintf(stderr,
            "%s%s%s%s" CONSOLE_RESET "\n",
            msg,
            delim,
            color,
            strerror(errno));
    fflush(stderr);
    consoleSelect(tmp);
}

void util_presult(const char *msg, Result res)
{
    char *delim       = (msg == NULL) ? "" : ": ";
    char *color       = R_SUCCEEDED(res) ? CONSOLE_GREEN : CONSOLE_RED;
    PrintConsole *tmp = consoleSelect(&debug);
    fprintf(stderr,
            "Error %s0x%08x" CONSOLE_RESET "%s%s\n",
            color,
            (unsigned int) res,
            delim,
            msg);
    fflush(stderr);
    consoleSelect(tmp);
}

void util_debug_print_delim()
{
    PrintConsole *tmp = consoleSelect(&debug);
    for (int i = 0; i < debug.windowWidth; i++) {
        putchar('=');
    }
    consoleSelect(tmp);
}

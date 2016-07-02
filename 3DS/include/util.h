#ifndef UTIL_H
#define UTIL_H

#include <3ds/result.h>
#include <3ds/console.h>

#define inc_mod(a, b) (a = ((a) + 1) % (b))
#define dec_mod(a, b) (a = ((a) + (b) -1) % (b))

#define sizeof_member(s, m) sizeof(((s *) 0)->m)
#define sizeof_array(a) sizeof(a) / sizeof(a[0])

#define UTIL_STR_SUCCESS(s) CONSOLE_GREEN s CONSOLE_RESET
#define UTIL_STR_FAILURE(s) CONSOLE_RED s CONSOLE_RESET

#define CONSOLE_REVERSE CONSOLE_ESC(7m)

#ifdef DEBUG
#undef NDEBUG
#endif

#ifdef NDEBUG
#undef DEBUG
#endif

#if !defined(DEBUG) && !defined(NDEBUG)
#define DEBUG
#endif

#ifdef DEBUG
extern Result util_debug_result;
#define util_debug_exec(task)                                                  \
    do {                                                                       \
        util_debug_printf(__FILE__ "@l%d:\n " #task "\n", __LINE__);           \
        if ((util_debug_result = task)) {                                      \
            util_debug_printf(UTIL_STR_FAILURE(" Failed: %ld.\n"),             \
                              (long) util_assert_result);                      \
        }                                                                      \
        util_debug_printf(UTIL_STR_SUCCESS(" Success.\n"));                    \
    } while (0);
#else
#define util_debug_exec(task)                                                  \
    do {                                                                       \
        task;                                                                  \
    } while (0);
#endif

extern PrintConsole debug;

void util_debug_init(void);
void util_hang(Result);

__attribute__((format(printf, 1, 2))) void
util_debug_printf(const char *restrict fmt, ...);

void util_perror(const char *msg);
void util_presult(const char *msg, Result res) __attribute__((nonnull(1)));

void util_debug_print_delim(void);

#endif /* ----- #ifndef UTIL_H  ----- */

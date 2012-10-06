#ifndef _HELPERS_H
#define _HELPERS_H

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

#define LENGTH(x)         (sizeof(x) / sizeof(*x))
#define MAX(A, B)         ((A) > (B) ? (A) : (B))
#define MIN(A, B)         ((A) < (B) ? (A) : (B))
#define BOOLSTR(A)        ((A) ? "true" : "false")

#define XCB_CONFIG_WINDOW_X_Y_WIDTH_HEIGHT XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT
#define XCB_CONFIG_WINDOW_X_Y XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
#define MAXLEN 256

#define REMLEN(x)         (BUFSIZ - strlen(x) - 1)

#ifdef DEBUG
#  define PUTS(x)         puts(x)
#  define PRINTF(x,...)   printf(x, __VA_ARGS__)
#else
#  define PUTS(x)         ((void)0)
#  define PRINTF(x,...)   ((void)0)
#endif

void warn(char *, ...);
__attribute__((noreturn))
void err(char *, ...);
uint32_t get_color(char *);

#endif

#ifndef BSPWM_HELPERS_H
#define BSPWM_HELPERS_H

#include <xcb/xcb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define LENGTH(x)         (sizeof(x) / sizeof(*x))
#define MAX(A, B)         ((A) > (B) ? (A) : (B))
#define MIN(A, B)         ((A) < (B) ? (A) : (B))
#define BOOLSTR(A)        ((A) ? "true" : "false")

#define XCB_CONFIG_WINDOW_X_Y               XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
#define XCB_CONFIG_WINDOW_WIDTH_HEIGHT      XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT
#define XCB_CONFIG_WINDOW_X_Y_WIDTH_HEIGHT  XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT

#define MAXLEN    256
#define SMALEN     32
#define INIT_CAP    8

#define REMLEN(x)         (BUFSIZ - strlen(x) - 1)
#define streq(s1, s2)     (strcmp((s1), (s2)) == 0)

#ifdef DEBUG
#  define PUTS(x)         puts(x)
#  define PRINTF(x,...)   printf(x, __VA_ARGS__)
#else
#  define PUTS(x)         ((void)0)
#  define PRINTF(x,...)   ((void)0)
#endif

void warn(char *fmt, ...);
__attribute__((noreturn))
void err(char *fmt, ...);
bool get_color(char *col, xcb_window_t win, uint32_t *pxl);
double distance(xcb_point_t a, xcb_point_t b);

#endif

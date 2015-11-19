/* Copyright (c) 2012, Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
#define ONOFFSTR(A)       ((A) ? "on" : "off")
#define LAYERSTR(A)       ((A) == LAYER_BELOW ? "below" : ((A) == LAYER_NORMAL ? "normal" : "above"))
#define STATESTR(A)       ((A) == STATE_TILED ? "tiled" : ((A) == STATE_FLOATING ? "floating" : ((A) == STATE_FULLSCREEN ? "fullscreen" : "pseudo_tiled")))
#define IS_TILED(c)       (c->state == STATE_TILED || c->state == STATE_PSEUDO_TILED)
#define IS_FLOATING(c)    (c->state == STATE_FLOATING)
#define IS_FULLSCREEN(c)  (c->state == STATE_FULLSCREEN)

#define XCB_CONFIG_WINDOW_X_Y               (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y)
#define XCB_CONFIG_WINDOW_WIDTH_HEIGHT      (XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT)
#define XCB_CONFIG_WINDOW_X_Y_WIDTH_HEIGHT  (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT)

#define MAXLEN    256
#define SMALEN     32
#define INIT_CAP    8

#define streq(s1, s2)     (strcmp((s1), (s2)) == 0)

void warn(char *fmt, ...);
void err(char *fmt, ...);
bool get_color(char *col, xcb_window_t win, uint32_t *pxl);
double distance(xcb_point_t a, xcb_point_t b);

#endif

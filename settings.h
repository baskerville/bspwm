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

#ifndef BSPWM_SETTINGS_H
#define BSPWM_SETTINGS_H

#include "types.h"

#define WM_NAME                 "bspwm"
#define CONFIG_NAME             WM_NAME "rc"
#define CONFIG_HOME_ENV         "XDG_CONFIG_HOME"
#define EXTERNAL_RULES_COMMAND  ""
#define STATUS_PREFIX           "W"

#define FOCUSED_BORDER_COLOR          "#7E7F89"
#define ACTIVE_BORDER_COLOR           "#545350"
#define NORMAL_BORDER_COLOR           "#3F3E3B"
#define PRESEL_BORDER_COLOR           "#E8E8F4"
#define FOCUSED_LOCKED_BORDER_COLOR   "#C7B579"
#define ACTIVE_LOCKED_BORDER_COLOR    "#545350"
#define NORMAL_LOCKED_BORDER_COLOR    "#3F3E3B"
#define FOCUSED_STICKY_BORDER_COLOR   "#E3A5DA"
#define ACTIVE_STICKY_BORDER_COLOR    "#545350"
#define NORMAL_STICKY_BORDER_COLOR    "#3F3E3B"
#define FOCUSED_PRIVATE_BORDER_COLOR  "#42CAD9"
#define ACTIVE_PRIVATE_BORDER_COLOR   "#5C5955"
#define NORMAL_PRIVATE_BORDER_COLOR   "#34322E"
#define URGENT_BORDER_COLOR           "#EFA29A"

#define SPLIT_RATIO    0.5
#define WINDOW_GAP     6
#define BORDER_WIDTH   1

#define HISTORY_AWARE_FOCUS         false
#define FOCUS_BY_DISTANCE           false
#define BORDERLESS_MONOCLE          false
#define GAPLESS_MONOCLE             false
#define FOCUS_FOLLOWS_POINTER       false
#define POINTER_FOLLOWS_FOCUS       false
#define POINTER_FOLLOWS_MONITOR     false
#define AUTO_ALTERNATE              false
#define AUTO_CANCEL                 false
#define APPLY_FLOATING_ATOM         false
#define IGNORE_EWMH_FOCUS           false
#define CENTER_PSEUDO_TILED         true
#define REMOVE_DISABLED_MONITORS    false
#define REMOVE_UNPLUGGED_MONITORS   false
#define MERGE_OVERLAPPING_MONITORS  false

#define ENABLE_AUTOSPLIT_MODE       false

char external_rules_command[MAXLEN];
char status_prefix[MAXLEN];

char focused_border_color[MAXLEN];
char active_border_color[MAXLEN];
char normal_border_color[MAXLEN];
char presel_border_color[MAXLEN];
char focused_locked_border_color[MAXLEN];
char active_locked_border_color[MAXLEN];
char normal_locked_border_color[MAXLEN];
char focused_sticky_border_color[MAXLEN];
char active_sticky_border_color[MAXLEN];
char normal_sticky_border_color[MAXLEN];
char focused_private_border_color[MAXLEN];
char active_private_border_color[MAXLEN];
char normal_private_border_color[MAXLEN];
char urgent_border_color[MAXLEN];

double split_ratio;
int window_gap;
unsigned int border_width;
child_polarity_t initial_polarity;

bool borderless_monocle;
bool gapless_monocle;
bool focus_follows_pointer;
bool pointer_follows_focus;
bool pointer_follows_monitor;
bool apply_floating_atom;
bool auto_alternate;
bool auto_cancel;
bool history_aware_focus;
bool focus_by_distance;
bool ignore_ewmh_focus;
bool center_pseudo_tiled;
bool remove_disabled_monitors;
bool remove_unplugged_monitors;
bool merge_overlapping_monitors;

bool enable_autosplit_mode;

void run_config(void);
void load_settings(void);

#endif

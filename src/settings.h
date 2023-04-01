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

#define POINTER_MODIFIER         XCB_MOD_MASK_4
#define POINTER_MOTION_INTERVAL  17
#define EXTERNAL_RULES_COMMAND   ""
#define STATUS_PREFIX            "W"

#define NORMAL_BORDER_COLOR           "#30302f"
#define ACTIVE_BORDER_COLOR           "#474645"
#define FOCUSED_BORDER_COLOR          "#817f7f"
#define PRESEL_FEEDBACK_COLOR         "#f4d775"

#define PADDING              {0, 0, 0, 0}
#define MONOCLE_PADDING      {0, 0, 0, 0}
#define WINDOW_GAP           6
#define BORDER_WIDTH         1
#define SPLIT_RATIO          0.5
#define AUTOMATIC_SCHEME     SCHEME_LONGEST_SIDE
#define REMOVAL_ADJUSTMENT   true

#define PRESEL_FEEDBACK             true
#define BORDERLESS_MONOCLE          false
#define GAPLESS_MONOCLE             false
#define SINGLE_MONOCLE              false
#define BORDERLESS_SINGLETON        false

#define FOCUS_FOLLOWS_POINTER       false
#define POINTER_FOLLOWS_FOCUS       false
#define POINTER_FOLLOWS_MONITOR     false
#define CLICK_TO_FOCUS              XCB_BUTTON_INDEX_1
#define SWALLOW_FIRST_CLICK         false
#define IGNORE_EWMH_FOCUS           false
#define IGNORE_EWMH_FULLSCREEN      0
#define IGNORE_EWMH_STRUTS          false

#define CENTER_PSEUDO_TILED         true
#define HONOR_SIZE_HINTS            HONOR_SIZE_HINTS_NO
#define MAPPING_EVENTS_COUNT        1

#define REMOVE_DISABLED_MONITORS    false
#define REMOVE_UNPLUGGED_MONITORS   false
#define MERGE_OVERLAPPING_MONITORS  false

extern char external_rules_command[MAXLEN];
extern char status_prefix[MAXLEN];

extern char normal_border_color[MAXLEN];
extern char active_border_color[MAXLEN];
extern char focused_border_color[MAXLEN];
extern char presel_feedback_color[MAXLEN];

extern padding_t padding;
extern padding_t monocle_padding;
extern int window_gap;
extern unsigned int border_width;
extern double split_ratio;
extern child_polarity_t initial_polarity;
extern automatic_scheme_t automatic_scheme;
extern bool removal_adjustment;
extern tightness_t directional_focus_tightness;

extern uint16_t pointer_modifier;
extern uint32_t pointer_motion_interval;
extern pointer_action_t pointer_actions[3];
extern int8_t mapping_events_count;

extern bool presel_feedback;
extern bool borderless_monocle;
extern bool gapless_monocle;
extern bool single_monocle;
extern bool borderless_singleton;

extern bool focus_follows_pointer;
extern bool pointer_follows_focus;
extern bool pointer_follows_monitor;
extern int8_t click_to_focus;
extern bool swallow_first_click;
extern bool ignore_ewmh_focus;
extern bool ignore_ewmh_struts;
extern state_transition_t ignore_ewmh_fullscreen;

extern bool center_pseudo_tiled;
extern honor_size_hints_mode_t honor_size_hints;

extern bool remove_disabled_monitors;
extern bool remove_unplugged_monitors;
extern bool merge_overlapping_monitors;

void run_config(int run_level);
void load_settings(void);

#endif

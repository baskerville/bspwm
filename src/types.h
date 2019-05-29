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

#ifndef BSPWM_TYPES_H
#define BSPWM_TYPES_H
#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/randr.h>
#include <xcb/xcb_event.h>
#include "helpers.h"

#define MISSING_VALUE        "N/A"
#define MAX_WM_STATES        4

typedef enum {
	TYPE_HORIZONTAL,
	TYPE_VERTICAL
} split_type_t;

typedef enum {
	MODE_AUTOMATIC,
	MODE_MANUAL
} split_mode_t;

typedef enum {
	SCHEME_LONGEST_SIDE,
	SCHEME_ALTERNATE,
	SCHEME_SPIRAL
} automatic_scheme_t;

typedef enum {
	STATE_TILED,
	STATE_PSEUDO_TILED,
	STATE_FLOATING,
	STATE_FULLSCREEN
} client_state_t;

typedef enum {
	WM_FLAG_MODAL = 1 << 0,
	WM_FLAG_STICKY = 1 << 1,
	WM_FLAG_MAXIMIZED_VERT = 1 << 2,
	WM_FLAG_MAXIMIZED_HORZ = 1 << 3,
	WM_FLAG_SHADED = 1 << 4,
	WM_FLAG_SKIP_TASKBAR = 1 << 5,
	WM_FLAG_SKIP_PAGER = 1 << 6,
	WM_FLAG_HIDDEN = 1 << 7,
	WM_FLAG_FULLSCREEN = 1 << 8,
	WM_FLAG_ABOVE = 1 << 9,
	WM_FLAG_BELOW = 1 << 10,
	WM_FLAG_DEMANDS_ATTENTION = 1 << 11,
} wm_flags_t;

typedef enum {
	LAYER_BELOW,
	LAYER_NORMAL,
	LAYER_ABOVE
} stack_layer_t;

typedef enum {
	OPTION_NONE,
	OPTION_TRUE,
	OPTION_FALSE
} option_bool_t;

typedef enum {
	ALTER_TOGGLE,
	ALTER_SET
} alter_state_t;

typedef enum {
	CYCLE_NEXT,
	CYCLE_PREV
} cycle_dir_t;

typedef enum {
	CIRCULATE_FORWARD,
	CIRCULATE_BACKWARD
} circulate_dir_t;

typedef enum {
	HISTORY_OLDER,
	HISTORY_NEWER
} history_dir_t;

typedef enum {
	DIR_NORTH,
	DIR_WEST,
	DIR_SOUTH,
	DIR_EAST
} direction_t;

typedef enum {
	HANDLE_LEFT = 1 << 0,
	HANDLE_TOP = 1 << 1,
	HANDLE_RIGHT = 1 << 2,
	HANDLE_BOTTOM = 1 << 3,
	HANDLE_TOP_LEFT = HANDLE_TOP | HANDLE_LEFT,
	HANDLE_TOP_RIGHT = HANDLE_TOP | HANDLE_RIGHT,
	HANDLE_BOTTOM_RIGHT = HANDLE_BOTTOM | HANDLE_RIGHT,
	HANDLE_BOTTOM_LEFT = HANDLE_BOTTOM | HANDLE_LEFT
} resize_handle_t;

typedef enum {
	ACTION_NONE,
	ACTION_FOCUS,
	ACTION_MOVE,
	ACTION_RESIZE_SIDE,
	ACTION_RESIZE_CORNER
} pointer_action_t;

typedef enum {
	LAYOUT_TILED,
	LAYOUT_MONOCLE
} layout_t;

typedef enum {
	FLIP_HORIZONTAL,
	FLIP_VERTICAL
} flip_t;

typedef enum {
	FIRST_CHILD,
	SECOND_CHILD
} child_polarity_t;

typedef enum {
	TIGHTNESS_LOW,
	TIGHTNESS_HIGH,
} tightness_t;

typedef enum {
	AREA_BIGGEST,
	AREA_SMALLEST,
} area_peak_t;

typedef enum {
	STATE_TRANSITION_ENTER = 1 << 0,
	STATE_TRANSITION_EXIT = 1 << 1,
} state_transition_t;

typedef struct {
	option_bool_t automatic;
	option_bool_t focused;
	option_bool_t active;
	option_bool_t local;
	option_bool_t leaf;
	option_bool_t window;
	option_bool_t tiled;
	option_bool_t pseudo_tiled;
	option_bool_t floating;
	option_bool_t fullscreen;
	option_bool_t hidden;
	option_bool_t sticky;
	option_bool_t private;
	option_bool_t locked;
	option_bool_t marked;
	option_bool_t urgent;
	option_bool_t same_class;
	option_bool_t descendant_of;
	option_bool_t ancestor_of;
	option_bool_t below;
	option_bool_t normal;
	option_bool_t above;
} node_select_t;

typedef struct {
	option_bool_t occupied;
	option_bool_t focused;
	option_bool_t active;
	option_bool_t urgent;
	option_bool_t local;
} desktop_select_t;

typedef struct {
	option_bool_t occupied;
	option_bool_t focused;
} monitor_select_t;

typedef struct icccm_props_t icccm_props_t;
struct icccm_props_t {
	bool take_focus;
	bool input_hint;
	bool delete_window;
};

typedef struct {
	char class_name[3 * SMALEN / 2];
	char instance_name[3 * SMALEN / 2];
	unsigned int border_width;
	bool urgent;
	bool shown;
	client_state_t state;
	client_state_t last_state;
	stack_layer_t layer;
	stack_layer_t last_layer;
	xcb_rectangle_t floating_rectangle;
	xcb_rectangle_t tiled_rectangle;
	xcb_size_hints_t size_hints;
	icccm_props_t icccm_props;
	wm_flags_t wm_flags;
} client_t;

typedef struct presel_t presel_t;
struct presel_t {
	double split_ratio;
	direction_t split_dir;
	xcb_window_t feedback;
};

typedef struct constraints_t constraints_t;
struct constraints_t {
	uint16_t min_width;
	uint16_t min_height;
};

typedef struct node_t node_t;
struct node_t {
	uint32_t id;
	split_type_t split_type;
	double split_ratio;
	presel_t *presel;
	xcb_rectangle_t rectangle;
	constraints_t constraints;
	bool vacant;
	bool hidden;
	bool sticky;
	bool private;
	bool locked;
	bool marked;
	node_t *first_child;
	node_t *second_child;
	node_t *parent;
	client_t *client;
};

typedef struct padding_t padding_t;
struct padding_t {
	int top;
	int right;
	int bottom;
	int left;
};

typedef struct desktop_t desktop_t;
struct desktop_t {
	char name[SMALEN];
	uint32_t id;
	layout_t layout;
	layout_t user_layout;
	node_t *root;
	node_t *focus;
	desktop_t *prev;
	desktop_t *next;
	padding_t padding;
	int window_gap;
	unsigned int border_width;
};

typedef struct monitor_t monitor_t;
struct monitor_t {
	char name[SMALEN];
	uint32_t id;
	xcb_randr_output_t randr_id;
	xcb_window_t root;
	bool wired;
	padding_t padding;
	unsigned int sticky_count;
	int window_gap;
	unsigned int border_width;
	xcb_rectangle_t rectangle;
	desktop_t *desk;
	desktop_t *desk_head;
	desktop_t *desk_tail;
	monitor_t *prev;
	monitor_t *next;
};

typedef struct {
	monitor_t *monitor;
	desktop_t *desktop;
	node_t *node;
} coordinates_t;

typedef struct history_t history_t;
struct history_t {
	coordinates_t loc;
	bool latest;
	history_t *prev;
	history_t *next;
};

typedef struct stacking_list_t stacking_list_t;
struct stacking_list_t {
	node_t *node;
	stacking_list_t *prev;
	stacking_list_t *next;
};

typedef struct event_queue_t event_queue_t;
struct event_queue_t {
	xcb_generic_event_t event;
	event_queue_t *prev;
	event_queue_t *next;
};

typedef struct subscriber_list_t subscriber_list_t;
struct subscriber_list_t {
	FILE *stream;
	char* fifo_path;
	int field;
	int count;
	subscriber_list_t *prev;
	subscriber_list_t *next;
};

typedef struct rule_t rule_t;
struct rule_t {
	char class_name[MAXLEN];
	char instance_name[MAXLEN];
	char effect[MAXLEN];
	bool one_shot;
	rule_t *prev;
	rule_t *next;
};

typedef struct {
	char class_name[3 * SMALEN / 2];
	char instance_name[3 * SMALEN / 2];
	char monitor_desc[MAXLEN];
	char desktop_desc[MAXLEN];
	char node_desc[MAXLEN];
	char split_dir[SMALEN];
	double split_ratio;
	stack_layer_t *layer;
	client_state_t *state;
	bool hidden;
	bool sticky;
	bool private;
	bool locked;
	bool marked;
	bool center;
	bool follow;
	bool manage;
	bool focus;
	bool border;
	xcb_rectangle_t *rect;
} rule_consequence_t;

typedef struct pending_rule_t pending_rule_t;
struct pending_rule_t {
	int fd;
	xcb_window_t win;
	rule_consequence_t *csq;
	event_queue_t *event_head;
	event_queue_t *event_tail;
	pending_rule_t *prev;
	pending_rule_t *next;
};

#endif

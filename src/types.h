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
#include "backend.h"
#include "helpers.h"
#include "rule_match.h"

#define MISSING_VALUE        "N/A"
#define MAX_WM_STATES        4
#define MAX_CLASS_NAME_LEN   256
#define MAX_INSTANCE_NAME_LEN 256
#define MAX_RULE_EFFECT_LEN  1024


typedef enum split_type : unsigned char {
	TYPE_HORIZONTAL,
	TYPE_VERTICAL
} split_type_t;

typedef enum split_mode : unsigned char {
	MODE_AUTOMATIC,
	MODE_MANUAL
} split_mode_t;

typedef enum automatic_scheme : unsigned char {
	SCHEME_LONGEST_SIDE,
	SCHEME_ALTERNATE,
	SCHEME_SPIRAL
} automatic_scheme_t;

typedef enum honor_size_hints_mode : unsigned char {
	HONOR_SIZE_HINTS_NO = 0,
	HONOR_SIZE_HINTS_YES,
	HONOR_SIZE_HINTS_FLOATING,
	HONOR_SIZE_HINTS_TILED,
	HONOR_SIZE_HINTS_DEFAULT
} honor_size_hints_mode_t;

typedef enum client_state : unsigned char {
	STATE_TILED,
	STATE_PSEUDO_TILED,
	STATE_FLOATING,
	STATE_FULLSCREEN
} client_state_t;

typedef enum wm_flags : uint16_t {
	WM_FLAG_MODAL = 0b1,
	WM_FLAG_STICKY = 0b10,
	WM_FLAG_MAXIMIZED_VERT = 0b100,
	WM_FLAG_MAXIMIZED_HORZ = 0b1000,
	WM_FLAG_SHADED = 0b10000,
	WM_FLAG_SKIP_TASKBAR = 0b100000,
	WM_FLAG_SKIP_PAGER = 0b1000000,
	WM_FLAG_HIDDEN = 0b10000000,
	WM_FLAG_FULLSCREEN = 0b100000000,
	WM_FLAG_ABOVE = 0b1000000000,
	WM_FLAG_BELOW = 0b10000000000,
	WM_FLAG_DEMANDS_ATTENTION = 0b100000000000,
} wm_flags_t;

typedef enum stack_layer : unsigned char {
	LAYER_BELOW,
	LAYER_NORMAL,
	LAYER_ABOVE
} stack_layer_t;

typedef enum option_bool : unsigned char {
	OPTION_NONE,
	OPTION_TRUE,
	OPTION_FALSE
} option_bool_t;

typedef enum alter_state : unsigned char {
	ALTER_TOGGLE,
	ALTER_SET
} alter_state_t;

typedef enum cycle_dir : unsigned char {
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

typedef enum direction : unsigned char {
	DIR_NORTH,
	DIR_WEST,
	DIR_SOUTH,
	DIR_EAST
} direction_t;

typedef enum resize_handle : unsigned char {
	HANDLE_LEFT = 0b1,
	HANDLE_TOP = 0b10,
	HANDLE_RIGHT = 0b100,
	HANDLE_BOTTOM = 0b1000,
	HANDLE_TOP_LEFT = HANDLE_TOP | HANDLE_LEFT,
	HANDLE_TOP_RIGHT = HANDLE_TOP | HANDLE_RIGHT,
	HANDLE_BOTTOM_RIGHT = HANDLE_BOTTOM | HANDLE_RIGHT,
	HANDLE_BOTTOM_LEFT = HANDLE_BOTTOM | HANDLE_LEFT
} resize_handle_t;

typedef enum pointer_action : unsigned char {
	ACTION_NONE,
	ACTION_FOCUS,
	ACTION_MOVE,
	ACTION_RESIZE_SIDE,
	ACTION_RESIZE_CORNER
} pointer_action_t;

/* Windows-like edge snap zones */
typedef enum snap_zone : unsigned char {
	SNAP_NONE,
	SNAP_LEFT,           /* Left half */
	SNAP_RIGHT,          /* Right half */
	SNAP_TOP,            /* Top half (rarely used) */
	SNAP_BOTTOM,         /* Bottom half (rarely used) */
	SNAP_TOP_LEFT,       /* Top-left quarter */
	SNAP_TOP_RIGHT,      /* Top-right quarter */
	SNAP_BOTTOM_LEFT,    /* Bottom-left quarter */
	SNAP_BOTTOM_RIGHT,   /* Bottom-right quarter */
	SNAP_MAXIMIZE        /* Fullscreen/maximize (top edge) */
} snap_zone_t;

typedef enum layout : unsigned char {
	LAYOUT_TILED,
	LAYOUT_MONOCLE
} layout_t;

typedef enum flip : unsigned char {
	FLIP_HORIZONTAL,
	FLIP_VERTICAL
} flip_t;

typedef enum child_polarity : unsigned char {
	FIRST_CHILD,
	SECOND_CHILD
} child_polarity_t;

typedef enum tightness : unsigned char {
	TIGHTNESS_LOW,
	TIGHTNESS_HIGH,
} tightness_t;

typedef enum area_peak : unsigned char {
	AREA_BIGGEST,
	AREA_SMALLEST,
} area_peak_t;

typedef enum state_transition : unsigned char {
	STATE_TRANSITION_ENTER = 0b1,
	STATE_TRANSITION_EXIT = 0b10,
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
	option_bool_t horizontal;
	option_bool_t vertical;
} node_select_t;

typedef struct {
	option_bool_t occupied;
	option_bool_t focused;
	option_bool_t active;
	option_bool_t urgent;
	option_bool_t local;
	option_bool_t tiled;
	option_bool_t monocle;
	option_bool_t user_tiled;
	option_bool_t user_monocle;
} desktop_select_t;

typedef struct {
	option_bool_t occupied;
	option_bool_t focused;
} monitor_select_t;

/* icccm_props_t is now an alias for the backend-portable type */
typedef bspwm_icccm_props_t icccm_props_t;

typedef struct {
	/* HOT fields - first cache line (64 bytes) - accessed on every operation */
	client_state_t state;              /* 1 byte */
	client_state_t last_state;         /* 1 byte */
	stack_layer_t layer;               /* 1 byte */
	stack_layer_t last_layer;          /* 1 byte */
	unsigned int border_width;         /* 4 bytes - offset 4 */
	bspwm_rect_t floating_rectangle;   /* 8 bytes - offset 8 */
	bspwm_rect_t tiled_rectangle;      /* 8 bytes - offset 16 */
	bool urgent;                       /* 1 byte - offset 24 */
	bool shown;                        /* 1 byte - offset 25 */
	honor_size_hints_mode_t honor_size_hints; /* 1 byte - offset 26 */
	/* padding to 28, then wm_flags */
	wm_flags_t wm_flags;               /* 2 bytes - offset 28 */
	icccm_props_t icccm_props;         /* 3 bytes - offset 30 */
	/* ~33 bytes hot data in first cache line */

	/* WARM fields - size hints used during resize */
	bspwm_size_hints_t size_hints;

	/* COLD fields - only accessed during window creation/rule matching */
	char class_name[MAX_CLASS_NAME_LEN];
	char instance_name[MAX_INSTANCE_NAME_LEN];
	char name[MAXLEN];
} client_t;

#define GEOMETRY_CACHE_SIZE 32
#define GEOMETRY_CACHE_TTL_MS 100

typedef struct {
	bspwm_wid_t win;
	bspwm_rect_t geometry;
	uint64_t timestamp;
	bool valid;
} geometry_cache_entry_t;

typedef struct presel_t presel_t;
struct presel_t {
	double split_ratio;
	direction_t split_dir;
	bspwm_wid_t feedback;
};

typedef struct constraints_t constraints_t;
struct constraints_t {
	uint16_t min_width;
	uint16_t min_height;
};

typedef struct node_t node_t;
struct node_t {
	node_t *parent;
    node_t *first_child;
    node_t *second_child;
    client_t *client;
    uint32_t id;
	split_type_t split_type;
	double split_ratio;
	presel_t *presel;
	bspwm_rect_t rectangle;
	constraints_t constraints;
	bool vacant;
	bool hidden;
	bool sticky;
	bool private;
	bool locked;
	bool marked;
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
	unsigned int urgent_count;
	bool tile_limit_enabled;
	int max_tiles_per_desktop;
};

typedef struct monitor_t monitor_t;
struct monitor_t {
	char name[SMALEN];
	uint32_t id;
	bspwm_output_id_t output_id;
	bspwm_wid_t root;
	bool wired;
	padding_t padding;
	unsigned int sticky_count;
	int window_gap;
	unsigned int border_width;
	bspwm_rect_t rectangle;
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

/* Deferred event for pending rule processing.
 * Holds a copy of the raw xcb_generic_event_t. */
typedef struct event_queue_t event_queue_t;
struct event_queue_t {
	uint8_t data[64]; /* large enough for an xcb event */
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
	rule_cond_t conds[RULE_PROP_COUNT];
	/* The conditions as they were written: what `rule -l` prints, and what
	 * `rule -r` compares a cause written as conditions against. */
	char cause[3 * MAXLEN];
	char effect[MAXLEN];
	bool one_shot;
	rule_t *prev;
	rule_t *next;
};

typedef struct {
	char class_name[MAXLEN];
	char instance_name[MAXLEN];
	char name[MAXLEN];
	char monitor_desc[MAXLEN];
	char desktop_desc[MAXLEN];
	char node_desc[MAXLEN];
	direction_t *split_dir;
	double split_ratio;
	stack_layer_t *layer;
	client_state_t *state;
	honor_size_hints_mode_t honor_size_hints;
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
	bspwm_rect_t *rect;
} rule_consequence_t;

typedef struct pending_rule_t pending_rule_t;
struct pending_rule_t {
	int fd;
	bspwm_wid_t win;
	rule_consequence_t *csq;
	event_queue_t *event_head;
	event_queue_t *event_tail;
	pending_rule_t *prev;
	pending_rule_t *next;
};

#endif

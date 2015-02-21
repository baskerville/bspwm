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
#include <xcb/randr.h>
#include <xcb/xcb_event.h>
#include "helpers.h"

#define MISSING_VALUE        "N/A"
#define MAX_STATE            4

typedef enum {
	TYPE_HORIZONTAL,
	TYPE_VERTICAL
} split_type_t;

typedef enum {
	MODE_AUTOMATIC,
	MODE_MANUAL
} split_mode_t;

typedef enum {
	CLIENT_TYPE_ALL,
	CLIENT_TYPE_FLOATING,
	CLIENT_TYPE_TILED
} client_type_t;

typedef enum {
	CLIENT_CLASS_ALL,
	CLIENT_CLASS_EQUAL,
	CLIENT_CLASS_DIFFER
} client_class_t;

typedef enum {
	CLIENT_MODE_ALL,
	CLIENT_MODE_AUTOMATIC,
	CLIENT_MODE_MANUAL
} client_mode_t;

typedef struct {
	client_type_t type;
	client_class_t class;
	client_mode_t mode;
	bool urgent;
	bool local;
	bool unfocused;
} client_select_t;

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
	DIR_RIGHT,
	DIR_DOWN,
	DIR_LEFT,
	DIR_UP
} direction_t;

typedef enum {
	CORNER_TOP_LEFT,
	CORNER_TOP_RIGHT,
	CORNER_BOTTOM_RIGHT,
	CORNER_BOTTOM_LEFT
} corner_t;

typedef enum {
	SIDE_LEFT,
	SIDE_TOP,
	SIDE_RIGHT,
	SIDE_BOTTOM
} side_t;

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
	DESKTOP_STATUS_ALL,
	DESKTOP_STATUS_FREE,
	DESKTOP_STATUS_OCCUPIED
} desktop_status_t;

typedef enum {
	DESKTOP_URGENCY_ALL,
	DESKTOP_URGENCY_ON,
	DESKTOP_URGENCY_OFF
} desktop_urgency_t;

typedef enum {
	FIRST_CHILD,
	SECOND_CHILD
} child_polarity_t;

typedef struct {
	desktop_status_t status;
	bool urgent;
	bool local;
} desktop_select_t;

typedef struct {
	xcb_window_t window;
	char class_name[3 * SMALEN / 2];
	char instance_name[3 * SMALEN / 2];
	unsigned int border_width;
	bool pseudo_tiled;
	bool floating;
	bool fullscreen;
	bool locked;				/* protects window from being closed */
	bool sticky;
	bool urgent;
	bool private;
	bool icccm_focus;
	xcb_rectangle_t floating_rectangle;
	xcb_rectangle_t tiled_rectangle;
	uint16_t min_width;
	uint16_t max_width;
	uint16_t min_height;
	uint16_t max_height;
	xcb_atom_t wm_state[MAX_STATE];
	int num_states;
} client_t;

typedef struct node_t node_t;
struct node_t {
	split_type_t split_type;
	double split_ratio;
	split_mode_t split_mode;
	direction_t split_dir;
	int birth_rotation;
	xcb_rectangle_t rectangle;
	bool vacant;				/* vacant nodes only hold floating clients */
	int privacy_level;
	node_t *first_child;
	node_t *second_child;
	node_t *parent;
	client_t *client;			/* NULL except for leaves */
};

typedef struct desktop_t desktop_t;
struct desktop_t {
	char name[SMALEN];
	layout_t layout;
	node_t *root;
	node_t *focus;
	desktop_t *prev;
	desktop_t *next;
	int top_padding;
	int right_padding;
	int bottom_padding;
	int left_padding;
	int window_gap;
	unsigned int border_width;
	bool floating;
};

typedef struct monitor_t monitor_t;
struct monitor_t {
	char name[SMALEN];
	xcb_randr_output_t id;
	xcb_rectangle_t rectangle;
	xcb_window_t root;
	bool wired;
	int top_padding;
	int right_padding;
	int bottom_padding;
	int left_padding;
	desktop_t *desk;
	desktop_t *desk_head;
	desktop_t *desk_tail;
	monitor_t *prev;
	monitor_t *next;
	int num_sticky;
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

typedef struct subscriber_list_t subscriber_list_t;
struct subscriber_list_t {
	int fd;
	FILE *stream;
	subscriber_list_t *prev;
	subscriber_list_t *next;
};

typedef struct rule_t rule_t;
struct rule_t {
	char cause[MAXLEN];
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
	uint16_t min_width;
	uint16_t max_width;
	uint16_t min_height;
	uint16_t max_height;
	bool pseudo_tiled;
	bool floating;
	bool fullscreen;
	bool locked;
	bool sticky;
	bool private;
	bool center;
	bool follow;
	bool manage;
	bool focus;
	bool border;
} rule_consequence_t;

typedef struct pending_rule_t pending_rule_t;
struct pending_rule_t {
	int fd;
	xcb_window_t win;
	rule_consequence_t *csq;
	pending_rule_t *prev;
	pending_rule_t *next;
};

typedef struct {
	xcb_point_t position;
	pointer_action_t action;
	xcb_rectangle_t rectangle;
	node_t *vertical_fence;
	node_t *horizontal_fence;
	monitor_t *monitor;
	desktop_t *desktop;
	node_t *node;
	client_t *client;
	xcb_window_t window;
	bool is_tiled;
	double vertical_ratio;
	double horizontal_ratio;
	corner_t corner;
	side_t side;
} pointer_state_t;

typedef struct {
	node_t *fence;
	unsigned int distance;
} fence_distance_t;

#endif

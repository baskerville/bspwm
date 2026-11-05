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

#ifndef BSPWM_BACKEND_H
#define BSPWM_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Backend abstraction layer for the X11 display server.
 *
 * The core bspwm logic (tree, stack, rules, IPC) uses only these types
 * and functions; backend_x11.c provides the implementations.
 */

/* ------------------------------------------------------------------ */
/*  Portable geometry types (replace xcb_rectangle_t / xcb_point_t)   */
/* ------------------------------------------------------------------ */

typedef struct {
	int16_t  x, y;
	uint16_t width, height;
} bspwm_rect_t;

typedef struct {
	int16_t x, y;
} bspwm_point_t;

/* Window / surface identifier — opaque to core.
 * Maps directly to xcb_window_t (uint32_t). */
typedef uint32_t bspwm_wid_t;

/* Output (monitor) identifier — opaque to core. */
typedef uint32_t bspwm_output_id_t;

/* Null values */
#define BSPWM_WID_NONE       ((bspwm_wid_t)0)
#define BSPWM_OUTPUT_NONE    ((bspwm_output_id_t)0)

/* ------------------------------------------------------------------ */
/*  Portable modifier mask constants (match X11 values)               */
/* ------------------------------------------------------------------ */

#define BSP_MOD_MASK_SHIFT    (1 << 0)
#define BSP_MOD_MASK_LOCK     (1 << 1)
#define BSP_MOD_MASK_CONTROL  (1 << 2)
#define BSP_MOD_MASK_1        (1 << 3)
#define BSP_MOD_MASK_2        (1 << 4)
#define BSP_MOD_MASK_3        (1 << 5)
#define BSP_MOD_MASK_4        (1 << 6)
#define BSP_MOD_MASK_5        (1 << 7)

/* Portable button indices */
#define BSP_BUTTON_ANY  0
#define BSP_BUTTON_1    1
#define BSP_BUTTON_2    2
#define BSP_BUTTON_3    3

/* ------------------------------------------------------------------ */
/*  Size hints (portable replacement for xcb_size_hints_t)            */
/* ------------------------------------------------------------------ */

enum {
	BSP_SIZE_HINT_P_MIN_SIZE   = (1 << 0),
	BSP_SIZE_HINT_P_MAX_SIZE   = (1 << 1),
	BSP_SIZE_HINT_P_RESIZE_INC = (1 << 2),
	BSP_SIZE_HINT_BASE_SIZE    = (1 << 3),
	BSP_SIZE_HINT_P_ASPECT     = (1 << 4),
};

typedef struct {
	uint32_t flags;
	int32_t  min_width, min_height;
	int32_t  max_width, max_height;
	int32_t  width_inc, height_inc;
	int32_t  base_width, base_height;
	int32_t  min_aspect_num, min_aspect_den;
	int32_t  max_aspect_num, max_aspect_den;
} bspwm_size_hints_t;

/* ------------------------------------------------------------------ */
/*  ICCCM-equivalent properties (portable)                            */
/* ------------------------------------------------------------------ */

typedef struct {
	bool take_focus;
	bool input_hint;
	bool delete_window;
} bspwm_icccm_props_t;

/* Window state for set_window_state (maps to WM_STATE on X11) */
typedef enum {
	BSP_WM_STATE_WITHDRAWN = 0,
	BSP_WM_STATE_NORMAL    = 1,
	BSP_WM_STATE_ICONIC    = 3,
} bspwm_wm_state_t;

/* ------------------------------------------------------------------ */
/*  Backend output info                                               */
/* ------------------------------------------------------------------ */

typedef struct {
	char              name[32];
	bspwm_output_id_t id;
	bspwm_rect_t      rect;
	bool              primary;
} bspwm_output_info_t;

/* ------------------------------------------------------------------ */
/*  Backend lifecycle                                                 */
/* ------------------------------------------------------------------ */

/* Initialize the display server connection / compositor.
 * Returns 0 on success, -1 on failure. */
int  backend_init(int *default_screen);

/* Tear down the display server connection / compositor. */
void backend_destroy(void);

/* Get the file descriptor for epoll integration (xcb_get_file_descriptor()). */
int  backend_get_fd(void);

/* Flush pending requests to the display server. */
void backend_flush(void);

/* Poll and dispatch all pending display server events.
 * Returns false if the connection is dead. */
bool backend_dispatch_events(void);

/* Check if the connection/compositor is still alive. */
bool backend_check_connection(void);

/* ------------------------------------------------------------------ */
/*  Screen / root                                                     */
/* ------------------------------------------------------------------ */

/* Get screen dimensions. */
void backend_get_screen_size(int *width, int *height);

/* Get the root window ID. */
bspwm_wid_t backend_get_root(void);

/* ------------------------------------------------------------------ */
/*  Window management                                                 */
/* ------------------------------------------------------------------ */

/* Create an internal (non-client) window for WM use.
 * kind: "meta", "motion_recorder", "presel_feedback"
 * Returns the new window ID. */
bspwm_wid_t backend_create_internal_window(const char *kind, bspwm_rect_t rect, bool input_only);

/* Destroy a window. */
void backend_destroy_window(bspwm_wid_t win);

/* Show (map) a window. */
void backend_window_show(bspwm_wid_t win);

/* Hide (unmap) a window. */
void backend_window_hide(bspwm_wid_t win);

/* Move a window. */
void backend_window_move(bspwm_wid_t win, int16_t x, int16_t y);

/* Resize a window. */
void backend_window_resize(bspwm_wid_t win, uint16_t w, uint16_t h);

/* Move and resize a window in one call. */
void backend_window_move_resize(bspwm_wid_t win, int16_t x, int16_t y, uint16_t w, uint16_t h);

/* Set border width. */
void backend_window_set_border_width(bspwm_wid_t win, uint32_t bw);

/* Set border color (0xAARRGGBB). */
void backend_window_set_border_color(bspwm_wid_t win, uint32_t color);

/* Check if a window exists. */
bool backend_window_exists(bspwm_wid_t win);

/* Get window geometry. Returns false if unavailable. */
bool backend_window_get_geometry(bspwm_wid_t win, bspwm_rect_t *rect);

/* Enable/disable enter-notify event listening on a window. */
void backend_window_listen_enter(bspwm_wid_t win, bool enable);

/* ------------------------------------------------------------------ */
/*  Stacking order                                                    */
/* ------------------------------------------------------------------ */

/* Stack w1 above w2. */
void backend_window_stack_above(bspwm_wid_t w1, bspwm_wid_t w2);

/* Stack w1 below w2. */
void backend_window_stack_below(bspwm_wid_t w1, bspwm_wid_t w2);

/* Raise window to top. */
void backend_window_raise(bspwm_wid_t win);

/* Lower window to bottom. */
void backend_window_lower(bspwm_wid_t win);

/* ------------------------------------------------------------------ */
/*  Focus                                                             */
/* ------------------------------------------------------------------ */

/* Set input focus to a window. */
void backend_set_input_focus(bspwm_wid_t win);

/* Clear input focus (focus root / nothing). */
void backend_clear_input_focus(void);

/* ------------------------------------------------------------------ */
/*  Window properties (replaces ICCCM queries)                        */
/* ------------------------------------------------------------------ */

/* Get WM_CLASS (class_name, instance_name). */
bool backend_get_window_class(bspwm_wid_t win, char *class_name, char *instance_name, size_t len);

/* Get window title / _NET_WM_NAME. */
bool backend_get_window_name(bspwm_wid_t win, char *name, size_t len);
/* WM_WINDOW_ROLE. Returns false when unset. */
bool backend_get_window_role(bspwm_wid_t win, char *role, size_t len);

/* Get ICCCM hints (take_focus, input_hint, delete_window). */
bool backend_get_icccm_props(bspwm_wid_t win, bspwm_icccm_props_t *props);

/* Get WM_NORMAL_HINTS (size hints). */
bool backend_get_size_hints(bspwm_wid_t win, bspwm_size_hints_t *hints);

/* Get WM_TRANSIENT_FOR. Returns false if not transient. */
bool backend_get_transient_for(bspwm_wid_t win, bspwm_wid_t *transient_for);

/* Check if window has override_redirect set. */
bool backend_is_override_redirect(bspwm_wid_t win);

/* Get urgency hint. */
bool backend_get_urgency(bspwm_wid_t win);

/* Set WM_STATE property. */
void backend_set_window_state(bspwm_wid_t win, bspwm_wm_state_t state);

/* ------------------------------------------------------------------ */
/*  Window type queries (replaces EWMH _NET_WM_WINDOW_TYPE)          */
/* ------------------------------------------------------------------ */

typedef enum {
	BSP_WINDOW_TYPE_NORMAL,
	BSP_WINDOW_TYPE_DOCK,
	BSP_WINDOW_TYPE_DESKTOP,
	BSP_WINDOW_TYPE_NOTIFICATION,
	BSP_WINDOW_TYPE_DIALOG,
	BSP_WINDOW_TYPE_UTILITY,
	BSP_WINDOW_TYPE_TOOLBAR,
} bspwm_window_type_t;

bool backend_get_window_type(bspwm_wid_t win, bspwm_window_type_t *type);

/* ------------------------------------------------------------------ */
/*  Compositor state (replaces EWMH set_ functions)                   */
/* ------------------------------------------------------------------ */

void backend_ewmh_init(void);
void backend_ewmh_update_active_window(bspwm_wid_t win);
void backend_ewmh_update_number_of_desktops(uint32_t count);
void backend_ewmh_update_current_desktop(uint32_t index);
void backend_ewmh_update_desktop_names(const char *names, size_t len);
void backend_ewmh_update_desktop_viewport(void);
void backend_ewmh_set_wm_desktop(bspwm_wid_t win, uint32_t desktop);
void backend_ewmh_update_client_list(bspwm_wid_t *list, uint32_t count, bool stacking);
void backend_ewmh_wm_state_update(bspwm_wid_t win, uint16_t wm_flags);
void backend_ewmh_set_supporting(bspwm_wid_t win);
bool backend_ewmh_handle_struts(bspwm_wid_t win);
void backend_ewmh_get_struts(bspwm_wid_t win, int *top, int *right, int *bottom, int *left);

/* ------------------------------------------------------------------ */
/*  Monitor / output discovery (replaces RandR / Xinerama)            */
/* ------------------------------------------------------------------ */

/* Query available outputs. Returns count, fills array up to max.
 * Caller provides array. */
int backend_query_outputs(bspwm_output_info_t *outputs, int max);

/* Register for output change events (hotplug). */
void backend_listen_output_changes(void);

/* ------------------------------------------------------------------ */
/*  Input handling (replaces pointer grabs)                           */
/* ------------------------------------------------------------------ */

/* Initialize pointer subsystem (lock masks etc). */
void backend_pointer_init(void);

/* Grab mouse buttons on a window for click-to-focus / actions. */
void backend_grab_buttons_on_window(bspwm_wid_t win);

/* Grab / ungrab all buttons globally. */
void backend_grab_buttons(void);
void backend_ungrab_buttons(void);

/* Grab the pointer for a drag operation. Returns true on success. */
bool backend_grab_pointer(void);

/* Release the pointer after a drag. */
void backend_ungrab_pointer(void);

/* Query pointer position. Optionally returns window under pointer. */
void backend_query_pointer(bspwm_wid_t *win, bspwm_point_t *pos);

/* Warp (move) the pointer to a position. */
void backend_warp_pointer(bspwm_rect_t rect);

/* Enable/disable motion recording for focus-follows-mouse. */
void backend_enable_motion_recorder(bspwm_wid_t win);
void backend_disable_motion_recorder(void);

/* Allow/replay pointer events after a sync grab. */
void backend_allow_events(bool replay, uint32_t timestamp);

/* Get lock key modfield. */
uint16_t backend_get_lock_fields(void);

/* ------------------------------------------------------------------ */
/*  Presel feedback window helpers                                    */
/* ------------------------------------------------------------------ */

/* Create a presel feedback window with given color.
 * Sets input shape to pass-through. */
bspwm_wid_t backend_create_presel_feedback(uint32_t color);

/* ------------------------------------------------------------------ */
/*  Client message / close                                            */
/* ------------------------------------------------------------------ */

/* Kill a window forcefully (xcb_kill_client on X11). */
void backend_close_window(bspwm_wid_t win);

/* Send a graceful close request (WM_DELETE_WINDOW). */
void backend_request_close(bspwm_wid_t win);

/* Send a "take focus" message if the window supports it. */
void backend_send_take_focus(bspwm_wid_t win, bspwm_icccm_props_t *props);

/* Send a configure notify to a window (for tiled windows that didn't move). */
void backend_send_configure_notify(bspwm_wid_t win, bspwm_rect_t rect, uint32_t border_width);

/* ------------------------------------------------------------------ */
/*  Atom / property helpers                                           */
/* ------------------------------------------------------------------ */

/* Set a 32-bit property on a window (backend-specific atom). */
void backend_set_atom(bspwm_wid_t win, const char *atom_name, uint32_t value);

/* ------------------------------------------------------------------ */
/*  Color                                                             */
/* ------------------------------------------------------------------ */

/* Convert a hex color string (#RRGGBB) to a pixel value. */
uint32_t backend_get_color_pixel(const char *color);

/* ------------------------------------------------------------------ */
/*  Manage existing windows (replaces xcb_query_tree + adopt)         */
/* ------------------------------------------------------------------ */

/* Iterate over all existing top-level windows and call the callback
 * for each one. Used during startup to adopt orphan windows. */
typedef void (*backend_window_visitor_t)(bspwm_wid_t win);
void backend_enumerate_windows(backend_window_visitor_t visitor);

/* ------------------------------------------------------------------ */
/*  Display name / restart support                                    */
/* ------------------------------------------------------------------ */

/* Parse the display string for restart state path.
 * Returns false if unable to parse. */
bool backend_parse_display(char **host, int *display_num, int *screen_num);

/* Register events on the root window. Returns false if another WM
 * is running (X11) or if binding fails. */
bool backend_register_root_events(void);

/* ------------------------------------------------------------------ */
/*  Keybinding grabs                                                   */
/* ------------------------------------------------------------------ */

/* Grab all keys from the keybind table on the root window (xcb_grab_key). */
void backend_grab_keys(void);

/* Ungrab all keys from the root window. */
void backend_ungrab_keys(void);

#endif

#ifndef _TYPES_H
#define _TYPES_H

#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_event.h>
#include "helpers.h"

#define DEFAULT_DESK_NAME    "Desktop"
#define DEFAULT_MON_NAME     "Monitor"
#define MISSING_VALUE        "N/A"

typedef enum {
    TYPE_HORIZONTAL,
    TYPE_VERTICAL
} split_type_t;

typedef enum {
    MODE_AUTOMATIC,
    MODE_MANUAL
} split_mode_t;

typedef enum {
    LAYOUT_TILED,
    LAYOUT_MONOCLE
} layout_t;

typedef enum {
    MOVE_PULL,
    MOVE_PUSH
} fence_move_t;

typedef enum {
    CHANGE_INCREASE,
    CHANGE_DECREASE
} value_change_t;

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
} client_select_t;

typedef enum {
    ALTER_NONE,
    ALTER_TOGGLE,
    ALTER_SET
} state_alter_t;

typedef enum {
    DESKTOP_ALL,
    DESKTOP_FREE,
    DESKTOP_OCCUPIED
} desktop_select_t;

typedef enum {
    CYCLE_NEXT,
    CYCLE_PREV
} cycle_dir_t;

typedef enum {
    CIRCULATE_FORWARD,
    CIRCULATE_BACKWARD
} circulate_dir_t;

typedef enum {
    FLIP_HORIZONTAL,
    FLIP_VERTICAL
} flip_t;

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

typedef struct {
    xcb_window_t window;
    char class_name[MAXLEN];
    unsigned int border_width;
    bool floating;
    bool transient;  /* transient window are always floating */
    bool fullscreen;
    bool locked;     /* protects window from being closed */
    bool urgent;
    xcb_rectangle_t floating_rectangle;
    xcb_rectangle_t tiled_rectangle;
} client_t;

typedef struct node_t node_t;
struct node_t {
    split_type_t split_type;
    double split_ratio;
    split_mode_t split_mode;
    direction_t split_dir;
    int birth_rotation;
    xcb_rectangle_t rectangle;
    bool vacant;          /* vacant nodes only hold floating clients */
    node_t *first_child;
    node_t *second_child;
    node_t *parent;
    client_t *client;     /* NULL except for leaves */
};

typedef struct node_list_t node_list_t;
struct node_list_t {
    node_t *node;
    bool latest;          /* used for z-ordering tiled windows */
    node_list_t *prev;
    node_list_t *next;
};

typedef struct {
    node_list_t *head;
    node_list_t *tail;
} focus_history_t;

typedef struct desktop_t desktop_t;
struct desktop_t {
    char name[MAXLEN];
    layout_t layout;
    node_t *root;
    node_t *focus;
    focus_history_t *history;
    desktop_t *prev;
    desktop_t *next;
};

typedef struct monitor_t monitor_t;
struct monitor_t {
    char name[MAXLEN];
    xcb_randr_output_t id;
    xcb_rectangle_t rectangle;
    bool wired;
    int top_padding;
    int right_padding;
    int bottom_padding;
    int left_padding;
    desktop_t *desk;
    desktop_t *last_desk;
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

typedef struct {
    char name[MAXLEN];
} rule_cause_t;

typedef struct {
    bool floating;
    bool follow;
    char desc[MAXLEN];
} rule_effect_t;

typedef struct rule_t rule_t;
struct rule_t {
    unsigned int uid;
    rule_cause_t cause;
    rule_effect_t effect;
    rule_t *prev;
    rule_t *next;
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

node_t *make_node(void);
monitor_t *make_monitor(xcb_rectangle_t *);
monitor_t *find_monitor(char *);
monitor_t *get_monitor_by_id(xcb_randr_output_t);
monitor_t *add_monitor(xcb_rectangle_t *);
void remove_monitor(monitor_t *);
void merge_monitors(monitor_t *, monitor_t *);
desktop_t *make_desktop(const char *);
void insert_desktop(monitor_t *, desktop_t *);
void add_desktop(monitor_t *, desktop_t *);
void empty_desktop(desktop_t *);
void unlink_desktop(monitor_t *, desktop_t *);
void remove_desktop(monitor_t *, desktop_t *);
void transfer_desktop(monitor_t *, monitor_t *, desktop_t *);
rule_t *make_rule(void);
pointer_state_t *make_pointer_state(void);
client_t *make_client(xcb_window_t);
focus_history_t *make_focus_history(void);
node_list_t *make_node_list(void);
void history_add(focus_history_t *, node_t *);
void history_remove(focus_history_t *, node_t *);
void empty_history(focus_history_t *);
node_t *history_get(focus_history_t *, int);
int history_rank(focus_history_t *, node_t *);

#endif

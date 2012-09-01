#ifndef _TYPES_H
#define _TYPES_H

#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "helpers.h"

#define SPLIT_RATIO  0.5
#define DESK_NAME    "One"

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
    SKIP_NONE,
    SKIP_FLOATING,
    SKIP_TILED
} skip_client_t;

typedef enum {
    DIR_NEXT,
    DIR_PREV
} cycle_dir_t;

typedef enum {
    ROTATE_CLOCK_WISE,
    ROTATE_COUNTER_CW,
    ROTATE_FULL_CYCLE
} rotate_t;

typedef enum {
    DIR_LEFT,
    DIR_UP,
    DIR_RIGHT,
    DIR_DOWN
} direction_t;

typedef struct {
    xcb_window_t window;
    bool floating;
    bool fullscreen;
    bool locked;
} client_t;

typedef struct node_t node_t;
struct node_t {
    split_type_t split_type;
    double split_ratio;
    xcb_rectangle_t rectangle;
    bool vacant; /* vacant nodes only hold floating clients */
    node_t *first_child;
    node_t *second_child;
    node_t *parent;
    /* the value of the following properties is NULL except for leaves: */
    client_t *client; 
    node_t *prev_leaf;
    node_t *next_leaf;
};

typedef struct desktop_t desktop_t;
struct desktop_t {
    char *name;
    layout_t layout;
    node_t *root;
    node_t *view; /* initially view = root, can be changed by zooming */
    node_t *focus;
    node_t *last_focus;
    node_t *head; /* first element in the list of leaves */
    node_t *tail;
    desktop_t *prev;
    desktop_t *next;
};

typedef struct rule_t rule_t;
struct rule_t {
    char *class_name;
    char *win_title;
    bool floating;
    bool fullscreen;
    bool locked;
    rule_t *next;
};

node_t *make_node(void);
desktop_t *make_desktop(void);
client_t *make_client(void);

#endif

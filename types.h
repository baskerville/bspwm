#ifndef _TYPES_H
#define _TYPES_H

#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "helpers.h"

typedef enum {
    TYPE_HORIZONTAL,
    TYPE_VERTICAL
} split_type_t;

typedef enum {
    MODE_AUTOMATIC,
    MODE_MANUAL
} split_mode_t;

typedef enum {
    MOVE_PULL,
    MOVE_PUSH
} fence_move_t;

typedef enum {
    CHANGE_INCREASE,
    CHANGE_DECREASE
} value_change_t;

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
    bool maximized;
    bool fullscreen;
    bool locked;
} Client;

typedef struct Node Node;
struct Node {
    split_type_t split_type;
    double split_ratio;
    xcb_rectangle_t rectangle;
    bool vacant; /* vacant nodes only hold floating clients */
    Node *first_child;
    Node *second_child;
    Node *parent;
    /* the value of the following properties is NULL except for leaves: */
    Client *client; 
    Node *prev_leaf;
    Node *next_leaf;
};

typedef struct Rule Rule;
struct Rule {
    char *class_name;
    char *desk_name;
    bool floating;
    bool maximized;
    bool fullscreen;
    bool locked;
    Rule *next;
};

typedef struct Desktop Desktop;
struct Desktop {
    char *name;
    Node *root;
    Node *view; /* initially view = root, can be changed by zooming */
    Node *focus;
    Node *last_focus;
    Node *head; /* first element in the list of leaves */
    Node *tail;
    Desktop *prev;
    Desktop *next;
};

Node *make_node(void);
Desktop *make_desktop(void);

#endif

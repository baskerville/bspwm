#ifndef _TYPES_H
#define _TYPES_H

#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "utils.h"

typedef enum {
    LAYOUT_TILED,
    LAYOUT_MAX
} layout_t;

typedef enum {
    LAYER_TILING,
    LAYER_FLOATING
} layer_t;

typedef enum {
    TYPE_HORIZONTAL,
    TYPE_VERTICAL
} split_type_t;

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
    xcb_window_t win;
    bool floating;
    bool fullscreen;
    bool urgent;
    bool locked;
} Client;

typedef struct Node Node;
struct Node {
    split_type_t split_type;
    double split_ratio;
    xcb_rectangle_t rectangle;
    Node *first_child;
    Node *second_child;
    Node *parent;
    Client *client; /* equals NULL except for leaves */
};

typedef struct NodeFocusHistory NodeFocusHistory;
struct NodeFocusHistory {
    Node *node;
    NodeFocusHistory *prev;
};

typedef struct {
    Node *head;
    Node *focus;
    NodeFocusHistory *focus_history;
} Layer;

typedef Layer TilingLayer;
typedef Layer FloatingLayer;

typedef struct Desktop Desktop;
struct Desktop {
    char *name;
    Layer tiling_layer;
    Layer floating_layer;
    layer_t selected_layer;
    layout_t tiling_layout;
    Desktop *previous;
    Desktop *next;
};

Node *make_node(void);
Desktop *make_desktop(void);

#endif

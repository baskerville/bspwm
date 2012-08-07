#ifndef _TYPES_H
#define _TYPES_H

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
    ROTATE_CWISE,
    ROTATE_CCWISE
} tree_rotate_t;

typedef enum {
    DIR_LEFT,
    DIR_UP,
    DIR_RIGHT,
    DIR_DOWN
} pair_dir_t;

typedef struct {
    xcb_window_t win;
    bool floating;
    bool fullscreen;
    bool urgent;
    bool locked;
} Client;

typedef struct {
    split_type_t split_type;
    double split_ratio;
    xcb_rectangle_t rectangle;
    struct Node *first_child;
    struct Node *second_child;
    struct Node *parent;
    Client *client; /* equals NULL except for leaves */
} Node;

typedef struct {
    Node *node;
    struct NodeFocusHistory *prev;
} NodeFocusHistory;

typedef struct {
    Node *head;
    Node *focus;
    NodeFocusHistory *focus_history;
} Layer;

typedef Layer TilingLayer;
typedef Layer FloatingLayer;

typedef struct {
    char *name;
    Layer tiling_layer;
    Layer floating_layer;
    layer_t selected_layer;
    layout_t tiling_layout;
    struct Desktop *previous;
    struct Desktop *next;
} Desktop;

#endif

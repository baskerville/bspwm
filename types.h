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
    MODE_AUTOMATIC,
    MODE_MANUAL
} split_mode_t;

typedef enum {
    STRATEGY_REPLACE,
    STRATEGY_PAIR
} insertion_strategy_t;

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
    split_mode_t split_mode;
    insertion_strategy_t insertion_strategy;
    xcb_rectangle_t rectangle;
    struct Node *first_child;
    struct Node *second_child;
    struct Node *parent;
    Client *client; /* equals NULL except for leaves */
} Node;

typedef struct {
    Node *prev;
} NodeFocusHistory;

typedef struct {
    Client *prev;
} ClientFocusHistory;

typedef struct {
    Node *tree;
    Node *focus;
    NodeFocusHistory *focus_history;
} TilingLayer;

typedef struct {
    Client *clients;
    Client *focus;
    ClientFocusHistory *focus_history;
} FloatingLayer;

typedef struct {
    char *name;
    layout_t layout;
    layer_t focus;
    TilingLayer tiling_layer;
    FloatingLayer floating_layer;
    struct Desktop *previous;
    struct Desktop *next;
} Desktop;

#endif

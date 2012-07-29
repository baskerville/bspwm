typedef enum {
    false,
    true
} bool;

typedef enum {
    LAYOUT_TILED,
    LAYOUT_MAX
} layout_t;

typedef enum {
    LAYER_TILING,
    LAYER_FLOATING
} layer_t;

typedef enum {
    SPLIT_TYPE_HORIZONTAL,
    SPLIT_TYPE_VERTICAL
} splittype_t;

typedef enum {
    SPLIT_MODE_AUTOMATIC,
    SPLIT_MODE_MANUAL
} splitmode_t;

typedef struct {
    xcb_window_t win;
    bool floating;
    bool fullscreen;
    bool urgent;
    bool locked;
} Client;

typedef struct {
    splittype_t split_type;
    double split_ratio;
    splitmode_t split_mode;
    xcb_rectangle_t rectangle;
    struct Node *first_child;
    struct Node *second_child;
    struct Node *parent;
    Client *client; /* equals NULL except for leaves */
} Node;

typedef struct {
    Node *tree;
    Node *focus;
} TilingLayer;

typedef struct {
    Client *clients;
    Client *focus;
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

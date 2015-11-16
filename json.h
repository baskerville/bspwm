#include <jansson.h>
#include "subscribe.h"

#define SERIALIZE(TYPE) \
json_t* json_serialize_##TYPE##_type(TYPE##_t *obj);

#define DESERIALIZE(TYPE) \
TYPE##_t* json_deserialize_##TYPE##_type(json_t *json);

#define SERIALIZATION(TYPE) SERIALIZE(TYPE) DESERIALIZE(TYPE)

// Enums
SERIALIZATION(split_type)
SERIALIZATION(split_mode)
SERIALIZATION(client_state)
SERIALIZATION(stack_layer)
SERIALIZATION(option_bool)
SERIALIZATION(alter_state)
SERIALIZATION(cycle_dir)
SERIALIZATION(circulate_dir)
SERIALIZATION(history_dir)
SERIALIZATION(direction)
SERIALIZATION(corner)
SERIALIZATION(side)
SERIALIZATION(pointer_action)
SERIALIZATION(layout)
SERIALIZATION(flip)
SERIALIZATION(child_polarity)
SERIALIZATION(subscriber_mask)

// Structs
SERIALIZATION(xcb_rectangle)
SERIALIZATION(client)
SERIALIZATION(node)
SERIALIZATION(desktop)
SERIALIZATION(monitor)
SERIALIZATION(coordinates)

#undef SERIALIZE
#undef DESERALIZE
#undef SERIALIZATION

// Misc
json_t* json_serialize_node_window(node_t *obj);
node_t* json_deserialize_node_window(json_t *json);

json_t* json_serialize_desktop_name(desktop_t *obj);
desktop_t* json_deserialize_desktop_name(json_t *json);

json_t* json_serialize_monitor_name(monitor_t *obj);
monitor_t* json_deserialize_monitor_name(json_t *json);

json_t* json_serialize_status_node(monitor_t *m, desktop_t *d, node_t *n);
json_t* json_serialize_status_node_nullable(monitor_t *m, desktop_t *d, node_t *n);
json_t* json_serialize_status_node_transfer(monitor_t *ms, desktop_t *ds, monitor_t *md, desktop_t *dd, node_t *nd);
json_t* json_serialize_status_node_swap(monitor_t *m1, desktop_t *d1, node_t *n1, monitor_t *m2, desktop_t *d2, node_t *n2);
json_t* json_serialize_status_desktop(monitor_t *m, desktop_t *d);
json_t* json_serialize_status_desktop_transfer(monitor_t *ms, monitor_t *md, desktop_t *d);
json_t* json_serialize_status_desktop_rename(monitor_t *m, desktop_t *d, const char *name_last);
json_t* json_serialize_status_desktop_swap(monitor_t *m1, desktop_t *d1, monitor_t *m2, desktop_t *d2);
json_t* json_serialize_status_monitor_rename(monitor_t *m, const char *name_last);
json_t* json_serialize_status_monitor_swap(monitor_t *m1, monitor_t *m2);

json_t* json_serialize_windows(coordinates_t loc);
json_t* json_serialize_desktops(coordinates_t loc);
json_t* json_serialize_monitors(coordinates_t loc);
json_t* json_serialize_tree(coordinates_t loc);
json_t* json_serialize_history(coordinates_t loc);
json_t* json_serialize_stack();

json_t* json_deserialize_file(const char *file_path);

#include <jansson.h>

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
json_t* json_serialize_node_windowid(node_t *obj);
node_t* json_deserialize_node_windowid(json_t *json);
json_t* json_serialize_desktop_name(desktop_t *obj);
desktop_t* json_deserialize_desktop_name(json_t *json);
json_t* json_serialize_monitor_name(monitor_t *obj);
monitor_t* json_deserialize_monitor_name(json_t *json);
json_t* json_serialize_monitor_id(monitor_t *obj);
json_t* json_serialize_desktops_array(monitor_t *m);
json_t* json_serialize_windows(coordinates_t loc);
json_t* json_serialize_desktops(coordinates_t loc);
json_t* json_serialize_monitors(coordinates_t loc);
json_t* json_serialize_tree(coordinates_t loc);
json_t* json_serialize_history(coordinates_t loc);
json_t* json_serialize_stack();

json_t* json_deserialize_file(const char *file_path);

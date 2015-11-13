#include <jansson.h>

#define SERIALIZE(type) \
json_t* json_serialize_##type(type##_t *obj)

#define DESERIALIZE(type) \
bool json_deserialize_##type(json_t *json, type##_t *obj)

SERIALIZE(split_type);
DESERIALIZE(split_type);
SERIALIZE(split_mode);
DESERIALIZE(split_mode);
SERIALIZE(client_state);
DESERIALIZE(client_state);
SERIALIZE(stack_layer);
DESERIALIZE(stack_layer);
SERIALIZE(option_bool);
DESERIALIZE(option_bool);
SERIALIZE(alter_state);
DESERIALIZE(alter_state);
SERIALIZE(cycle_dir);
DESERIALIZE(cycle_dir);
SERIALIZE(circulate_dir);
DESERIALIZE(circulate_dir);
SERIALIZE(history_dir);
DESERIALIZE(history_dir);
SERIALIZE(direction);
DESERIALIZE(direction);
SERIALIZE(corner);
DESERIALIZE(corner);
SERIALIZE(side);
DESERIALIZE(side);
SERIALIZE(pointer_action);
DESERIALIZE(pointer_action);
SERIALIZE(layout);
DESERIALIZE(layout);
SERIALIZE(flip);
DESERIALIZE(flip);
SERIALIZE(child_polarity);
DESERIALIZE(child_polarity);

#undef SERIALIZE
#undef DESERALIZE

json_t* json_serialize_xcb_rectangle(xcb_rectangle_t rec);
json_t* json_serialize_client(client_t *c);
json_t* json_serialize_node(node_t *n);
json_t* json_serialize_desktop(desktop_t *d);
json_t* json_serialize_monitor(monitor_t *m);

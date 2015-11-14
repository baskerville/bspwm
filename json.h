#include <jansson.h>

#define SERIALIZE(type) \
json_t* json_serialize_##type(type##_t *obj);

#define DESERIALIZE(type) \
type##_t* json_deserialize_##type(json_t *json);

#define SERIALIZATION(type) SERIALIZE(type) DESERIALIZE(type)

//Enums
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

//Structs
SERIALIZATION(xcb_rectangle)
SERIALIZATION(client)
SERIALIZATION(node)
SERIALIZATION(desktop)
SERIALIZATION(monitor)

#undef SERIALIZE
#undef DESERALIZE
#undef SERIALIZATION

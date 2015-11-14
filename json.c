#include <string.h>
#include <jansson.h>
#include "types.h"
#include "tree.h"
#include "query.h"

// Add more if __VA_ARGS__ > 25
#define FE_1(WHAT, X) WHAT(X)
#define FE_2(WHAT, X, ...) WHAT(X)FE_1(WHAT, __VA_ARGS__)
#define FE_3(WHAT, X, ...) WHAT(X)FE_2(WHAT, __VA_ARGS__)
#define FE_4(WHAT, X, ...) WHAT(X)FE_3(WHAT, __VA_ARGS__)
#define FE_5(WHAT, X, ...) WHAT(X)FE_4(WHAT, __VA_ARGS__)
#define FE_6(WHAT, X, ...) WHAT(X)FE_5(WHAT, __VA_ARGS__)
#define FE_7(WHAT, X, ...) WHAT(X)FE_6(WHAT, __VA_ARGS__)
#define FE_8(WHAT, X, ...) WHAT(X)FE_7(WHAT, __VA_ARGS__)
#define FE_9(WHAT, X, ...) WHAT(X)FE_8(WHAT, __VA_ARGS__)
#define FE_10(WHAT, X, ...) WHAT(X)FE_9(WHAT, __VA_ARGS__)
#define FE_11(WHAT, X, ...) WHAT(X)FE_10(WHAT, __VA_ARGS__)
#define FE_12(WHAT, X, ...) WHAT(X)FE_11(WHAT, __VA_ARGS__)
#define FE_13(WHAT, X, ...) WHAT(X)FE_12(WHAT, __VA_ARGS__)
#define FE_14(WHAT, X, ...) WHAT(X)FE_13(WHAT, __VA_ARGS__)
#define FE_15(WHAT, X, ...) WHAT(X)FE_14(WHAT, __VA_ARGS__)
#define FE_16(WHAT, X, ...) WHAT(X)FE_15(WHAT, __VA_ARGS__)
#define FE_17(WHAT, X, ...) WHAT(X)FE_16(WHAT, __VA_ARGS__)
#define FE_18(WHAT, X, ...) WHAT(X)FE_17(WHAT, __VA_ARGS__)
#define FE_19(WHAT, X, ...) WHAT(X)FE_18(WHAT, __VA_ARGS__)
#define FE_20(WHAT, X, ...) WHAT(X)FE_19(WHAT, __VA_ARGS__)
#define FE_21(WHAT, X, ...) WHAT(X)FE_20(WHAT, __VA_ARGS__)
#define FE_22(WHAT, X, ...) WHAT(X)FE_21(WHAT, __VA_ARGS__)
#define FE_23(WHAT, X, ...) WHAT(X)FE_22(WHAT, __VA_ARGS__)
#define FE_24(WHAT, X, ...) WHAT(X)FE_23(WHAT, __VA_ARGS__)
#define FE_25(WHAT, X, ...) WHAT(X)FE_24(WHAT, __VA_ARGS__)
#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, NAME, ...) NAME
#define FOR_EACH(ACTION, ...) \
	GET_MACRO(__VA_ARGS__, FE_25, FE_24, FE_23, FE_22, FE_21, FE_20, FE_19, FE_18, FE_17, FE_16, FE_15, FE_14, FE_13, FE_12, FE_11, FE_10, FE_9, FE_8, FE_7, FE_6, FE_5, FE_4, FE_3, FE_2, FE_1)(ACTION, __VA_ARGS__)

// Enums
#define SERIALIZE_BEGIN(TYPE) \
	json_t* json_serialize_##TYPE##_type(TYPE##_t *obj) \
	{ \
		if (obj == NULL) \
			return json_null();
#define SERIALIZE_IF(ENUM, VALUE) \
		else if (*obj == ENUM) \
			return json_string(VALUE);
#define SERIALIZE_END \
		else \
			return json_null(); \
	}

#define DESERIALIZE_BEGIN(TYPE) \
	TYPE##_t* json_deserialize_##TYPE##_type(json_t *json) \
	{ \
		if (json == NULL || !json_is_string(json)) \
			return NULL; \
		TYPE##_t *obj = malloc(sizeof(TYPE##_t)); \
		const char* value = json_string_value(json); \
		if (value == NULL) \
			obj = NULL;
#define DESERIALIZE_IF(ENUM, VALUE) \
		else if (strcmp(value, VALUE) == 0) \
			*obj = ENUM;
#define DESERIALIZE_END \
		else \
			obj = NULL; \
		return obj; \
	}

#define SERIALIZE_CAT(X) SERIALIZE_##X
#define DESERIALIZE_CAT(X) DESERIALIZE_##X
#define SERIALIZATION(TYPE, ...) \
	SERIALIZE_BEGIN(TYPE) \
	FOR_EACH(SERIALIZE_CAT, __VA_ARGS__) \
	SERIALIZE_END \
	DESERIALIZE_BEGIN(TYPE) \
	FOR_EACH(DESERIALIZE_CAT, __VA_ARGS__) \
	DESERIALIZE_END

SERIALIZATION(split_type,
	IF(TYPE_HORIZONTAL, "horizontal"),
	IF(TYPE_VERTICAL, "vertical")
)

SERIALIZATION(split_mode,
	IF(MODE_AUTOMATIC, "automatic"),
	IF(MODE_MANUAL, "manual")
)

SERIALIZATION(client_state,
	IF(STATE_TILED, "tiled"),
	IF(STATE_PSEUDO_TILED, "pseudo_tiled"),
	IF(STATE_FLOATING, "floating"),
	IF(STATE_FULLSCREEN, "fullscreen")
)

SERIALIZATION(stack_layer,
	IF(LAYER_BELOW, "below"),
	IF(LAYER_NORMAL, "normal"),
	IF(LAYER_ABOVE, "above")
)

SERIALIZATION(option_bool,
	IF(OPTION_NONE, "none"),
	IF(OPTION_TRUE, "true"),
	IF(OPTION_FALSE, "false")
)

SERIALIZATION(alter_state,
	IF(ALTER_TOGGLE, "toggle"),
	IF(ALTER_SET, "set")
)

SERIALIZATION(cycle_dir,
	IF(CYCLE_NEXT, "next"),
	IF(CYCLE_PREV, "prev")
)

SERIALIZATION(circulate_dir,
	IF(CIRCULATE_FORWARD, "forward"),
	IF(CIRCULATE_BACKWARD, "backward")
)

SERIALIZATION(history_dir,
	IF(HISTORY_OLDER, "older"),
	IF(HISTORY_NEWER, "newer")
)

SERIALIZATION(direction,
	IF(DIR_RIGHT, "right"),
	IF(DIR_DOWN, "down"),
	IF(DIR_LEFT, "left"),
	IF(DIR_UP, "up")
)

SERIALIZATION(corner,
	IF(CORNER_TOP_LEFT, "top_left"),
	IF(CORNER_TOP_RIGHT, "top_right"),
	IF(CORNER_BOTTOM_RIGHT, "bottom_right"),
	IF(CORNER_BOTTOM_LEFT, "bottom_left")
)

SERIALIZATION(side,
	IF(SIDE_LEFT, "left"),
	IF(SIDE_TOP, "top"),
	IF(SIDE_RIGHT, "right"),
	IF(SIDE_BOTTOM, "bottom")
)

SERIALIZATION(pointer_action,
	IF(ACTION_NONE, "none"),
	IF(ACTION_FOCUS, "focus"),
	IF(ACTION_MOVE, "move"),
	IF(ACTION_RESIZE_SIDE, "resize_side"),
	IF(ACTION_RESIZE_CORNER, "resize_corner")
)

SERIALIZATION(layout,
	IF(LAYOUT_TILED, "tiled"),
	IF(LAYOUT_MONOCLE, "monocle")
)

SERIALIZATION(flip,
	IF(FLIP_HORIZONTAL, "horizontal"),
	IF(FLIP_VERTICAL, "vertical")
)

SERIALIZATION(child_polarity,
	IF(FIRST_CHILD, "first"),
	IF(SECOND_CHILD, "second")
)

#undef SERIALIZE_BEGIN
#undef SERIALIZE_IF
#undef SERIALIZE_END

#undef DESERIALIZE_BEGIN
#undef DESERIALIZE_IF
#undef DESERIALIZE_END

//Structs
#define SERIALIZE_BEGIN(TYPE) \
	json_t* json_serialize_##TYPE##_type(TYPE##_t *obj) \
	{ \
		if (obj == NULL) \
			return json_null(); \
		json_t *json = json_object();
#define SERIALIZE_INTEGER(KEY, TYPE, MEMBER) \
		if((json_object_set_new(json, KEY, json_integer(*(MEMBER)))) == -1) { \
			json_decref(json); \
			return json_null(); \
		}
#define SERIALIZE_REAL(KEY, TYPE, MEMBER) \
		if((json_object_set_new(json, KEY, json_real(*(MEMBER)))) == -1) { \
			json_decref(json); \
			return json_null(); \
		}
#define SERIALIZE_STRING(KEY, MEMBER) \
		if((json_object_set_new(json, KEY, json_string(*MEMBER))) == -1) { \
			json_decref(json); \
			return json_null(); \
		}
#define SERIALIZE_BOOLEAN(KEY, MEMBER) \
		if((json_object_set_new(json, KEY, json_boolean(*MEMBER))) == -1) { \
			json_decref(json); \
			return json_null(); \
		}
#define SERIALIZE_OBJECT(KEY, FUNCTION, MEMBER) \
		if((json_object_set_new(json, KEY, json_serialize_##FUNCTION(MEMBER))) == -1) { \
			json_decref(json); \
			return json_null(); \
		}
#define SERIALIZE_CUSTOM(KEY, FUNCTION, MEMBER) \
		if((json_object_set_new(json, KEY, json_serialize_##FUNCTION(MEMBER))) == -1) { \
			json_decref(json); \
			return json_null(); \
		}
#define SERIALIZE_END \
		return json; \
	}
#define SERIALIZE_SERONLY(WHAT) WHAT

#define DESERIALIZE_BEGIN(TYPE) \
	TYPE##_t* json_deserialize_##TYPE##_type(json_t *json) \
	{ \
		TYPE##_t *obj = malloc(sizeof(TYPE##_t)); \
		json_t *get;
#define DESERIALIZE_INTEGER(KEY, TYPE, MEMBER) \
		get = json_object_get(json, KEY); \
		if (get == NULL || !json_is_integer(get)) { \
			json_decref(get); \
			free(obj); \
			return NULL; \
		}\
		*MEMBER = (TYPE)json_number_value(get); \
		json_decref(get);
#define DESERIALIZE_REAL(KEY, TYPE, MEMBER) \
		get = json_object_get(json, KEY); \
		if (get == NULL || !json_is_real(get)) { \
			json_decref(get); \
			free(obj); \
			return NULL; \
		}\
		*MEMBER = (TYPE)json_real_value(get); \
		json_decref(get);
#define DESERIALIZE_STRING(KEY, MEMBER) \
		get = json_object_get(json, KEY); \
		if (get == NULL || !json_is_string(get)) { \
			json_decref(get); \
			free(obj); \
			return NULL; \
		} \
		strcpy(*MEMBER, json_string_value(get)); \
		json_decref(get);
#define DESERIALIZE_BOOLEAN(KEY, MEMBER) \
		get = json_object_get(json, KEY); \
		if (get == NULL || !json_is_boolean(get)) { \
			json_decref(get); \
			free(obj); \
			return NULL; \
		}\
		*MEMBER = json_boolean_value(get); \
		json_decref(get);
#define DESERIALIZE_OBJECT(KEY, FUNCTION, MEMBER) \
		get = json_object_get(json, KEY); \
		if (get == NULL || !json_is_object(get)) { \
			json_decref(get); \
			free(obj); \
			return NULL; \
		} \
		*MEMBER = *json_deserialize_##FUNCTION(get); \
		json_decref(get);
#define DESERIALIZE_CUSTOM(KEY, FUNCTION, MEMBER) \
		get = json_object_get(json, KEY); \
		if (get == NULL) { \
			json_decref(get); \
			free(obj); \
			return NULL; \
		} \
		MEMBER = json_deserialize_##FUNCTION(get); \
		json_decref(get);
#define DESERIALIZE_END \
		return obj; \
	}
#define DESERIALIZE_SERONLY(WHAT)

SERIALIZATION(xcb_rectangle,
	INTEGER("x", int16_t, &obj->x),
	INTEGER("y", int16_t, &obj->y),
	INTEGER("height", uint16_t, &obj->height),
	INTEGER("width", uint16_t, &obj->width)
)

SERIALIZATION(client,
	INTEGER("windowId", xcb_window_t, &obj->window),
	STRING("nameClass", &obj->class_name),
	STRING("nameInstance", &obj->instance_name),
	INTEGER("borderWidth", unsigned int, &obj->border_width),
	BOOLEAN("locked", &obj->locked),
	BOOLEAN("sticky", &obj->sticky),
	BOOLEAN("urgent", &obj->urgent),
	BOOLEAN("private", &obj->private),
	BOOLEAN("icccmFocus", &obj->icccm_focus),
	BOOLEAN("icccmInput", &obj->icccm_input),
	OBJECT("state", client_state_type, &obj->state),
	OBJECT("stateLast", client_state_type, &obj->last_state),
	OBJECT("layer", stack_layer_type, &obj->layer),
	OBJECT("layerLast", stack_layer_type, &obj->last_layer),
	OBJECT("rectangleFloating", xcb_rectangle_type, &obj->floating_rectangle),
	OBJECT("rectangleTiled", xcb_rectangle_type, &obj->tiled_rectangle),
	INTEGER("minWidth", uint16_t, &obj->min_width),
	INTEGER("maxWidth", uint16_t, &obj->max_width),
	INTEGER("minHeight", uint16_t, &obj->min_height),
	INTEGER("maxHeight", uint16_t, &obj->max_height),
	// wm_state
	INTEGER("statesNumber", int, &obj->num_states)
)

SERIALIZATION(node,
	OBJECT("type", split_type_type, &obj->split_type),
	REAL("ratio", double, &obj->split_ratio),
	OBJECT("mode", split_mode_type, &obj->split_mode),
	OBJECT("direction", direction_type, &obj->split_dir),
	INTEGER("birthRotation", int, &obj->birth_rotation),
	OBJECT("rectangle", xcb_rectangle_type, &obj->rectangle),
	BOOLEAN("vacant", &obj->vacant),
	INTEGER("privacyLevel", int, &obj->privacy_level),
	OBJECT("childFirst", node_type, obj->first_child),
	OBJECT("childSecond", node_type, obj->second_child),
	// parent
	OBJECT("client", client_type, obj->client)
)

json_t* json_serialize_desktop_name(desktop_t *obj)
{
	if (obj == NULL)
		return json_null();
	return json_string(obj->name);
}

desktop_t* json_deserialize_desktop_name(json_t *json)
{
	if (json == NULL || !json_is_string(json))
		return NULL;
	coordinates_t loc;
	if (!locate_desktop(json_string_value(json), &loc))
		return NULL;
	return loc.desktop;
}

SERIALIZATION(desktop,
	STRING("name", &obj->name),
	OBJECT("layout", layout_type, &obj->layout),
	// root
	// focus
	SERONLY(SERIALIZE_CUSTOM("prevName", desktop_name, obj->prev)),
	SERONLY(SERIALIZE_CUSTOM("nextName", desktop_name, obj->next)),
	INTEGER("paddingTop", int, &obj->top_padding),
	INTEGER("paddingRight", int, &obj->right_padding),
	INTEGER("paddingBottom", int, &obj->bottom_padding),
	INTEGER("paddingLeft", int, &obj->left_padding),
	INTEGER("windowGap", int, &obj->window_gap),
	INTEGER("borderWidth", unsigned int, &obj->border_width)
)

json_t* json_serialize_monitor_name(monitor_t *obj)
{
	if (obj == NULL)
		return json_null();
	return json_string(obj->name);
}

monitor_t* json_deserialize_monitor_name(json_t *json)
{
	if (json == NULL || !json_is_string(json))
		return NULL;
	coordinates_t loc;
	if (!locate_monitor(json_string_value(json), &loc))
		return NULL;
	return loc.monitor;
}

json_t* json_serialize_monitor_id(monitor_t *obj)
{
	if (obj == NULL)
		return json_null();
	return json_integer(obj->id);
}

SERIALIZATION(monitor,
	STRING("name", &obj->name),
	INTEGER("id", xcb_randr_output_t, &obj->id),
	OBJECT("rectangle", xcb_rectangle_type, &obj->rectangle),
	INTEGER("rootWindowId", xcb_window_t, &obj->root),
	BOOLEAN("wired", &obj->wired),
	INTEGER("paddingTop", int, &obj->top_padding),
	INTEGER("paddingRight", int, &obj->right_padding),
	INTEGER("paddingBottom", int, &obj->bottom_padding),
	INTEGER("paddingLeft", int, &obj->left_padding),
	CUSTOM("desktopFocusedName", desktop_name, obj->desk),
	CUSTOM("desktopHeadName", desktop_name, obj->desk_head),
	CUSTOM("desktopTailName", desktop_name, obj->desk_tail),
	CUSTOM("prevName", monitor_name, obj->prev),
	SERONLY(SERIALIZE_CUSTOM("prevId", monitor_id, obj->prev)),
	CUSTOM("nextName", monitor_name, obj->next),
	SERONLY(SERIALIZE_CUSTOM("nextId", monitor_id, obj->next)),
	INTEGER("stickyNumber", int, &obj->num_sticky)
)

#undef SERIALIZE_BEGIN
#undef SERIALIZE_INTEGER
#undef SERIALIZE_REAL
#undef SERIALIZE_STRING
#undef SERIALIZE_BOOLEAN
#undef SERIALIZE_OBJECT
#undef SERIALIZE_END
#undef SERIALIZE_SERONLY

#undef DESERIALIZE_BEGIN
#undef DESERIALIZE_INTEGER
#undef DESERIALIZE_REAL
#undef DESERIALIZE_STRING
#undef DESERIALIZE_BOOLEAN
#undef DESERIALIZE_OBJECT
#undef DESERIALIZE_END
#undef DESERIALIZE_SERONLY

#include <string.h>
#include <jansson.h>
#include "types.h"
#include "tree.h"

#define SERIALIZE_BEGIN(type) \
json_t* json_serialize_##type(type##_t *obj) { \
	switch (*obj) {
#define SERIALIZE_CASE(enumerator, match) \
		case enumerator: \
			return json_string(match);
#define SERIALIZE_END \
		default: \
			return json_null(); \
	} \
}

#define DESERIALIZE_BEGIN(type) \
bool json_deserialize_##type(json_t *json, type##_t *obj) { \
	const char* value = json_string_value(json); \
	if (value == NULL) \
		return false;
#define DESERIALIZE_IF(enumerator, match) \
	if(strcmp(value, match) == 0) { \
		*obj = enumerator; \
		return true; \
	}
#define DESERIALIZE_ELIF(enumerator, match) \
	else if(strcmp(value, match) == 0) { \
		*obj = enumerator; \
		return true; \
	}
#define DESERIALIZE_END \
	else \
		return false; \
}

SERIALIZE_BEGIN(split_type)
SERIALIZE_CASE(TYPE_HORIZONTAL, "horizontal")
SERIALIZE_CASE(TYPE_VERTICAL, "vertical")
SERIALIZE_END

DESERIALIZE_BEGIN(split_type)
DESERIALIZE_IF(TYPE_HORIZONTAL, "horizontal")
DESERIALIZE_ELIF(TYPE_VERTICAL, "vertical")
DESERIALIZE_END

SERIALIZE_BEGIN(split_mode)
SERIALIZE_CASE(MODE_AUTOMATIC, "automatic")
SERIALIZE_CASE(MODE_MANUAL, "manual")
SERIALIZE_END

DESERIALIZE_BEGIN(split_mode)
DESERIALIZE_IF(MODE_AUTOMATIC, "automatic")
DESERIALIZE_ELIF(MODE_MANUAL, "manual")
DESERIALIZE_END

SERIALIZE_BEGIN(client_state)
SERIALIZE_CASE(STATE_TILED, "tiled")
SERIALIZE_CASE(STATE_PSEUDO_TILED, "pseudo_tiled")
SERIALIZE_CASE(STATE_FLOATING, "floating")
SERIALIZE_CASE(STATE_FULLSCREEN, "fullscreen")
SERIALIZE_END

DESERIALIZE_BEGIN(client_state)
DESERIALIZE_IF(STATE_TILED, "tiled")
DESERIALIZE_ELIF(STATE_PSEUDO_TILED, "pseudo_tiled")
DESERIALIZE_ELIF(STATE_FLOATING, "floating")
DESERIALIZE_ELIF(STATE_FULLSCREEN, "fullscreen")
DESERIALIZE_END

SERIALIZE_BEGIN(stack_layer)
SERIALIZE_CASE(LAYER_BELOW, "below")
SERIALIZE_CASE(LAYER_NORMAL, "normal")
SERIALIZE_CASE(LAYER_ABOVE, "above")
SERIALIZE_END

DESERIALIZE_BEGIN(stack_layer)
DESERIALIZE_IF(LAYER_BELOW, "below")
DESERIALIZE_ELIF(LAYER_NORMAL, "normal")
DESERIALIZE_ELIF(LAYER_ABOVE, "above")
DESERIALIZE_END

SERIALIZE_BEGIN(option_bool)
SERIALIZE_CASE(OPTION_NONE, "none")
SERIALIZE_CASE(OPTION_TRUE, "true")
SERIALIZE_CASE(OPTION_FALSE, "false")
SERIALIZE_END

DESERIALIZE_BEGIN(option_bool)
DESERIALIZE_IF(OPTION_NONE, "none")
DESERIALIZE_ELIF(OPTION_TRUE, "true")
DESERIALIZE_ELIF(OPTION_FALSE, "false")
DESERIALIZE_END

SERIALIZE_BEGIN(alter_state)
SERIALIZE_CASE(ALTER_TOGGLE, "toggle")
SERIALIZE_CASE(ALTER_SET, "set")
SERIALIZE_END

DESERIALIZE_BEGIN(alter_state)
DESERIALIZE_IF(ALTER_TOGGLE, "toggle")
DESERIALIZE_ELIF(ALTER_SET, "set")
DESERIALIZE_END

SERIALIZE_BEGIN(cycle_dir)
SERIALIZE_CASE(CYCLE_NEXT, "next")
SERIALIZE_CASE(CYCLE_PREV, "prev")
SERIALIZE_END

DESERIALIZE_BEGIN(cycle_dir)
DESERIALIZE_IF(CYCLE_NEXT, "next")
DESERIALIZE_ELIF(CYCLE_PREV, "prev")
DESERIALIZE_END

SERIALIZE_BEGIN(circulate_dir)
SERIALIZE_CASE(CIRCULATE_FORWARD, "forward")
SERIALIZE_CASE(CIRCULATE_BACKWARD, "backward")
SERIALIZE_END

DESERIALIZE_BEGIN(circulate_dir)
DESERIALIZE_IF(CIRCULATE_FORWARD, "forward")
DESERIALIZE_ELIF(CIRCULATE_BACKWARD, "backward")
DESERIALIZE_END

SERIALIZE_BEGIN(history_dir)
SERIALIZE_CASE(HISTORY_OLDER, "older")
SERIALIZE_CASE(HISTORY_NEWER, "newer")
SERIALIZE_END

DESERIALIZE_BEGIN(history_dir)
DESERIALIZE_IF(HISTORY_OLDER, "older")
DESERIALIZE_ELIF(HISTORY_NEWER, "newer")
DESERIALIZE_END

SERIALIZE_BEGIN(direction)
SERIALIZE_CASE(DIR_RIGHT, "right")
SERIALIZE_CASE(DIR_DOWN, "down")
SERIALIZE_CASE(DIR_LEFT, "left")
SERIALIZE_CASE(DIR_UP, "up")
SERIALIZE_END

DESERIALIZE_BEGIN(direction)
DESERIALIZE_IF(DIR_RIGHT, "right")
DESERIALIZE_ELIF(DIR_DOWN, "down")
DESERIALIZE_ELIF(DIR_LEFT, "left")
DESERIALIZE_ELIF(DIR_UP, "up")
DESERIALIZE_END

SERIALIZE_BEGIN(corner)
SERIALIZE_CASE(CORNER_TOP_LEFT, "top_left")
SERIALIZE_CASE(CORNER_TOP_RIGHT, "top_right")
SERIALIZE_CASE(CORNER_BOTTOM_RIGHT, "bottom_right")
SERIALIZE_CASE(CORNER_BOTTOM_LEFT, "bottom_left")
SERIALIZE_END

DESERIALIZE_BEGIN(corner)
DESERIALIZE_IF(CORNER_TOP_LEFT, "top_left")
DESERIALIZE_ELIF(CORNER_TOP_RIGHT, "top_right")
DESERIALIZE_ELIF(CORNER_BOTTOM_RIGHT, "bottom_right")
DESERIALIZE_ELIF(CORNER_BOTTOM_LEFT, "bottom_left")
DESERIALIZE_END

SERIALIZE_BEGIN(side)
SERIALIZE_CASE(SIDE_LEFT, "left")
SERIALIZE_CASE(SIDE_TOP, "top")
SERIALIZE_CASE(SIDE_RIGHT, "right")
SERIALIZE_CASE(SIDE_BOTTOM, "bottom")
SERIALIZE_END

DESERIALIZE_BEGIN(side)
DESERIALIZE_IF(SIDE_LEFT, "left")
DESERIALIZE_ELIF(SIDE_TOP, "top")
DESERIALIZE_ELIF(SIDE_RIGHT, "right")
DESERIALIZE_ELIF(SIDE_BOTTOM, "bottom")
DESERIALIZE_END

SERIALIZE_BEGIN(pointer_action)
SERIALIZE_CASE(ACTION_NONE, "none")
SERIALIZE_CASE(ACTION_FOCUS, "focus")
SERIALIZE_CASE(ACTION_MOVE, "move")
SERIALIZE_CASE(ACTION_RESIZE_SIDE, "resize_side")
SERIALIZE_CASE(ACTION_RESIZE_CORNER, "resize_corner")
SERIALIZE_END

DESERIALIZE_BEGIN(pointer_action)
DESERIALIZE_IF(ACTION_NONE, "none")
DESERIALIZE_ELIF(ACTION_FOCUS, "focus")
DESERIALIZE_ELIF(ACTION_MOVE, "move")
DESERIALIZE_ELIF(ACTION_RESIZE_SIDE, "resize_side")
DESERIALIZE_ELIF(ACTION_RESIZE_CORNER, "resize_corner")
DESERIALIZE_END

SERIALIZE_BEGIN(layout)
SERIALIZE_CASE(LAYOUT_TILED, "tiled")
SERIALIZE_CASE(LAYOUT_MONOCLE, "monocle")
SERIALIZE_END

DESERIALIZE_BEGIN(layout)
DESERIALIZE_IF(LAYOUT_TILED, "tiled")
DESERIALIZE_ELIF(LAYOUT_MONOCLE, "monocle")
DESERIALIZE_END

SERIALIZE_BEGIN(flip)
SERIALIZE_CASE(FLIP_HORIZONTAL, "horizontal")
SERIALIZE_CASE(FLIP_VERTICAL, "vertical")
SERIALIZE_END

DESERIALIZE_BEGIN(flip)
DESERIALIZE_IF(FLIP_HORIZONTAL, "horizontal")
DESERIALIZE_ELIF(FLIP_VERTICAL, "vertical")
DESERIALIZE_END

SERIALIZE_BEGIN(child_polarity)
SERIALIZE_CASE(FIRST_CHILD, "first")
SERIALIZE_CASE(SECOND_CHILD, "second")
SERIALIZE_END

DESERIALIZE_BEGIN(child_polarity)
DESERIALIZE_IF(FIRST_CHILD, "first")
DESERIALIZE_ELIF(SECOND_CHILD, "second")
DESERIALIZE_END

#undef SERIALIZE_BEGIN
#undef SERIALIZE_CASE
#undef SERIALIZE_END

#undef DESERIALIZE_BEGIN
#undef DESERIALIZE_IF

json_t* json_serialize_xcb_rectangle(xcb_rectangle_t *rec)
{
	return json_pack(
		"{"
			"s:i,"
			"s:i,"
			"s:i,"
			"s:i"
		"}",
			"x", rec->x,
			"y", rec->y,
			"width", rec->width,
			"height", rec->height
	);
}

json_t* json_serialize_client(client_t *c)
{
	return json_pack(
		"{"
			"s:i,"
			"s:s,"
			"s:s,"
			"s:i,"
			"s:b,"
			"s:b,"
			"s:b,"
			"s:b,"
			"s:b,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:{"
				"s:i,"
				"s:i,"
			"},"
			"s:{"
				"s:i,"
				"s:i,"
			"},"
			"s:i"
		"}",
			"windowId", c->window,
			"nameClass", c->class_name,
			"nameInstance", c->instance_name,
			"borderWidth", c->border_width,
			"locked", c->locked,
			"sticky", c->sticky,
			"urgent", c->urgent,
			"private", c->private,
			"icccmFocus", c->icccm_focus,
			"state", json_serialize_client_state(&c->state),
			"stateLast", json_serialize_client_state(&c->last_state),
			"layer", json_serialize_stack_layer(&c->layer),
			"layerLast", json_serialize_stack_layer(&c->last_layer),
			"rectangleFloating", json_serialize_xcb_rectangle(&c->floating_rectangle),
			"rectangleTiled", json_serialize_xcb_rectangle(&c->tiled_rectangle),
			"sizeMinimum",
				"width", c->min_width,
				"height", c->min_height,
			"sizeMaximum",
				"width", c->max_width,
				"height", c->max_height,
			"stateNumber", c->num_states
	);
}

json_t* json_serialize_node(node_t *n)
{
	return json_pack(
		"{"
			"s:{"
				"s:o,"
				"s:f,"
				"s:o,"
				"s:o,"
			"},"
			"s:i,"
			"s:o,"
			"s:b,"
			"s:i,"
			"s:o,"
			"s:o,"
			"s:o"
		"}",
			"split",
				"type", json_serialize_split_type(&n->split_type),
				"ratio", n->split_ratio,
				"mode", json_serialize_split_mode(&n->split_mode),
				"direction", json_serialize_direction(&n->split_dir),
			"birthRotation", n->birth_rotation,
			"rectangle", json_serialize_xcb_rectangle(&n->rectangle),
			"vacant", n->vacant,
			"privacyLevel", n->privacy_level,
			"childFirst", n->first_child != NULL ? json_serialize_node(n->first_child) : json_null(),
			"childSecond", n->second_child != NULL ? json_serialize_node(n->second_child) : json_null(),
			"client", is_leaf(n) ? json_serialize_client(n->client) : json_null()
	);
}

json_t* json_serialize_desktop(desktop_t *d)
{
	return json_pack(
		"{"
			"s:s,"
			"s:o,"
			"s:o,"
			"s:i,"
			"s:i,"
			"s:{"
				"s:i,"
				"s:i,"
				"s:i,"
				"s:i"
			"}"
		"}",
			"name", d->name,
			"layout", json_serialize_layout(&d->layout),
			"nodes", d->root != NULL ? json_serialize_node(d->root) : json_null(),
			"borderWidth", d->border_width,
			"windowGap", d->window_gap,
			"padding",
				"top", d->top_padding,
				"right", d->right_padding,
				"bottom", d->bottom_padding,
				"left", d->left_padding
	);
}

json_t* json_serialize_monitor(monitor_t *m)
{
	return json_pack(
		"{"
			"s:s,"
			"s:i,"
			"s:o,"
			"s:i,"
			"s:b,"
			"s:{"
				"s:i,"
				"s:i,"
				"s:i,"
				"s:i"
			"},"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:i"
		"}",
			"name", m->name,
			"id", m->id,
			"rectangle", json_serialize_xcb_rectangle(&m->rectangle),
			"rootWindowId", m->root,
			"wired", m->wired,
			"padding",
				"top", m->top_padding,
				"right", m->right_padding,
				"bottom", m->bottom_padding,
				"left", m->left_padding,
			"desktopFocused", m->desk != NULL ? json_string(m->desk->name) : json_null() ,
			"desktopHead", m->desk_head != NULL ? json_string(m->desk_head->name) : json_null(),
			"desktopTail", m->desk_tail != NULL ? json_string(m->desk_tail->name) : json_null(),
			"prevName", m->prev != NULL ? json_string(m->prev->name) : json_null(),
			"prevId", m->prev != NULL ? json_integer(m->prev->id) : json_null(),
			"nextName", m->next != NULL ? json_string(m->next->name) : json_null(),
			"nextId", m->next != NULL ? json_integer(m->next->id) : json_null(),
			"stickyNumber", m->num_sticky
	);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "parse.h"

bool parse_bool(char *value, bool *b)
{
	if (streq("true", value) || streq("on", value)) {
		*b = true;
		return true;
	} else if (streq("false", value) || streq("off", value)) {
		*b = false;
		return true;
	}
	return false;
}

bool parse_split_type(char *s, split_type_t *t)
{
	if (streq("horizontal", s)) {
		*t = TYPE_HORIZONTAL;
		return true;
	} else if (streq("vertical", s)) {
		*t = TYPE_VERTICAL;
		return true;
	}
	return false;
}

bool parse_split_mode(char *s, split_mode_t *m)
{
	if (streq("automatic", s)) {
		*m = MODE_AUTOMATIC;
		return true;
	} else if (streq("vertical", s)) {
		*m = MODE_MANUAL;
		return true;
	}
	return false;
}

bool parse_layout(char *s, layout_t *l)
{
	if (streq("monocle", s)) {
		*l = LAYOUT_MONOCLE;
		return true;
	} else if (streq("tiled", s)) {
		*l = LAYOUT_TILED;
		return true;
	}
	return false;
}

bool parse_client_state(char *s, client_state_t *t)
{
	if (streq("tiled", s)) {
		*t = STATE_TILED;
		return true;
	} else if (streq("pseudo_tiled", s)) {
		*t = STATE_PSEUDO_TILED;
		return true;
	} else if (streq("floating", s)) {
		*t = STATE_FLOATING;
		return true;
	} else if (streq("fullscreen", s)) {
		*t = STATE_FULLSCREEN;
		return true;
	}
	return false;
}

bool parse_stack_layer(char *s, stack_layer_t *l)
{
	if (streq("below", s)) {
		*l = LAYER_BELOW;
		return true;
	} else if (streq("normal", s)) {
		*l = LAYER_NORMAL;
		return true;
	} else if (streq("above", s)) {
		*l = LAYER_ABOVE;
		return true;
	}
	return false;
}

bool parse_direction(char *s, direction_t *d)
{
	if (streq("north", s)) {
		*d = DIR_NORTH;
		return true;
	} else if (streq("west", s)) {
		*d = DIR_WEST;
		return true;
	} else if (streq("south", s)) {
		*d = DIR_SOUTH;
		return true;
	} else if (streq("east", s)) {
		*d = DIR_EAST;
		return true;
	}
	return false;
}

bool parse_cycle_direction(char *s, cycle_dir_t *d)
{
	if (streq("next", s)) {
		*d = CYCLE_NEXT;
		return true;
	} else if (streq("prev", s)) {
		*d = CYCLE_PREV;
		return true;
	}
	return false;
}

bool parse_circulate_direction(char *s, circulate_dir_t *d)
{
	if (streq("forward", s)) {
		*d = CIRCULATE_FORWARD;
		return true;
	} else if (streq("backward", s)) {
		*d = CIRCULATE_BACKWARD;
		return true;
	}
	return false;
}

bool parse_history_direction(char *s, history_dir_t *d)
{
	if (streq("older", s)) {
		*d = HISTORY_OLDER;
		return true;
	} else if (streq("newer", s)) {
		*d = HISTORY_NEWER;
		return true;
	}
	return false;
}


bool parse_flip(char *s, flip_t *f)
{
	if (streq("horizontal", s)) {
		*f = FLIP_HORIZONTAL;
		return true;
	} else if (streq("vertical", s)) {
		*f = FLIP_VERTICAL;
		return true;
	}
	return false;
}

bool parse_resize_handle(char *s, resize_handle_t *h)
{
	if (streq("left", s)) {
		*h = HANDLE_LEFT;
		return true;
	} else if (streq("top", s)) {
		*h = HANDLE_TOP;
		return true;
	} else if (streq("right", s)) {
		*h = HANDLE_RIGHT;
		return true;
	} else if (streq("bottom", s)) {
		*h = HANDLE_BOTTOM;
		return true;
	} else if (streq("top_left", s)) {
		*h = HANDLE_TOP_LEFT;
		return true;
	} else if (streq("top_right", s)) {
		*h = HANDLE_TOP_RIGHT;
		return true;
	} else if (streq("bottom_right", s)) {
		*h = HANDLE_BOTTOM_RIGHT;
		return true;
	} else if (streq("bottom_left", s)) {
		*h = HANDLE_BOTTOM_LEFT;
		return true;
	}
	return false;
}

bool parse_modifier_mask(char *s, uint16_t *m)
{
	if (strcmp(s, "shift") == 0) {
		*m = XCB_MOD_MASK_SHIFT;
		return true;
	} else if (strcmp(s, "control") == 0) {
		*m = XCB_MOD_MASK_CONTROL;
		return true;
	} else if (strcmp(s, "lock") == 0) {
		*m = XCB_MOD_MASK_LOCK;
		return true;
	} else if (strcmp(s, "mod1") == 0) {
		*m = XCB_MOD_MASK_1;
		return true;
	} else if (strcmp(s, "mod2") == 0) {
		*m = XCB_MOD_MASK_2;
		return true;
	} else if (strcmp(s, "mod3") == 0) {
		*m = XCB_MOD_MASK_3;
		return true;
	} else if (strcmp(s, "mod4") == 0) {
		*m = XCB_MOD_MASK_4;
		return true;
	} else if (strcmp(s, "mod5") == 0) {
		*m = XCB_MOD_MASK_5;
		return true;
	}
	return false;
}

bool parse_button_index(char *s, int8_t *b)
{
	if (strcmp(s, "any") == 0) {
		*b = XCB_BUTTON_INDEX_ANY;
		return true;
	} else if (strcmp(s, "button1") == 0) {
		*b = XCB_BUTTON_INDEX_1;
		return true;
	} else if (strcmp(s, "button2") == 0) {
		*b = XCB_BUTTON_INDEX_2;
		return true;
	} else if (strcmp(s, "button3") == 0) {
		*b = XCB_BUTTON_INDEX_3;
		return true;
	} else if (strcmp(s, "none") == 0) {
		*b = -1;
		return true;
	}
	return false;
}

bool parse_pointer_action(char *s, pointer_action_t *a)
{
	if (streq("move", s)) {
		*a = ACTION_MOVE;
		return true;
	} else if (streq("resize_corner", s)) {
		*a = ACTION_RESIZE_CORNER;
		return true;
	} else if (streq("resize_side", s)) {
		*a = ACTION_RESIZE_SIDE;
		return true;
	} else if (streq("focus", s)) {
		*a = ACTION_FOCUS;
		return true;
	} else if (streq("none", s)) {
		*a = ACTION_NONE;
		return true;
	}
	return false;
}

bool parse_child_polarity(char *s, child_polarity_t *p)
{
	if (streq("first_child", s)) {
		*p = FIRST_CHILD;
		return true;
	} else if (streq("second_child", s)) {
		*p = SECOND_CHILD;
		return true;
	}
	return false;
}

bool parse_automatic_scheme(char *s, automatic_scheme_t *a)
{
	if (streq("longest_side", s)) {
		*a = SCHEME_LONGEST_SIDE;
		return true;
	} else if (streq("alternate", s)) {
		*a = SCHEME_ALTERNATE;
		return true;
	} else if (streq("spiral", s)) {
		*a = SCHEME_SPIRAL;
		return true;
	}
	return false;
}

bool parse_state_transition(char *s, state_transition_t *m)
{
	if (streq("none", s)) {
		*m = 0;
		return true;
	} else if (streq("all", s)) {
		*m = STATE_TRANSITION_ENTER | STATE_TRANSITION_EXIT;
		return true;
	} else {
		state_transition_t w = 0;
		char *x = copy_string(s, strlen(s));
		char *key = strtok(x, ",");
		while (key != NULL) {
			if (streq("enter", key)) {
				w |= STATE_TRANSITION_ENTER;
			} else if (streq("exit", key)) {
				w |= STATE_TRANSITION_EXIT;
			} else {
				free(x);
				return false;
			}
			key = strtok(NULL, ",");
		}
		free(x);
		if (w != 0) {
			*m = w;
			return true;
		} else {
			return false;
		}
	}
	return false;
}

bool parse_tightness(char *s, tightness_t *t)
{
	if (streq("high", s)) {
		*t = TIGHTNESS_HIGH;
		return true;
	} else if (streq("low", s)) {
		*t = TIGHTNESS_LOW;
		return true;
	}
	return false;
}

bool parse_degree(char *s, int *d)
{
	int i = atoi(s);
	while (i < 0)
		i += 360;
	while (i > 359)
		i -= 360;
	if ((i % 90) != 0) {
		return false;
	} else {
		*d = i;
		return true;
	}
}

bool parse_id(char *s, uint32_t *id)
{
	char *end;
	errno = 0;
	uint32_t v = strtol(s, &end, 0);
	if (errno != 0 || *end != '\0') {
		return false;
	}
	*id = v;
	return true;
}

bool parse_bool_declaration(char *s, char **key, bool *value, alter_state_t *state)
{
	*key = strtok(s, EQL_TOK);
	char *v = strtok(NULL, EQL_TOK);
	if (v == NULL) {
		*state = ALTER_TOGGLE;
		return true;
	} else {
		if (parse_bool(v, value)) {
			*state = ALTER_SET;
			return true;
		} else {
			return false;
		}
	}
	return false;
}

bool parse_index(char *s, uint16_t *idx)
{
	return (sscanf(s, "^%hu", idx) == 1);
}

bool parse_rectangle(char *s, xcb_rectangle_t *r)
{
	uint16_t w, h;
	int16_t x, y;
	if (sscanf(s, "%hux%hu+%hi+%hi", &w, &h, &x, &y) != 4) {
		return false;
	}
	r->width = w;
	r->height = h;
	r->x = x;
	r->y = y;
	return true;
}

bool parse_subscriber_mask(char *s, subscriber_mask_t *mask)
{
	if (streq("all", s)) {
		*mask = SBSC_MASK_ALL;
	} else if (streq("node", s)) {
		*mask = SBSC_MASK_NODE;
	} else if (streq("desktop", s)) {
		*mask = SBSC_MASK_DESKTOP;
	} else if (streq("monitor", s)) {
		*mask = SBSC_MASK_MONITOR;
	} else if (streq("pointer_action", s)) {
		*mask = SBSC_MASK_POINTER_ACTION;
	} else if (streq("node_add", s)) {
		*mask = SBSC_MASK_NODE_ADD;
	} else if (streq("node_remove", s)) {
		*mask = SBSC_MASK_NODE_REMOVE;
	} else if (streq("node_swap", s)) {
		*mask = SBSC_MASK_NODE_SWAP;
	} else if (streq("node_transfer", s)) {
		*mask = SBSC_MASK_NODE_TRANSFER;
	} else if (streq("node_focus", s)) {
		*mask = SBSC_MASK_NODE_FOCUS;
	} else if (streq("node_presel", s)) {
		*mask = SBSC_MASK_NODE_PRESEL;
	} else if (streq("node_stack", s)) {
		*mask = SBSC_MASK_NODE_STACK;
	} else if (streq("node_activate", s)) {
		*mask = SBSC_MASK_NODE_ACTIVATE;
	} else if (streq("node_geometry", s)) {
		*mask = SBSC_MASK_NODE_GEOMETRY;
	} else if (streq("node_state", s)) {
		*mask = SBSC_MASK_NODE_STATE;
	} else if (streq("node_flag", s)) {
		*mask = SBSC_MASK_NODE_FLAG;
	} else if (streq("node_layer", s)) {
		*mask = SBSC_MASK_NODE_LAYER;
	} else if (streq("desktop_add", s)) {
		*mask = SBSC_MASK_DESKTOP_ADD;
	} else if (streq("desktop_rename", s)) {
		*mask = SBSC_MASK_DESKTOP_RENAME;
	} else if (streq("desktop_remove", s)) {
		*mask = SBSC_MASK_DESKTOP_REMOVE;
	} else if (streq("desktop_swap", s)) {
		*mask = SBSC_MASK_DESKTOP_SWAP;
	} else if (streq("desktop_transfer", s)) {
		*mask = SBSC_MASK_DESKTOP_TRANSFER;
	} else if (streq("desktop_focus", s)) {
		*mask = SBSC_MASK_DESKTOP_FOCUS;
	} else if (streq("desktop_activate", s)) {
		*mask = SBSC_MASK_DESKTOP_ACTIVATE;
	} else if (streq("desktop_layout", s)) {
		*mask = SBSC_MASK_DESKTOP_LAYOUT;
	} else if (streq("monitor_add", s)) {
		*mask = SBSC_MASK_MONITOR_ADD;
	} else if (streq("monitor_rename", s)) {
		*mask = SBSC_MASK_MONITOR_RENAME;
	} else if (streq("monitor_remove", s)) {
		*mask = SBSC_MASK_MONITOR_REMOVE;
	} else if (streq("monitor_swap", s)) {
		*mask = SBSC_MASK_MONITOR_SWAP;
	} else if (streq("monitor_focus", s)) {
		*mask = SBSC_MASK_MONITOR_FOCUS;
	} else if (streq("monitor_geometry", s)) {
		*mask = SBSC_MASK_MONITOR_GEOMETRY;
	} else if (streq("report", s)) {
		*mask = SBSC_MASK_REPORT;
	} else {
		return false;
	}
	return true;
}


#define GET_MOD(k) \
	} else if (streq(#k, tok)) { \
		sel->k = OPTION_TRUE; \
	} else if (streq("!" #k, tok)) { \
		sel->k = OPTION_FALSE;

bool parse_monitor_modifiers(char *desc, monitor_select_t *sel)
{
	char *tok;
	while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
		tok[0] = '\0';
		tok++;
		if (streq("occupied", tok)) {
			sel->occupied = OPTION_TRUE;
		} else if (streq("!occupied", tok)) {
			sel->occupied = OPTION_FALSE;
		GET_MOD(focused)
		} else {
			return false;
		}
	}
	return true;
}

bool parse_desktop_modifiers(char *desc, desktop_select_t *sel)
{
	char *tok;
	while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
		tok[0] = '\0';
		tok++;
		if (streq("occupied", tok)) {
			sel->occupied = OPTION_TRUE;
		} else if (streq("!occupied", tok)) {
			sel->occupied = OPTION_FALSE;
		GET_MOD(focused)
		GET_MOD(active)
		GET_MOD(urgent)
		GET_MOD(local)
		GET_MOD(tiled)
		GET_MOD(monocle)
		GET_MOD(user_tiled)
		GET_MOD(user_monocle)
		} else {
			return false;
		}
	}
	return true;

}

bool parse_node_modifiers(char *desc, node_select_t *sel)
{
	char *tok;
	while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
		tok[0] = '\0';
		tok++;
		if (streq("tiled", tok)) {
			sel->tiled = OPTION_TRUE;
		} else if (streq("!tiled", tok)) {
			sel->tiled = OPTION_FALSE;
		GET_MOD(automatic)
		GET_MOD(focused)
		GET_MOD(active)
		GET_MOD(local)
		GET_MOD(leaf)
		GET_MOD(window)
		GET_MOD(pseudo_tiled)
		GET_MOD(floating)
		GET_MOD(fullscreen)
		GET_MOD(hidden)
		GET_MOD(sticky)
		GET_MOD(private)
		GET_MOD(locked)
		GET_MOD(marked)
		GET_MOD(urgent)
		GET_MOD(same_class)
		GET_MOD(descendant_of)
		GET_MOD(ancestor_of)
		GET_MOD(below)
		GET_MOD(normal)
		GET_MOD(above)
		GET_MOD(horizontal)
		GET_MOD(vertical)
		} else {
			return false;
		}
	}
	return true;
}

#undef GET_MOD

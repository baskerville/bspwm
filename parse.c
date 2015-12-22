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

bool parse_id(char *s, uint32_t *i)
{
	char *end;
	errno = 0;
	uint32_t ret = strtol(s, &end, 0);
	if (errno != 0 || *end != '\0') {
		return false;
	} else {
		*i = ret;
	}
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

bool parse_index(char *s, int *i)
{
	int idx;
	if (sscanf(s, "^%i", &idx) != 1 || idx < 1) {
		return false;
	}
	*i = idx;
	return true;
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
	} else if (streq("node_manage", s)) {
		*mask = SBSC_MASK_NODE_MANAGE;
	} else if (streq("node_unmanage", s)) {
		*mask = SBSC_MASK_NODE_UNMANAGE;
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
		GET_MOD(urgent)
		GET_MOD(local)
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
		GET_MOD(local)
		GET_MOD(leaf)
		GET_MOD(pseudo_tiled)
		GET_MOD(floating)
		GET_MOD(fullscreen)
		GET_MOD(locked)
		GET_MOD(sticky)
		GET_MOD(private)
		GET_MOD(urgent)
		GET_MOD(same_class)
		GET_MOD(below)
		GET_MOD(normal)
		GET_MOD(above)
		} else {
			return false;
		}
	}
	return true;
}

#undef GET_MOD

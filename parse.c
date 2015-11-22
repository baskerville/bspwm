#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "helpers.h"
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
	if (streq("right", s)) {
		*d = DIR_RIGHT;
		return true;
	} else if (streq("down", s)) {
		*d = DIR_DOWN;
		return true;
	} else if (streq("left", s)) {
		*d = DIR_LEFT;
		return true;
	} else if (streq("up", s)) {
		*d = DIR_UP;
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

bool parse_window_id(char *s, long int *i)
{
	char *end;
	errno = 0;
	long int ret = strtol(s, &end, 0);
	if (errno != 0 || *end != '\0')
		return false;
	else
		*i = ret;
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
	if (sscanf(s, "^%i", &idx) != 1 || idx < 1)
		return false;
	*i = idx;
	return true;
}

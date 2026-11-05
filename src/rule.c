/* Copyright (c) 2012, Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>
#include "bspwm.h"
#include "backend_x11.h"
#include "ewmh.h"
#include "window.h"
#include "query.h"
#include "parse.h"
#include "settings.h"
#include "rule.h"

rule_t *make_rule(void)
{
	rule_t *r = calloc(1, sizeof(rule_t));
	if (r == NULL) {
		return NULL;
	}
	r->cause[0] = r->effect[0] = '\0';
	r->next = r->prev = NULL;
	r->one_shot = false;
	return r;
}

void add_rule(rule_t *r)
 {
	if (rule_head == NULL) {
		rule_head = rule_tail = r;
	} else {
		rule_tail->next = r;
		r->prev = rule_tail;
		rule_tail = r;
	}
}

void remove_rule(rule_t *r)
{
	if (r == NULL) {
		return;
	}
	rule_t *prev = r->prev;
	rule_t *next = r->next;
	if (prev != NULL) {
		prev->next = next;
	}
	if (next != NULL) {
		next->prev = prev;
	}
	if (r == rule_head) {
		rule_head = next;
	}
	if (r == rule_tail) {
		rule_tail = prev;
	}
	for (int i = 0; i < RULE_PROP_COUNT; i++) {
		rule_cond_free(&r->conds[i]);
	}
	free(r);
}

void remove_rule_by_cause(char *cause)
{
    if (cause == NULL || strlen(cause) >= MAXLEN * 3) {
        return;
    }

    /* A rule written as conditions has no CLASS:INSTANCE:NAME to take
     * apart: it is removed by the very text `rule -l` prints for it,
     * compared whole, so a rule with one more condition or a different
     * operator is a different rule and stays. `*:*:*` is not written as
     * conditions and still goes the old way below, removing every rule. */
    if (rule_is_key_value(cause)) {
        rule_t *r = rule_head;
        while (r != NULL) {
            rule_t *next = r->next;
            if (streq(r->cause, cause)) {
                remove_rule(r);
            }
            r = next;
        }
        return;
    }

    rule_t *r = rule_head;
    struct tokenize_state state;
    char *class_name = tokenize_with_escape(&state, cause, COL_TOK[0]);
    char *instance_name = tokenize_with_escape(&state, NULL, COL_TOK[0]);
    char *name = tokenize_with_escape(&state, NULL, COL_TOK[0]);

    if (class_name && strnlen(class_name, MAXLEN) >= MAXLEN) {
        free(class_name);
        class_name = NULL;
    }
    if (instance_name && strnlen(instance_name, MAXLEN) >= MAXLEN) {
        free(instance_name);
        instance_name = NULL;
    }
    if (name && strnlen(name, MAXLEN) >= MAXLEN) {
        free(name);
        name = NULL;
    }

    if (!class_name || !instance_name || !name) {
        free(class_name);
        free(instance_name);
        free(name);
        return;
    }

    while (r != NULL) {
	    rule_t *next = r->next;
	    if ((streq(class_name, MATCH_ANY) || streq(rule_cond_pattern(&r->conds[RULE_PROP_CLASS]), class_name)) &&
	        (streq(instance_name, MATCH_ANY) || streq(rule_cond_pattern(&r->conds[RULE_PROP_INSTANCE]), instance_name)) &&
	        (streq(name, MATCH_ANY) || streq(rule_cond_pattern(&r->conds[RULE_PROP_NAME]), name))) {
		    remove_rule(r);
	    }
	    r = next;
    }
    free(class_name);
    free(instance_name);
    free(name);
}


bool remove_rule_by_index(int idx)
{
	for (rule_t *r = rule_head; r != NULL; r = r->next, idx--) {
		if (idx == 0) {
			remove_rule(r);
			return true;
		}
	}
	return false;
}

rule_consequence_t *make_rule_consequence(void)
{
	rule_consequence_t *rc = calloc(1, sizeof(rule_consequence_t));
	if (rc == NULL) {
		return NULL;
	}
	rc->manage = rc->focus = rc->border = true;
	rc->layer = NULL;
	rc->state = NULL;
	rc->rect = NULL;
	rc->honor_size_hints = HONOR_SIZE_HINTS_DEFAULT;
	return rc;
}

pending_rule_t *make_pending_rule(int fd, bspwm_wid_t win, rule_consequence_t *csq)
{
	pending_rule_t *pr = calloc(1, sizeof(pending_rule_t));
	if (pr == NULL) {
		return NULL;
	}
	pr->prev = pr->next = NULL;
	pr->event_head = pr->event_tail = NULL;
	pr->fd = fd;
	pr->win = win;
	pr->csq = csq;
	return pr;
}

void add_pending_rule(pending_rule_t *pr)
{
	if (pr == NULL) {
		return;
	}
	if (pending_rule_head == NULL) {
		pending_rule_head = pending_rule_tail = pr;
	} else {
		pending_rule_tail->next = pr;
		pr->prev = pending_rule_tail;
		pending_rule_tail = pr;
	}
	struct epoll_event ev = { .events = EPOLLIN, .data.fd = pr->fd };
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pr->fd, &ev) == -1) {
		/* The fd would never become readable, so the pending rule would
		 * never be serviced or freed — a permanent fd/memory leak. Drop it
		 * instead (the window simply manages without the external rule). */
		warn("add_pending_rule: epoll_ctl failed; dropping pending rule.\n");
		remove_pending_rule(pr);
	}
}

void remove_pending_rule(pending_rule_t *pr)
{
	if (pr == NULL) {
		return;
	}
	pending_rule_t *a = pr->prev;
	pending_rule_t *b = pr->next;
	if (a != NULL) {
		a->next = b;
	}
	if (b != NULL) {
		b->prev = a;
	}
	if (pr == pending_rule_head) {
		pending_rule_head = b;
	}
	if (pr == pending_rule_tail) {
		pending_rule_tail = a;
	}
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pr->fd, NULL);
	close(pr->fd);
	free(pr->csq);
	event_queue_t *eq = pr->event_head;
	while (eq != NULL) {
		event_queue_t *next = eq->next;
		free(eq);
		eq = next;
	}
	free(pr);
}

void postpone_event(pending_rule_t *pr, void *evt)
{
	event_queue_t *eq = make_event_queue(evt);
	if (pr->event_tail == NULL) {
		pr->event_head = pr->event_tail = eq;
	} else {
		pr->event_tail->next = eq;
		eq->prev = pr->event_tail;
		pr->event_tail = eq;
	}
}

event_queue_t *make_event_queue(void *evt)
{
    if (!evt) return NULL;

    event_queue_t *eq = calloc(1, sizeof(event_queue_t));
    if (!eq) return NULL;

    /* Copy up to 64 bytes of event data (fits any backend event) */
    memcpy(eq->data, evt, sizeof(eq->data));

    eq->prev = eq->next = NULL;
    return eq;
}

#define SET_CSQ_SPLIT_DIR(val) \
	do { \
		if (csq->split_dir == NULL) { \
			csq->split_dir = calloc(1, sizeof(direction_t)); \
		} \
		if (csq->split_dir == NULL) { \
			perror("SET_CSQ_SPLIT_DIR: calloc"); \
			break; \
		} \
		*(csq->split_dir) = (val); \
	} while (0)

#define SET_CSQ_STATE(val) \
	do { \
		if (csq->state == NULL) { \
			csq->state = calloc(1, sizeof(client_state_t)); \
		} \
		if (csq->state == NULL) { \
			perror("SET_CSQ_STATE: calloc"); \
			break; \
		} \
		*(csq->state) = (val); \
	} while (0)

#define SET_CSQ_LAYER(val) \
	do { \
		if (csq->layer == NULL) { \
			csq->layer = calloc(1, sizeof(stack_layer_t)); \
		} \
		if (csq->layer == NULL) { \
			perror("SET_CSQ_LAYER: calloc"); \
			break; \
		} \
		*(csq->layer) = (val); \
	} while (0)

void _apply_window_type(bspwm_wid_t win, bspwm_window_type_t type, rule_consequence_t *csq)
{
	switch (type) {
		case BSP_WINDOW_TYPE_TOOLBAR:
		case BSP_WINDOW_TYPE_UTILITY:
			csq->focus = false;
			break;
		case BSP_WINDOW_TYPE_DIALOG:
			SET_CSQ_STATE(STATE_FLOATING);
			csq->center = true;
			break;
		case BSP_WINDOW_TYPE_DOCK:
		case BSP_WINDOW_TYPE_DESKTOP:
		case BSP_WINDOW_TYPE_NOTIFICATION:
			csq->manage = false;
			if (type == BSP_WINDOW_TYPE_DESKTOP)
				window_lower(win);
			break;
		default:
			break;
	}
}

void _apply_window_state(bspwm_wid_t win, rule_consequence_t *csq)
{
	/* Query _NET_WM_STATE to set initial fullscreen/above/below/sticky */
	xcb_ewmh_get_atoms_reply_t wm_state;
	if (xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, win), &wm_state, NULL) == 1) {
		for (unsigned int i = 0; i < wm_state.atoms_len; i++) {
			xcb_atom_t a = wm_state.atoms[i];
			if (a == ewmh->_NET_WM_STATE_FULLSCREEN) {
				SET_CSQ_STATE(STATE_FULLSCREEN);
			} else if (a == ewmh->_NET_WM_STATE_BELOW) {
				SET_CSQ_LAYER(LAYER_BELOW);
			} else if (a == ewmh->_NET_WM_STATE_ABOVE) {
				SET_CSQ_LAYER(LAYER_ABOVE);
			} else if (a == ewmh->_NET_WM_STATE_STICKY) {
				csq->sticky = true;
			}
		}
		xcb_ewmh_get_atoms_reply_wipe(&wm_state);
	}
}

void _apply_transient(bspwm_wid_t transient_for, rule_consequence_t *csq)
{
	if (transient_for != BSPWM_WID_NONE) {
		SET_CSQ_STATE(STATE_FLOATING);
	}
}

void _apply_hints(bspwm_wid_t win, rule_consequence_t *csq)
{
	bspwm_size_hints_t size_hints;
	if (backend_get_size_hints(win, &size_hints)) {
		if ((size_hints.flags & (BSP_SIZE_HINT_P_MIN_SIZE | BSP_SIZE_HINT_P_MAX_SIZE)) &&
		    size_hints.min_width == size_hints.max_width && size_hints.min_height == size_hints.max_height) {
			SET_CSQ_STATE(STATE_FLOATING);
		}
	}
}

void _apply_class(bspwm_wid_t win, rule_consequence_t *csq)
{
	backend_get_window_class(win, csq->class_name, csq->instance_name, sizeof(csq->class_name));
}

void _apply_name(bspwm_wid_t win, rule_consequence_t *csq)
{
	backend_get_window_name(win, csq->name, sizeof(csq->name));
}

void parse_keys_values(char *buf, rule_consequence_t *csq)
{
    if (buf == NULL)
        return;

    size_t len = strnlen(buf, BUFSIZ);
    if (len >= BUFSIZ)
        return;

    char *buf_copy = malloc(len + 1);
    if (buf_copy == NULL)
        return;

    memcpy(buf_copy, buf, len);
    buf_copy[len] = '\0';

    char *key = strtok(buf_copy, CSQ_BLK);
    char *value = strtok(NULL, CSQ_BLK);

    while (key != NULL && value != NULL) {
        if (strnlen(key, MAXLEN) < MAXLEN && strnlen(value, MAXLEN) < MAXLEN) {
            parse_key_value(key, value, csq);
        }
        key = strtok(NULL, CSQ_BLK);
        value = strtok(NULL, CSQ_BLK);
    }

    free(buf_copy);
}


/* Everything a rule can match against, read once per window. */
typedef struct {
	char class_name[MAXLEN];
	char instance_name[MAXLEN];
	char name[MAXLEN];
	char role[MAXLEN];
	const char *type;
	const char *transient;
} window_props_t;

static const char *window_type_name(bspwm_window_type_t type)
{
	switch (type) {
		case BSP_WINDOW_TYPE_DOCK: return "dock";
		case BSP_WINDOW_TYPE_DESKTOP: return "desktop";
		case BSP_WINDOW_TYPE_NOTIFICATION: return "notification";
		case BSP_WINDOW_TYPE_DIALOG: return "dialog";
		case BSP_WINDOW_TYPE_UTILITY: return "utility";
		case BSP_WINDOW_TYPE_TOOLBAR: return "toolbar";
		default: return "normal";
	}
}

/* Whether any rule has a condition on the role. Reading WM_WINDOW_ROLE is a
 * round-trip to the server for every window managed, which is only worth
 * paying when some rule is going to look at the answer. The list is short,
 * and walking it keeps no count that a rule freed without ever being added
 * (cmd_rule()'s error paths) could throw off. */
static bool rules_need_role(void)
{
	for (rule_t *r = rule_head; r != NULL; r = r->next) {
		if (r->conds[RULE_PROP_ROLE].used) {
			return true;
		}
	}
	return false;
}

/* Fill `props` from what the rules already read, plus the role when a rule
 * needs it; otherwise the role is left empty, which no rule is looking at. */
static void collect_window_props(bspwm_wid_t win, rule_consequence_t *csq,
                                 bspwm_window_type_t type, bspwm_wid_t transient_for,
                                 window_props_t *props)
{
	snprintf(props->class_name, sizeof(props->class_name), "%s", csq->class_name);
	snprintf(props->instance_name, sizeof(props->instance_name), "%s", csq->instance_name);
	snprintf(props->name, sizeof(props->name), "%s", csq->name);
	props->role[0] = '\0';
	if (rules_need_role()) {
		backend_get_window_role(win, props->role, sizeof(props->role));
	}
	props->type = window_type_name(type);
	props->transient = (transient_for != BSPWM_WID_NONE) ? "on" : "off";
}

void apply_rules(bspwm_wid_t win, rule_consequence_t *csq)
{
	/* Query window properties via backend */
	bspwm_window_type_t type = BSP_WINDOW_TYPE_NORMAL;
	if (!backend_get_window_type(win, &type)) {
		type = BSP_WINDOW_TYPE_NORMAL;
	}
	bspwm_wid_t transient_for = BSPWM_WID_NONE;
	backend_get_transient_for(win, &transient_for);

	_apply_window_type(win, type, csq);
	_apply_window_state(win, csq);
	_apply_transient(transient_for, csq);
	_apply_hints(win, csq);
	_apply_class(win, csq);
	_apply_name(win, csq);

	window_props_t props;
	collect_window_props(win, csq, type, transient_for, &props);

	const char *values[RULE_PROP_COUNT] = {
		props.class_name, props.instance_name, props.name,
		props.type, props.role, props.transient,
	};

	rule_t *rule = rule_head;
	while (rule != NULL) {
		rule_t *next = rule->next;
		if (rule_conds_match(rule->conds, values)) {
			char effect[MAXLEN];
			snprintf(effect, sizeof(effect), "%s", rule->effect);
			parse_keys_values(effect, csq);
			if (rule->one_shot) {
				remove_rule(rule);
				break;
			}
		}
		rule = next;
	}
}

bool schedule_rules(bspwm_wid_t win, rule_consequence_t *csq)
{
	if (external_rules_command[0] == '\0') {
		return false;
	}
	resolve_rule_consequence(csq);
	int fds[2];
	if (pipe(fds) == -1) {
		return false;
	}
	pid_t pid = fork();
	if (pid == 0) {
		int dpy_fd = backend_get_fd();
		if (dpy_fd >= 0) {
			close(dpy_fd);
		}
		dup2(fds[1], 1);
		close(fds[0]);
		char wid[SMALEN];
		char *csq_buf;
		print_rule_consequence(&csq_buf, csq);
		snprintf(wid, sizeof(wid), "%i", win);
		setsid();
		execl(external_rules_command, external_rules_command, wid, csq->class_name, csq->instance_name, csq_buf, NULL);
		free(csq_buf);
		err("Couldn't spawn rule command.\n");
	} else if (pid > 0) {
		close(fds[1]);
		pending_rule_t *pr = make_pending_rule(fds[0], win, csq);
		if (pr == NULL) {
			/* Can't track the child's reply. Don't leak the read end; return
			 * false so the caller manages the window (and frees csq) directly. */
			close(fds[0]);
			return false;
		}
		add_pending_rule(pr);
	} else {
		/* fork() failed: neither end of the pipe will ever be used. */
		close(fds[0]);
		close(fds[1]);
	}
	return (pid != -1);
}

void parse_rule_consequence(int fd, rule_consequence_t *csq)
{
    if (fd == -1) {
        return;
    }
    char data[BUFSIZ];
    int nb;
    int total_read = 0;

    while ((nb = read(fd, data + total_read, sizeof(data) - total_read - 1)) > 0) {
        total_read += nb;
        if ((size_t)total_read >= sizeof(data) - 1) {

            break;
        }
    }

    if (total_read > 0) {
        data[total_read] = '\0';
        parse_keys_values(data, csq);
    }
}

/* One setter per consequence key. They are what `parse_key_value()` below
 * dispatches to, and they are named once, in `consequence_keys`. */
static void csq_set_monitor(char *value, rule_consequence_t *csq)
{
	snprintf(csq->monitor_desc, sizeof(csq->monitor_desc), "%s", value);
}

static void csq_set_desktop(char *value, rule_consequence_t *csq)
{
	snprintf(csq->desktop_desc, sizeof(csq->desktop_desc), "%s", value);
}

static void csq_set_node(char *value, rule_consequence_t *csq)
{
	snprintf(csq->node_desc, sizeof(csq->node_desc), "%s", value);
}

static void csq_set_split_dir(char *value, rule_consequence_t *csq)
{
	direction_t dir;
	if (parse_direction(value, &dir)) {
		SET_CSQ_SPLIT_DIR(dir);
	}
}

static void csq_set_state(char *value, rule_consequence_t *csq)
{
	client_state_t cst;
	if (parse_client_state(value, &cst)) {
		SET_CSQ_STATE(cst);
	}
}

static void csq_set_layer(char *value, rule_consequence_t *csq)
{
	stack_layer_t lyr;
	if (parse_stack_layer(value, &lyr)) {
		SET_CSQ_LAYER(lyr);
	}
}

static void csq_set_split_ratio(char *value, rule_consequence_t *csq)
{
	double rat;
	if (sscanf(value, "%lf", &rat) == 1 && rat > 0 && rat < 1) {
		csq->split_ratio = rat;
	}
}

static void csq_set_rectangle(char *value, rule_consequence_t *csq)
{
	if (csq->rect == NULL) {
		csq->rect = calloc(1, sizeof(bspwm_rect_t));
		if (csq->rect == NULL) {
			return;
		}
	}
	if (!parse_rectangle(value, csq->rect)) {
		free(csq->rect);
		csq->rect = NULL;
	}
}

static void csq_set_honor_size_hints(char *value, rule_consequence_t *csq)
{
	if (!parse_honor_size_hints_mode(value, &csq->honor_size_hints)) {
		csq->honor_size_hints = HONOR_SIZE_HINTS_DEFAULT;
	}
}

/* The boolean consequences all read the same way: a value that is not a
 * boolean leaves the flag alone. */
#define CSQ_SET_BOOL(name) \
	static void csq_set_##name(char *value, rule_consequence_t *csq) \
	{ \
		bool v; \
		if (parse_bool(value, &v)) { \
			csq->name = v; \
		} \
	}
CSQ_SET_BOOL(hidden)
CSQ_SET_BOOL(sticky)
CSQ_SET_BOOL(private)
CSQ_SET_BOOL(locked)
CSQ_SET_BOOL(marked)
CSQ_SET_BOOL(center)
CSQ_SET_BOOL(follow)
CSQ_SET_BOOL(manage)
CSQ_SET_BOOL(focus)
CSQ_SET_BOOL(border)
#undef CSQ_SET_BOOL

/* Every key a rule consequence takes, and nothing else. This is the only
 * place they are enumerated: `parse_key_value()` dispatches through it, and
 * `cmd_rule()` (src/messages.c) asks rule_is_consequence_key() whether an
 * argument is one before accepting the rule. A key cmd_rule() does not know
 * is an error, so a second list kept by hand beside this one would start
 * refusing legitimate rules the moment the two drifted apart.
 * `ignore_tile_limits` has no setter on purpose: nothing reads it off the
 * consequence — `effect_has()` (src/tree.c) reads it straight out of the
 * rule's effect text — but it is still a key a rule may carry. */
static const struct {
	const char *key;
	void (*set)(char *value, rule_consequence_t *csq);
} consequence_keys[] = {
	{"monitor", csq_set_monitor},
	{"desktop", csq_set_desktop},
	{"node", csq_set_node},
	{"split_dir", csq_set_split_dir},
	{"split_ratio", csq_set_split_ratio},
	{"state", csq_set_state},
	{"layer", csq_set_layer},
	{"honor_size_hints", csq_set_honor_size_hints},
	{"rectangle", csq_set_rectangle},
	{"hidden", csq_set_hidden},
	{"sticky", csq_set_sticky},
	{"private", csq_set_private},
	{"locked", csq_set_locked},
	{"marked", csq_set_marked},
	{"center", csq_set_center},
	{"follow", csq_set_follow},
	{"manage", csq_set_manage},
	{"focus", csq_set_focus},
	{"border", csq_set_border},
	{"ignore_tile_limits", NULL},
};

/* Whether `arg` opens the `property=pattern` form of a rule: a key written
 * right before an `=` or a `~=`. The two forms are told apart by the shape
 * of that key — lowercase letters and underscores, which is every key bspwm
 * has — and not by the mere presence of an `=`, because the old
 * CLASS[:INSTANCE[:NAME]] form ends in the window title, which is free text
 * and may well carry one of its own ("st:*:vim = notes"). Judging the shape
 * rather than asking whether the key is a key bspwm knows is what keeps a
 * misspelled condition ("clas=kitty") an error instead of turning it into a
 * class pattern that could never match anything. Only the first `=` is
 * read, so everything after it stays part of the value. */
bool rule_is_key_value(const char *arg)
{
	if (arg == NULL) {
		return false;
	}
	const char *sep = strchr(arg, '=');
	if (sep == NULL) {
		return false;
	}
	size_t key_len = (size_t) (sep - arg);
	/* A `~` right before the `=` belongs to the operator, not to the key. */
	if (key_len > 0 && sep[-1] == '~') {
		key_len--;
	}
	if (key_len == 0) {
		return false;
	}
	for (size_t i = 0; i < key_len; i++) {
		if ((arg[i] < 'a' || arg[i] > 'z') && arg[i] != '_') {
			return false;
		}
	}
	return true;
}

bool rule_is_consequence_key(const char *key)
{
	if (key == NULL) {
		return false;
	}
	for (size_t i = 0; i < LENGTH(consequence_keys); i++) {
		if (streq(consequence_keys[i].key, key)) {
			return true;
		}
	}
	return false;
}

void parse_key_value(char *key, char *value, rule_consequence_t *csq)
{
	for (size_t i = 0; i < LENGTH(consequence_keys); i++) {
		if (streq(consequence_keys[i].key, key)) {
			if (consequence_keys[i].set != NULL) {
				consequence_keys[i].set(value, csq);
			}
			return;
		}
	}
}

#undef SET_CSQ_LAYER
#undef SET_CSQ_STATE

void list_rules(FILE *rsp)
{
	for (rule_t *r = rule_head; r != NULL; r = r->next) {
		fprintf(rsp, "%s %c> %s\n", r->cause, r->one_shot?'-':'=', r->effect);
	}
}

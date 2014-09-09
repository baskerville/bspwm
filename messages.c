/* Copyright (c) 2012-2014, Bastien Dejean
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
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bspwm.h"
#include "desktop.h"
#include "ewmh.h"
#include "history.h"
#include "monitor.h"
#include "pointer.h"
#include "query.h"
#include "rule.h"
#include "restore.h"
#include "settings.h"
#include "tree.h"
#include "window.h"
#include "common.h"
#include "subscribe.h"
#include "messages.h"

int handle_message(char *msg, int msg_len, FILE *rsp)
{
	int cap = INIT_CAP;
	int num = 0;
	char **args = malloc(cap * sizeof(char *));
	if (args == NULL)
		return MSG_FAILURE;

	for (int i = 0, j = 0; i < msg_len; i++) {
		if (msg[i] == 0) {
			args[num++] = msg + j;
			j = i + 1;
		}
		if (num >= cap) {
			cap *= 2;
			char **new = realloc(args, cap * sizeof(char *));
			if (new == NULL) {
				free(args);
				return MSG_FAILURE;
			} else {
				args = new;
			}
		}
	}

	if (num < 1) {
		free(args);
		return MSG_SYNTAX;
	}

	char **args_orig = args;
	int ret = process_message(args, num, rsp);
	free(args_orig);
	return ret;
}

int process_message(char **args, int num, FILE *rsp)
{
	if (streq("window", *args)) {
		return cmd_window(++args, --num);
	} else if (streq("desktop", *args)) {
		return cmd_desktop(++args, --num);
	} else if (streq("monitor", *args)) {
		return cmd_monitor(++args, --num);
	} else if (streq("query", *args)) {
		return cmd_query(++args, --num, rsp);
	} else if (streq("restore", *args)) {
		return cmd_restore(++args, --num);
	} else if (streq("control", *args)) {
		return cmd_control(++args, --num, rsp);
	} else if (streq("rule", *args)) {
		return cmd_rule(++args, --num, rsp);
	} else if (streq("pointer", *args)) {
		return cmd_pointer(++args, --num);
	} else if (streq("config", *args)) {
		return cmd_config(++args, --num, rsp);
	} else if (streq("quit", *args)) {
		return cmd_quit(++args, --num);
	}

	return MSG_UNKNOWN;
}

int cmd_window(char **args, int num)
{
	if (num < 1)
		return MSG_SYNTAX;

	coordinates_t ref = {mon, mon->desk, mon->desk->focus};
	coordinates_t trg = ref;

	if ((*args)[0] != OPT_CHR) {
		if (node_from_desc(*args, &ref, &trg))
			num--, args++;
		else
			return MSG_FAILURE;
	}

	if (trg.node == NULL)
		return MSG_FAILURE;

	bool dirty = false;

	while (num > 0) {
		if (streq("-f", *args) || streq("--focus", *args)) {
			coordinates_t dst = trg;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if (!node_from_desc(*args, &trg, &dst))
					return MSG_FAILURE;
			}
			focus_node(dst.monitor, dst.desktop, dst.node);
		} else if (streq("-d", *args) || streq("--to-desktop", *args)) {
			num--, args++;
			coordinates_t dst;
			if (desktop_from_desc(*args, &trg, &dst)) {
				if (transfer_node(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.desktop, dst.desktop->focus)) {
					trg.monitor = dst.monitor;
					trg.desktop = dst.desktop;
				}
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-m", *args) || streq("--to-monitor", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			coordinates_t dst;
			if (monitor_from_desc(*args, &trg, &dst)) {
				if (transfer_node(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.monitor->desk, dst.monitor->desk->focus)) {
					trg.monitor = dst.monitor;
					trg.desktop = dst.monitor->desk;
				}
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-w", *args) || streq("--to-window", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			coordinates_t dst;
			if (node_from_desc(*args, &trg, &dst)) {
				if (transfer_node(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.desktop, dst.node)) {
					trg.monitor = dst.monitor;
					trg.desktop = dst.desktop;
				}
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-s", *args) || streq("--swap", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			coordinates_t dst;
			if (node_from_desc(*args, &trg, &dst)) {
				if (swap_nodes(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.desktop, dst.node)) {
					if (trg.desktop != dst.desktop)
						arrange(trg.monitor, trg.desktop);
					trg.monitor = dst.monitor;
					trg.desktop = dst.desktop;
					dirty = true;
				}
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-t", *args) || streq("--toggle", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			char *key = strtok(*args, EQL_TOK);
			char *val = strtok(NULL, EQL_TOK);
			alter_state_t a;
			bool b;
			if (val == NULL) {
				a = ALTER_TOGGLE;
			} else {
				if (parse_bool(val, &b))
					a = ALTER_SET;
				else
					return MSG_FAILURE;
			}
			if (streq("fullscreen", key)) {
				set_fullscreen(trg.node, (a == ALTER_SET ? b : !trg.node->client->fullscreen));
				dirty = true;
			} else if (streq("pseudo_tiled", key)) {
				set_pseudo_tiled(trg.node, (a == ALTER_SET ? b : !trg.node->client->pseudo_tiled));
				dirty = true;
			} else if (streq("floating", key)) {
				set_floating(trg.node, (a == ALTER_SET ? b : !trg.node->client->floating));
				dirty = true;
			} else if (streq("locked", key)) {
				set_locked(trg.monitor, trg.desktop, trg.node, (a == ALTER_SET ? b : !trg.node->client->locked));
			} else if (streq("sticky", key)) {
				set_sticky(trg.monitor, trg.desktop, trg.node, (a == ALTER_SET ? b : !trg.node->client->sticky));
			} else if (streq("private", key)) {
				set_private(trg.monitor, trg.desktop, trg.node, (a == ALTER_SET ? b : !trg.node->client->private));
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-p", *args) || streq("--presel", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			if (!is_tiled(trg.node->client) ||
			    trg.desktop->layout != LAYOUT_TILED)
				return MSG_FAILURE;
			if (streq("cancel", *args)) {
				reset_mode(&trg);
			} else {
				direction_t dir;
				if (parse_direction(*args, &dir)) {
					double rat = trg.node->split_ratio;
					if (num > 1 && *(args + 1)[0] != OPT_CHR) {
						num--, args++;
						if (sscanf(*args, "%lf", &rat) != 1 || rat <= 0 || rat >= 1)
							return MSG_FAILURE;
					}
					if (auto_cancel && trg.node->split_mode == MODE_MANUAL &&
					    dir == trg.node->split_dir &&
					    rat == trg.node->split_ratio) {
						reset_mode(&trg);
					} else {
						trg.node->split_mode = MODE_MANUAL;
						trg.node->split_dir = dir;
						trg.node->split_ratio = rat;
					}
					window_draw_border(trg.node, trg.desktop->focus == trg.node, mon == trg.monitor);
				} else {
					return MSG_FAILURE;
				}
			}
		} else if (streq("-e", *args) || streq("--edge", *args)) {
			num--, args++;
			if (num < 2)
				return MSG_SYNTAX;
			if (!is_tiled(trg.node->client))
				return MSG_FAILURE;
			direction_t dir;
			if (!parse_direction(*args, &dir))
				return MSG_FAILURE;
			node_t *n = find_fence(trg.node, dir);
			if (n == NULL)
				return MSG_FAILURE;
			num--, args++;
			if ((*args)[0] == '+' || (*args)[0] == '-') {
				int pix;
				if (sscanf(*args, "%i", &pix) == 1) {
					int max = (n->split_type == TYPE_HORIZONTAL ? n->rectangle.height : n->rectangle.width);
					double rat = ((max * n->split_ratio) + pix) / max;
					if (rat > 0 && rat < 1)
						n->split_ratio = rat;
					else
						return MSG_FAILURE;
				} else {
					return MSG_FAILURE;
				}
			} else {
				double rat;
				if (sscanf(*args, "%lf", &rat) == 1 && rat > 0 && rat < 1)
					n->split_ratio = rat;
				else
					return MSG_FAILURE;
			}
			dirty = true;
		} else if (streq("-r", *args) || streq("--ratio", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			double rat;
			if (sscanf(*args, "%lf", &rat) == 1 && rat > 0 && rat < 1) {
				trg.node->split_ratio = rat;
				window_draw_border(trg.node, trg.desktop->focus == trg.node, mon == trg.monitor);
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-R", *args) || streq("--rotate", *args)) {
			num--, args++;
			if (num < 2)
				return MSG_SYNTAX;
			direction_t dir;
			if (!parse_direction(*args, &dir))
				return MSG_FAILURE;
			node_t *n = find_fence(trg.node, dir);
			if (n == NULL)
				return MSG_FAILURE;
			num--, args++;
			int deg;
			if (parse_degree(*args, &deg)) {
				rotate_tree(n, deg);
				dirty = true;
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-c", *args) || streq("--close", *args)) {
			if (num > 1)
				return MSG_SYNTAX;
			window_close(trg.node);
		} else if (streq("-k", *args) || streq("--kill", *args)) {
			if (num > 1)
				return MSG_SYNTAX;
			window_kill(trg.monitor, trg.desktop, trg.node);
			dirty = true;
		} else {
			return MSG_SYNTAX;
		}

		num--, args++;
	}

	if (dirty)
		arrange(trg.monitor, trg.desktop);

	return MSG_SUCCESS;
}

int cmd_desktop(char **args, int num)
{
	if (num < 1)
		return MSG_SYNTAX;

	coordinates_t ref = {mon, mon->desk, NULL};
	coordinates_t trg = ref;

	if ((*args)[0] != OPT_CHR) {
		if (desktop_from_desc(*args, &ref, &trg))
			num--, args++;
		else
			return MSG_FAILURE;
	}

	bool dirty = false;

	while (num > 0) {
		if (streq("-f", *args) || streq("--focus", *args)) {
			coordinates_t dst = trg;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if (!desktop_from_desc(*args, &trg, &dst))
					return MSG_FAILURE;
			}
			if (auto_alternate && dst.desktop == mon->desk) {
				desktop_select_t sel = {DESKTOP_STATUS_ALL, false, false};
				history_find_desktop(HISTORY_OLDER, &trg, &dst, sel);
			}
			focus_node(dst.monitor, dst.desktop, dst.desktop->focus);
		} else if (streq("-m", *args) || streq("--to-monitor", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			if (trg.monitor->desk_head == trg.monitor->desk_tail)
				return MSG_FAILURE;
			coordinates_t dst;
			if (monitor_from_desc(*args, &trg, &dst)) {
				transfer_desktop(trg.monitor, dst.monitor, trg.desktop);
				trg.monitor = dst.monitor;
				update_current();
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-s", *args) || streq("--swap", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			coordinates_t dst;
			if (desktop_from_desc(*args, &trg, &dst))
				swap_desktops(trg.monitor, trg.desktop, dst.monitor, dst.desktop);
			else
				return MSG_FAILURE;
		} else if (streq("-l", *args) || streq("--layout", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			layout_t lyt;
			cycle_dir_t cyc;
			if (parse_cycle_direction(*args, &cyc))
				change_layout(trg.monitor, trg.desktop, (trg.desktop->layout + 1) % 2);
			else if (parse_layout(*args, &lyt))
				change_layout(trg.monitor, trg.desktop, lyt);
			else
				return MSG_FAILURE;
		} else if (streq("-n", *args) || streq("--rename", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			snprintf(trg.desktop->name, sizeof(trg.desktop->name), "%s", *args);
			ewmh_update_desktop_names();
			put_status();
		} else if (streq("-r", *args) || streq("--remove", *args)) {
			if (trg.desktop->root == NULL &&
			    trg.monitor->desk_head != trg.monitor->desk_tail) {
				remove_desktop(trg.monitor, trg.desktop);
				show_desktop(trg.monitor->desk);
				update_current();
				return MSG_SUCCESS;
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-c", *args) || streq("--cancel-presel", *args)) {
			reset_mode(&trg);
		} else if (streq("-F", *args) || streq("--flip", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			flip_t flp;
			if (parse_flip(*args, &flp)) {
				flip_tree(trg.desktop->root, flp);
				dirty = true;
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-R", *args) || streq("--rotate", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			int deg;
			if (parse_degree(*args, &deg)) {
				rotate_tree(trg.desktop->root, deg);
				dirty = true;
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-E", *args) || streq("--equalize", *args)) {
			equalize_tree(trg.desktop->root);
			dirty = true;
		} else if (streq("-B", *args) || streq("--balance", *args)) {
			balance_tree(trg.desktop->root);
			dirty = true;
		} else if (streq("-C", *args) || streq("--circulate", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			circulate_dir_t cir;
			if (parse_circulate_direction(*args, &cir)) {
				circulate_leaves(trg.monitor, trg.desktop, cir);
				dirty = true;
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-t", *args) || streq("--toggle", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			char *key = strtok(*args, EQL_TOK);
			char *val = strtok(NULL, EQL_TOK);
			alter_state_t a;
			bool b;
			if (val == NULL) {
				a = ALTER_TOGGLE;
			} else {
				if (parse_bool(val, &b))
					a = ALTER_SET;
				else
					return MSG_FAILURE;
			}
			if (streq("floating", key))
				trg.desktop->floating = (a == ALTER_SET ? b : !trg.desktop->floating);
			else
				return MSG_FAILURE;
		} else {
			return MSG_SYNTAX;
		}
		num--, args++;
	}

	if (dirty)
		arrange(trg.monitor, trg.desktop);

	return MSG_SUCCESS;
}

int cmd_monitor(char **args, int num)
{
	if (num < 1)
		return MSG_SYNTAX;

	coordinates_t ref = {mon, NULL, NULL};
	coordinates_t trg = ref;

	if ((*args)[0] != OPT_CHR) {
		if (monitor_from_desc(*args, &ref, &trg))
			num--, args++;
		else
			return MSG_FAILURE;
	}

	while (num > 0) {
		if (streq("-f", *args) || streq("--focus", *args)) {
			coordinates_t dst = trg;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if (!monitor_from_desc(*args, &trg, &dst))
					return MSG_FAILURE;
			}
			if (auto_alternate && dst.monitor == mon) {
				desktop_select_t sel = {DESKTOP_STATUS_ALL, false, false};
				history_find_monitor(HISTORY_OLDER, &trg, &dst, sel);
			}
			focus_node(dst.monitor, dst.monitor->desk, dst.monitor->desk->focus);
		} else if (streq("-d", *args) || streq("--reset-desktops", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			desktop_t *d = trg.monitor->desk_head;
			while (num > 0 && d != NULL) {
				snprintf(d->name, sizeof(d->name), "%s", *args);
				initialize_desktop(d);
				arrange(trg.monitor, d);
				d = d->next;
				num--, args++;
			}
			put_status();
			while (num > 0) {
				add_desktop(trg.monitor, make_desktop(*args));
				num--, args++;
			}
			while (d != NULL) {
				desktop_t *next = d->next;
				if (d == mon->desk)
					focus_node(trg.monitor, d->prev, d->prev->focus);
				merge_desktops(trg.monitor, d, mon, mon->desk);
				remove_desktop(trg.monitor, d);
				d = next;
			}
		} else if (streq("-a", *args) || streq("--add-desktops", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			while (num > 0) {
				add_desktop(trg.monitor, make_desktop(*args));
				num--, args++;
			}
		} else if (streq("-r", *args) || streq("--remove-desktops", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			while (num > 0) {
				coordinates_t dst;
				if (locate_desktop(*args, &dst) && dst.monitor->desk_head != dst.monitor->desk_tail && dst.desktop->root == NULL) {
					remove_desktop(dst.monitor, dst.desktop);
					show_desktop(dst.monitor->desk);
				}
				num--, args++;
			}
		} else if (streq("-o", *args) || streq("--order-desktops", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			desktop_t *d = trg.monitor->desk_head;
			while (d != NULL && num > 0) {
				desktop_t *next = d->next;
				coordinates_t dst;
				if (locate_desktop(*args, &dst) && dst.monitor == trg.monitor) {
					swap_desktops(trg.monitor, d, dst.monitor, dst.desktop);
					if (next == dst.desktop)
						next = d;
				}
				d = next;
				num--, args++;
			}
		} else if (streq("-n", *args) || streq("--rename", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			snprintf(trg.monitor->name, sizeof(trg.monitor->name), "%s", *args);
			put_status();
		} else if (streq("-s", *args) || streq("--swap", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			coordinates_t dst;
			if (monitor_from_desc(*args, &trg, &dst))
				swap_monitors(trg.monitor, dst.monitor);
			else
				return MSG_FAILURE;
		} else {
			return MSG_SYNTAX;
		}
		num--, args++;
	}

	return MSG_SUCCESS;
}

int cmd_query(char **args, int num, FILE *rsp)
{
	coordinates_t ref = {mon, mon->desk, mon->desk->focus};
	coordinates_t trg = {NULL, NULL, NULL};
	domain_t dom = DOMAIN_TREE;
	int d = 0, t = 0;

	while (num > 0) {
		if (streq("-T", *args) || streq("--tree", *args)) {
			dom = DOMAIN_TREE, d++;
		} else if (streq("-M", *args) || streq("--monitors", *args)) {
			dom = DOMAIN_MONITOR, d++;
		} else if (streq("-D", *args) || streq("--desktops", *args)) {
			dom = DOMAIN_DESKTOP, d++;
		} else if (streq("-W", *args) || streq("--windows", *args)) {
			dom = DOMAIN_WINDOW, d++;
		} else if (streq("-H", *args) || streq("--history", *args)) {
			dom = DOMAIN_HISTORY, d++;
		} else if (streq("-S", *args) || streq("--stack", *args)) {
			dom = DOMAIN_STACK, d++;
		} else if (streq("-m", *args) || streq("--monitor", *args)) {
			trg.monitor = ref.monitor;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if (!monitor_from_desc(*args, &ref, &trg))
					return MSG_FAILURE;
			}
			t++;
		} else if (streq("-d", *args) || streq("--desktop", *args)) {
			trg.monitor = ref.monitor;
			trg.desktop = ref.desktop;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if (!desktop_from_desc(*args, &ref, &trg))
					return MSG_FAILURE;
			}
			t++;
		} else if (streq("-w", *args) || streq("--window", *args)) {
			trg = ref;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if (!node_from_desc(*args, &ref, &trg))
					return MSG_FAILURE;
			}
			t++;
		} else {
			return MSG_SYNTAX;
		}
		num--, args++;
	}

	if (d != 1 || t > 1)
		return MSG_SYNTAX;

	if (dom == DOMAIN_HISTORY)
		query_history(trg, rsp);
	else if (dom == DOMAIN_STACK)
		query_stack(rsp);
	else if (dom == DOMAIN_WINDOW)
		query_windows(trg, rsp);
	else
		query_monitors(trg, dom, rsp);

	return MSG_SUCCESS;
}

int cmd_rule(char **args, int num, FILE *rsp)
{
	if (num < 1)
		return MSG_SYNTAX;
	while (num > 0) {
		if (streq("-a", *args) || streq("--add", *args)) {
			num--, args++;
			if (num < 2)
				return MSG_SYNTAX;
			rule_t *rule = make_rule();
			snprintf(rule->cause, sizeof(rule->cause), "%s", *args);
			num--, args++;
			size_t i = 0;
			while (num > 0) {
				if (streq("-o", *args) || streq("--one-shot", *args)) {
					rule->one_shot = true;
				} else {
					for (size_t j = 0; i < sizeof(rule->effect) && j < strlen(*args); i++, j++)
						rule->effect[i] = (*args)[j];
					if (num > 1 && i < sizeof(rule->effect))
						rule->effect[i++] = ' ';
				}
				num--, args++;
			}
			rule->effect[MIN(i, sizeof(rule->effect) - 1)] = '\0';
			add_rule(rule);
		} else if (streq("-r", *args) || streq("--remove", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			int idx;
			while (num > 0) {
				if (parse_index(*args, &idx))
					remove_rule_by_index(idx - 1);
				else if (streq("tail", *args))
					remove_rule(rule_tail);
				else if (streq("head", *args))
					remove_rule(rule_head);
				else
					remove_rule_by_cause(*args);
				num--, args++;
			}
		} else if (streq("-l", *args) || streq("--list", *args)) {
			num--, args++;
			list_rules(num > 0 ? *args : NULL, rsp);
		} else {
			return MSG_SYNTAX;
		}
		num--, args++;
	}

	return MSG_SUCCESS;
}

int cmd_pointer(char **args, int num)
{
	if (num < 1)
		return MSG_SYNTAX;
	while (num > 0) {
		if (streq("-t", *args) || streq("--track", *args)) {
			num--, args++;
			if (num < 2)
				return MSG_SYNTAX;
			int x, y;
			if (sscanf(*args, "%i", &x) == 1 && sscanf(*(args + 1), "%i", &y) == 1)
				track_pointer(x, y);
			else
				return MSG_FAILURE;
			num--, args++;
		} else if (streq("-g", *args) || streq("--grab", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			pointer_action_t pac;
			if (parse_pointer_action(*args, &pac))
				grab_pointer(pac);
			else
				return MSG_FAILURE;
		} else if (streq("-u", *args) || streq("--ungrab", *args)) {
			ungrab_pointer();
		} else {
			return MSG_SYNTAX;
		}
		num--, args++;
	}

	return MSG_SUCCESS;
}

int cmd_restore(char **args, int num)
{
	if (num < 1)
		return MSG_SYNTAX;
	while (num > 0) {
		if (streq("-T", *args) || streq("--tree", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			restore_tree(*args);
		} else if (streq("-H", *args) || streq("--history", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			restore_history(*args);
		} else if (streq("-S", *args) || streq("--stack", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			restore_stack(*args);
		} else {
			return MSG_SYNTAX;
		}
		num--, args++;
	}

	return MSG_SUCCESS;
}

int cmd_control(char **args, int num, FILE *rsp)
{
	if (num < 1)
		return MSG_SYNTAX;
	while (num > 0) {
		if (streq("--adopt-orphans", *args)) {
			adopt_orphans();
		} else if (streq("--toggle-visibility", *args)) {
			toggle_visibility();
		} else if (streq("--subscribe", *args)) {
			return MSG_SUBSCRIBE;
		} else if (streq("--get-status", *args)) {
			print_status(rsp);
		} else if (streq("--record-history", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			bool b;
			if (parse_bool(*args, &b))
				record_history = b;
			else
				return MSG_SYNTAX;
		} else {
			return MSG_SYNTAX;
		}
		num--, args++;
	}

	return MSG_SUCCESS;
}

int cmd_config(char **args, int num, FILE *rsp)
{
	if (num < 1)
		return MSG_SYNTAX;
	coordinates_t ref = {mon, mon->desk, mon->desk->focus};
	coordinates_t trg = {NULL, NULL, NULL};
	if ((*args)[0] == OPT_CHR) {
		if (streq("-m", *args) || streq("--monitor", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			if (!monitor_from_desc(*args, &ref, &trg))
				return MSG_FAILURE;
		} else if (streq("-d", *args) || streq("--desktop", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			if (!desktop_from_desc(*args, &ref, &trg))
				return MSG_FAILURE;
		} else if (streq("-w", *args) || streq("--window", *args)) {
			num--, args++;
			if (num < 1)
				return MSG_SYNTAX;
			if (!node_from_desc(*args, &ref, &trg))
				return MSG_FAILURE;
		} else {
			return MSG_SYNTAX;
		}
		num--, args++;
	}
	if (num == 2)
		return set_setting(trg, *args, *(args + 1));
	else if (num == 1)
		return get_setting(trg, *args, rsp);
	else
		return MSG_SYNTAX;
}

int cmd_quit(char **args, int num)
{
	if (num > 0 && sscanf(*args, "%i", &exit_status) != 1)
		return MSG_FAILURE;
	running = false;
	return MSG_SUCCESS;
}

int set_setting(coordinates_t loc, char *name, char *value)
{
#define DESKWINDEFSET(k, v) \
		if (loc.node != NULL) \
			loc.node->client->k = v; \
		else if (loc.desktop != NULL) \
			loc.desktop->k = v; \
		else if (loc.monitor != NULL) \
			for (desktop_t *d = loc.monitor->desk_head; d != NULL; d = d->next) \
				d->k = v; \
		else \
			k = v;
	if (streq("border_width", name)) {
		unsigned int bw;
		if (sscanf(value, "%u", &bw) != 1)
			return MSG_FAILURE;
		DESKWINDEFSET(border_width, bw)
#undef DESKWINDEFSET
#define DESKDEFSET(k, v) \
		if (loc.desktop != NULL) \
			loc.desktop->k = v; \
		else if (loc.monitor != NULL) \
			for (desktop_t *d = loc.monitor->desk_head; d != NULL; d = d->next) \
				d->k = v; \
		else \
			k = v;
	} else if (streq("window_gap", name)) {
		int wg;
		if (sscanf(value, "%i", &wg) != 1)
			return MSG_FAILURE;
		DESKDEFSET(window_gap, wg)
#undef DESKDEFSET
#define MONDESKSET(k, v) \
		if (loc.desktop != NULL) \
			loc.desktop->k = v; \
		else if (loc.monitor != NULL) \
			loc.monitor->k = v; \
		else \
			for (monitor_t *m = mon_head; m != NULL; m = m->next) \
				m->k = v;
	} else if (streq("top_padding", name)) {
		int tp;
		if (sscanf(value, "%i", &tp) != 1)
			return MSG_FAILURE;
		MONDESKSET(top_padding, tp)
	} else if (streq("right_padding", name)) {
		int rp;
		if (sscanf(value, "%i", &rp) != 1)
			return MSG_FAILURE;
		MONDESKSET(right_padding, rp)
	} else if (streq("bottom_padding", name)) {
		int bp;
		if (sscanf(value, "%i", &bp) != 1)
			return MSG_FAILURE;
		MONDESKSET(bottom_padding, bp)
	} else if (streq("left_padding", name)) {
		int lp;
		if (sscanf(value, "%i", &lp) != 1)
			return MSG_FAILURE;
		MONDESKSET(left_padding, lp)
#undef MONDESKSET
#define SETSTR(s) \
	} else if (streq(#s, name)) { \
		return snprintf(s, sizeof(s), "%s", value) >= 0;
	SETSTR(external_rules_command)
	SETSTR(status_prefix)
#undef SETSTR
	} else if (streq("split_ratio", name)) {
		double r;
		if (sscanf(value, "%lf", &r) == 1 && r > 0 && r < 1)
			split_ratio = r;
		else
			return MSG_FAILURE;
		return MSG_SUCCESS;
#define SETCOLOR(s) \
	} else if (streq(#s, name)) { \
		snprintf(s, sizeof(s), "%s", value);
	SETCOLOR(focused_border_color)
	SETCOLOR(active_border_color)
	SETCOLOR(normal_border_color)
	SETCOLOR(presel_border_color)
	SETCOLOR(focused_locked_border_color)
	SETCOLOR(active_locked_border_color)
	SETCOLOR(normal_locked_border_color)
	SETCOLOR(focused_sticky_border_color)
	SETCOLOR(active_sticky_border_color)
	SETCOLOR(normal_sticky_border_color)
	SETCOLOR(focused_private_border_color)
	SETCOLOR(active_private_border_color)
	SETCOLOR(normal_private_border_color)
	SETCOLOR(urgent_border_color)
#undef SETCOLOR
	} else if (streq("focus_follows_pointer", name)) {
		bool b;
		if (parse_bool(value, &b) && b != focus_follows_pointer) {
			focus_follows_pointer = b;
			for (monitor_t *m = mon_head; m != NULL; m = m->next)
				for (desktop_t *d = m->desk_head; d != NULL; d = d->next)
					for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
						uint32_t values[] = {CLIENT_EVENT_MASK | (focus_follows_pointer ? XCB_EVENT_MASK_ENTER_WINDOW : 0)};
						xcb_change_window_attributes(dpy, n->client->window, XCB_CW_EVENT_MASK, values);
					}
			if (focus_follows_pointer) {
				for (monitor_t *m = mon_head; m != NULL; m = m->next)
					window_show(m->root);
				enable_motion_recorder();
			} else {
				for (monitor_t *m = mon_head; m != NULL; m = m->next)
					window_hide(m->root);
				disable_motion_recorder();
			}
			return MSG_SUCCESS;
		} else {
			return MSG_FAILURE;
		}
#define SETBOOL(s) \
	} else if (streq(#s, name)) { \
		if (!parse_bool(value, &s)) \
			return MSG_FAILURE;
		SETBOOL(borderless_monocle)
		SETBOOL(gapless_monocle)
		SETBOOL(pointer_follows_monitor)
		SETBOOL(apply_floating_atom)
		SETBOOL(auto_alternate)
		SETBOOL(auto_cancel)
		SETBOOL(history_aware_focus)
		SETBOOL(focus_by_distance)
		SETBOOL(ignore_ewmh_focus)
#undef SETBOOL
#define SETMONBOOL(s) \
	} else if (streq(#s, name)) { \
		if (!parse_bool(value, &s)) \
			return MSG_FAILURE; \
		if (s) \
			update_monitors();
		SETMONBOOL(remove_disabled_monitors)
		SETMONBOOL(remove_unplugged_monitors)
		SETMONBOOL(merge_overlapping_monitors)
#undef SETMONBOOL
	} else {
		return MSG_FAILURE;
	}

	for (monitor_t *m = mon_head; m != NULL; m = m->next)
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next)
			arrange(m, d);

	return MSG_SUCCESS;
}

int get_setting(coordinates_t loc, char *name, FILE* rsp)
{
	if (streq("split_ratio", name))
		fprintf(rsp, "%lf", split_ratio);
	else if (streq("window_gap", name))
		if (loc.desktop != NULL)
			fprintf(rsp, "%i", loc.desktop->window_gap);
		else
			fprintf(rsp, "%i", window_gap);
	else if (streq("border_width", name))
		if (loc.node != NULL)
			fprintf(rsp, "%u", loc.node->client->border_width);
		else if (loc.desktop != NULL)
			fprintf(rsp, "%u", loc.desktop->border_width);
		else
			fprintf(rsp, "%u", border_width);
	else if (streq("external_rules_command", name))
		fprintf(rsp, "%s", external_rules_command);
	else if (streq("status_prefix", name))
		fprintf(rsp, "%s", status_prefix);
#define MONDESKGET(k) \
	else if (streq(#k, name)) \
		if (loc.desktop != NULL) \
			fprintf(rsp, "%i", loc.desktop->k); \
		else if (loc.monitor != NULL) \
			fprintf(rsp, "%i", loc.monitor->k); \
		else \
			return MSG_FAILURE;
	MONDESKGET(top_padding)
	MONDESKGET(right_padding)
	MONDESKGET(bottom_padding)
	MONDESKGET(left_padding)
#undef DESKGET
#define GETCOLOR(s) \
	else if (streq(#s, name)) \
		fprintf(rsp, "%s", s);
	GETCOLOR(focused_border_color)
	GETCOLOR(active_border_color)
	GETCOLOR(normal_border_color)
	GETCOLOR(presel_border_color)
	GETCOLOR(focused_locked_border_color)
	GETCOLOR(active_locked_border_color)
	GETCOLOR(normal_locked_border_color)
	GETCOLOR(focused_sticky_border_color)
	GETCOLOR(active_sticky_border_color)
	GETCOLOR(normal_sticky_border_color)
	GETCOLOR(urgent_border_color)
#undef GETCOLOR
#define GETBOOL(s) \
	else if (streq(#s, name)) \
		fprintf(rsp, "%s", BOOLSTR(s));
	GETBOOL(borderless_monocle)
	GETBOOL(gapless_monocle)
	GETBOOL(focus_follows_pointer)
	GETBOOL(pointer_follows_monitor)
	GETBOOL(apply_floating_atom)
	GETBOOL(auto_alternate)
	GETBOOL(auto_cancel)
	GETBOOL(history_aware_focus)
	GETBOOL(focus_by_distance)
	GETBOOL(ignore_ewmh_focus)
	GETBOOL(remove_disabled_monitors)
	GETBOOL(remove_unplugged_monitors)
	GETBOOL(merge_overlapping_monitors)
#undef GETBOOL
	else
		return MSG_FAILURE;
	return MSG_SUCCESS;
}

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

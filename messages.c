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
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "bspwm.h"
#include "desktop.h"
#include "monitor.h"
#include "pointer.h"
#include "query.h"
#include "rule.h"
#include "restore.h"
#include "settings.h"
#include "tree.h"
#include "window.h"
#include "common.h"
#include "parse.h"
#include "messages.h"

int handle_message(char *msg, int msg_len, FILE *rsp)
{
	int cap = INIT_CAP;
	int num = 0;
	char **args = malloc(cap * sizeof(char *));
	if (args == NULL) {
		return MSG_FAILURE;
	}

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
	if (streq("node", *args)) {
		return cmd_node(++args, --num);
	} else if (streq("desktop", *args)) {
		return cmd_desktop(++args, --num);
	} else if (streq("monitor", *args)) {
		return cmd_monitor(++args, --num);
	} else if (streq("query", *args)) {
		return cmd_query(++args, --num, rsp);
	} else if (streq("subscribe", *args)) {
		return cmd_subscribe(++args, --num, rsp);
	} else if (streq("wm", *args)) {
		return cmd_wm(++args, --num, rsp);
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

int cmd_node(char **args, int num)
{
	if (num < 1) {
		return MSG_SYNTAX;
	}

	coordinates_t ref = {mon, mon->desk, mon->desk->focus};
	coordinates_t trg = ref;

	if ((*args)[0] != OPT_CHR) {
		if (node_from_desc(*args, &ref, &trg)) {
			num--, args++;
		} else {
			return MSG_FAILURE;
		}
	}

	if (trg.node == NULL) {
		return MSG_FAILURE;
	}

	bool dirty = false;

	while (num > 0) {
		if (streq("-f", *args) || streq("--focus", *args)) {
			coordinates_t dst = trg;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if (!node_from_desc(*args, &trg, &dst)) {
					return MSG_FAILURE;
				}
			}
			focus_node(dst.monitor, dst.desktop, dst.node);
		} else if (streq("-a", *args) || streq("--activate", *args)) {
			coordinates_t dst = trg;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if (!node_from_desc(*args, &trg, &dst)) {
					return MSG_FAILURE;
				}
			}
			if (dst.desktop == mon->desk) {
				return MSG_FAILURE;
			}
			activate_node(dst.monitor, dst.desktop, dst.node);
		} else if (streq("-d", *args) || streq("--to-desktop", *args)) {
			num--, args++;
			coordinates_t dst;
			if (desktop_from_desc(*args, &trg, &dst)) {
				if (transfer_node(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.desktop, dst.desktop->focus)) {
					trg.monitor = dst.monitor;
					trg.desktop = dst.desktop;
				} else {
					return MSG_FAILURE;
				}
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-m", *args) || streq("--to-monitor", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			coordinates_t dst;
			if (monitor_from_desc(*args, &trg, &dst)) {
				if (transfer_node(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.monitor->desk, dst.monitor->desk->focus)) {
					trg.monitor = dst.monitor;
					trg.desktop = dst.monitor->desk;
				} else {
					return MSG_FAILURE;
				}
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-n", *args) || streq("--to-node", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			coordinates_t dst;
			if (node_from_desc(*args, &trg, &dst)) {
				if (transfer_node(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.desktop, dst.node)) {
					trg.monitor = dst.monitor;
					trg.desktop = dst.desktop;
				} else {
					return MSG_FAILURE;
				}
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-s", *args) || streq("--swap", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			coordinates_t dst;
			if (node_from_desc(*args, &trg, &dst)) {
				if (swap_nodes(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.desktop, dst.node)) {
					trg.monitor = dst.monitor;
					trg.desktop = dst.desktop;
				} else {
					return MSG_FAILURE;
				}
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-l", *args) || streq("--layer", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			if (trg.node->client == NULL) {
				return MSG_FAILURE;
			}
			stack_layer_t lyr;
			if (parse_stack_layer(*args, &lyr)) {
				set_layer(trg.monitor, trg.desktop, trg.node, lyr);
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-t", *args) || streq("--state", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			client_state_t cst;
			bool alternate = false;
			if ((*args)[0] == '~') {
				alternate = true;
				(*args)++;
			}
			if (parse_client_state(*args, &cst)) {
				if (trg.node->client == NULL) {
					return MSG_FAILURE;
				}
				if (alternate && trg.node->client->state == cst) {
					cst = trg.node->client->last_state;
				}
				set_state(trg.monitor, trg.desktop, trg.node, cst);
				dirty = true;
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-g", *args) || streq("--flag", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			char *key = strtok(*args, EQL_TOK);
			char *val = strtok(NULL, EQL_TOK);
			alter_state_t a;
			bool b;
			if (val == NULL) {
				a = ALTER_TOGGLE;
			} else {
				if (parse_bool(val, &b)) {
					a = ALTER_SET;
				} else {
					return MSG_FAILURE;
				}
			}
			if (streq("locked", key)) {
				set_locked(trg.monitor, trg.desktop, trg.node, (a == ALTER_SET ? b : !trg.node->locked));
			} else if (streq("sticky", key)) {
				set_sticky(trg.monitor, trg.desktop, trg.node, (a == ALTER_SET ? b : !trg.node->sticky));
			} else if (streq("private", key)) {
				set_private(trg.monitor, trg.desktop, trg.node, (a == ALTER_SET ? b : !trg.node->private));
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-p", *args) || streq("--presel-dir", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			if (trg.node->vacant) {
				return MSG_FAILURE;
			}
			if (streq("cancel", *args)) {
				cancel_presel(trg.monitor, trg.desktop, trg.node);
			} else {
				bool alternate = false;
				if ((*args)[0] == '~') {
					alternate = true;
					(*args)++;
				}
				direction_t dir;
				if (parse_direction(*args, &dir)) {
					if (alternate && trg.node->presel != NULL && trg.node->presel->split_dir == dir) {
						cancel_presel(trg.monitor, trg.desktop, trg.node);
					} else {
						presel_dir(trg.monitor, trg.desktop, trg.node, dir);
						draw_presel_feedback(trg.monitor, trg.desktop, trg.node);
					}
				} else {
					return MSG_FAILURE;
				}
			}
		} else if (streq("-o", *args) || streq("--presel-ratio", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			if (trg.node->vacant) {
				return MSG_FAILURE;
			}
			double rat;
			if (sscanf(*args, "%lf", &rat) != 1 || rat <= 0 || rat >= 1) {
				return MSG_FAILURE;
			} else {
				presel_ratio(trg.monitor, trg.desktop, trg.node, rat);
				draw_presel_feedback(trg.monitor, trg.desktop, trg.node);
			}
		} else if (streq("-r", *args) || streq("--ratio", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			if ((*args)[0] == '+' || (*args)[0] == '-') {
				int pix;
				if (sscanf(*args, "%i", &pix) == 1) {
					int max = (trg.node->split_type == TYPE_HORIZONTAL ? trg.node->rectangle.height : trg.node->rectangle.width);
					double rat = ((max * trg.node->split_ratio) + pix) / max;
					if (rat > 0 && rat < 1) {
						trg.node->split_ratio = rat;
					} else {
						return MSG_FAILURE;
					}
				} else {
					return MSG_FAILURE;
				}
			} else {
				double rat;
				if (sscanf(*args, "%lf", &rat) == 1 && rat > 0 && rat < 1) {
					trg.node->split_ratio = rat;
				} else {
					return MSG_FAILURE;
				}
			}
			dirty = true;
		} else if (streq("-F", *args) || streq("--flip", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			flip_t flp;
			if (parse_flip(*args, &flp)) {
				flip_tree(trg.node, flp);
				dirty = true;
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-R", *args) || streq("--rotate", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			int deg;
			if (parse_degree(*args, &deg)) {
				rotate_tree(trg.node, deg);
				dirty = true;
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-E", *args) || streq("--equalize", *args)) {
			equalize_tree(trg.node);
			dirty = true;
		} else if (streq("-B", *args) || streq("--balance", *args)) {
			balance_tree(trg.node);
			dirty = true;
		} else if (streq("-C", *args) || streq("--circulate", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			circulate_dir_t cir;
			if (parse_circulate_direction(*args, &cir)) {
				circulate_leaves(trg.monitor, trg.desktop, trg.node, cir);
				dirty = true;
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-c", *args) || streq("--close", *args)) {
			if (num > 1) {
				return MSG_SYNTAX;
			}
			if (locked_count(trg.node) > 0) {
				return MSG_FAILURE;
			}
			window_close(trg.node);
		} else if (streq("-k", *args) || streq("--kill", *args)) {
			if (num > 1) {
				return MSG_SYNTAX;
			}
			window_kill(trg.monitor, trg.desktop, trg.node);
			dirty = true;
		} else {
			return MSG_SYNTAX;
		}

		num--, args++;
	}

	if (dirty) {
		arrange(trg.monitor, trg.desktop);
	}

	return MSG_SUCCESS;
}

int cmd_desktop(char **args, int num)
{
	if (num < 1) {
		return MSG_SYNTAX;
	}

	coordinates_t ref = {mon, mon->desk, NULL};
	coordinates_t trg = ref;

	if ((*args)[0] != OPT_CHR) {
		if (desktop_from_desc(*args, &ref, &trg)) {
			num--, args++;
		} else {
			return MSG_FAILURE;
		}
	}

	bool dirty = false;

	while (num > 0) {
		if (streq("-f", *args) || streq("--focus", *args)) {
			coordinates_t dst = trg;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if (!desktop_from_desc(*args, &trg, &dst)) {
					return MSG_FAILURE;
				}
			}
			focus_node(dst.monitor, dst.desktop, dst.desktop->focus);
		} else if (streq("-a", *args) || streq("--activate", *args)) {
			coordinates_t dst = trg;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if (!desktop_from_desc(*args, &trg, &dst)) {
					return MSG_FAILURE;
				}
			}
			activate_desktop(dst.monitor, dst.desktop);
		} else if (streq("-m", *args) || streq("--to-monitor", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			if (trg.monitor->desk_head == trg.monitor->desk_tail) {
				return MSG_FAILURE;
			}
			coordinates_t dst;
			if (monitor_from_desc(*args, &trg, &dst)) {
				if (transfer_desktop(trg.monitor, dst.monitor, trg.desktop)) {
					trg.monitor = dst.monitor;
				} else {
					return MSG_FAILURE;
				}
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-s", *args) || streq("--swap", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			coordinates_t dst;
			if (desktop_from_desc(*args, &trg, &dst)) {
				if (swap_desktops(trg.monitor, trg.desktop, dst.monitor, dst.desktop)) {
					trg.monitor = dst.monitor;
				} else {
					return MSG_FAILURE;
				}
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-b", *args) || streq("--bubble", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			cycle_dir_t cyc;
			if (parse_cycle_direction(*args, &cyc)) {
				desktop_t *d = trg.desktop;
				if (cyc == CYCLE_PREV) {
					if (d->prev == NULL) {
						while (d->next != NULL) {
							swap_desktops(trg.monitor, d, trg.monitor, d->next);
						}
					} else {
						swap_desktops(trg.monitor, d, trg.monitor, d->prev);
					}
				} else {
					if (d->next == NULL) {
						while (d->prev != NULL) {
							swap_desktops(trg.monitor, d, trg.monitor, d->prev);
						}
					} else {
						swap_desktops(trg.monitor, d, trg.monitor, d->next);
					}
				}
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-l", *args) || streq("--layout", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			layout_t lyt;
			cycle_dir_t cyc;
			if (parse_cycle_direction(*args, &cyc)) {
				change_layout(trg.monitor, trg.desktop, (trg.desktop->layout + 1) % 2);
			} else if (parse_layout(*args, &lyt)) {
				change_layout(trg.monitor, trg.desktop, lyt);
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-n", *args) || streq("--rename", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			rename_desktop(trg.monitor, trg.desktop, *args);
		} else if (streq("-r", *args) || streq("--remove", *args)) {
			if (trg.desktop->root == NULL &&
			    trg.monitor->desk_head != trg.monitor->desk_tail) {
				remove_desktop(trg.monitor, trg.desktop);
				return MSG_SUCCESS;
			} else {
				return MSG_FAILURE;
			}
		}
		num--, args++;
	}

	if (dirty) {
		arrange(trg.monitor, trg.desktop);
	}

	return MSG_SUCCESS;
}

int cmd_monitor(char **args, int num)
{
	if (num < 1) {
		return MSG_SYNTAX;
	}

	coordinates_t ref = {mon, NULL, NULL};
	coordinates_t trg = ref;

	if ((*args)[0] != OPT_CHR) {
		if (monitor_from_desc(*args, &ref, &trg)) {
			num--, args++;
		} else {
			return MSG_FAILURE;
		}
	}

	while (num > 0) {
		if (streq("-f", *args) || streq("--focus", *args)) {
			coordinates_t dst = trg;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if (!monitor_from_desc(*args, &trg, &dst)) {
					return MSG_FAILURE;
				}
			}
			focus_node(dst.monitor, dst.monitor->desk, dst.monitor->desk->focus);
		} else if (streq("-d", *args) || streq("--reset-desktops", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			desktop_t *d = trg.monitor->desk_head;
			while (num > 0 && d != NULL) {
				rename_desktop(trg.monitor, d, *args);
				initialize_desktop(d);
				arrange(trg.monitor, d);
				d = d->next;
				num--, args++;
			}
			put_status(SBSC_MASK_REPORT);
			while (num > 0) {
				add_desktop(trg.monitor, make_desktop(*args));
				num--, args++;
			}
			while (d != NULL) {
				desktop_t *next = d->next;
				if (d == mon->desk) {
					focus_node(trg.monitor, d->prev, d->prev->focus);
				}
				merge_desktops(trg.monitor, d, mon, mon->desk);
				remove_desktop(trg.monitor, d);
				d = next;
			}
		} else if (streq("-a", *args) || streq("--add-desktops", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			while (num > 0) {
				add_desktop(trg.monitor, make_desktop(*args));
				num--, args++;
			}
		} else if (streq("-r", *args) || streq("--remove-desktops", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
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
			if (num < 1) {
				return MSG_SYNTAX;
			}
			desktop_t *d = trg.monitor->desk_head;
			while (d != NULL && num > 0) {
				desktop_t *next = d->next;
				coordinates_t dst;
				if (locate_desktop(*args, &dst) && dst.monitor == trg.monitor) {
					swap_desktops(trg.monitor, d, dst.monitor, dst.desktop);
					if (next == dst.desktop) {
						next = d;
					}
				}
				d = next;
				num--, args++;
			}
		} else if (streq("-n", *args) || streq("--rename", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			rename_monitor(trg.monitor, *args);
		} else if (streq("-s", *args) || streq("--swap", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			coordinates_t dst;
			if (monitor_from_desc(*args, &trg, &dst)) {
				swap_monitors(trg.monitor, dst.monitor);
			} else {
				return MSG_FAILURE;
			}
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
	monitor_select_t *monitor_sel = NULL;
	desktop_select_t *desktop_sel = NULL;
	node_select_t *node_sel = NULL;
	domain_t dom = DOMAIN_TREE;
	int d = 0, t = 0, ret = MSG_SUCCESS;

	while (num > 0) {
		if (streq("-T", *args) || streq("--tree", *args)) {
			dom = DOMAIN_TREE, d++;
		} else if (streq("-M", *args) || streq("--monitors", *args)) {
			dom = DOMAIN_MONITOR, d++;
		} else if (streq("-D", *args) || streq("--desktops", *args)) {
			dom = DOMAIN_DESKTOP, d++;
		} else if (streq("-N", *args) || streq("--nodes", *args)) {
			dom = DOMAIN_NODE, d++;
		} else if (streq("-m", *args) || streq("--monitor", *args)) {
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if ((*args)[0] == '.') {
					monitor_sel = malloc(sizeof(monitor_select_t));
					*monitor_sel = make_monitor_select();
					if (!parse_monitor_modifiers(*args, monitor_sel)) {
						ret = MSG_FAILURE;
						goto end;
					}
				} else if (!monitor_from_desc(*args, &ref, &trg)) {
					ret = MSG_FAILURE;
					goto end;
				}
			} else {
				trg.monitor = ref.monitor;
			}
			t++;
		} else if (streq("-d", *args) || streq("--desktop", *args)) {
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if ((*args)[0] == '.') {
					desktop_sel = malloc(sizeof(desktop_select_t));
					*desktop_sel = make_desktop_select();
					if (!parse_desktop_modifiers(*args, desktop_sel)) {
						ret = MSG_FAILURE;
						goto end;
					}
				} else if (!desktop_from_desc(*args, &ref, &trg)) {
					ret = MSG_FAILURE;
					goto end;
				}
			} else {
				trg.monitor = ref.monitor;
				trg.desktop = ref.desktop;
			}
			t++;
		} else if (streq("-n", *args) || streq("--node", *args)) {
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				if ((*args)[0] == '.') {
					node_sel = malloc(sizeof(node_select_t));
					*node_sel = make_node_select();
					if (!parse_node_modifiers(*args, node_sel)) {
						ret = MSG_FAILURE;
						goto end;
					}
				} else if (!node_from_desc(*args, &ref, &trg)) {
					ret = MSG_FAILURE;
					goto end;
				}
			} else {
				trg = ref;
			}
			t++;
		} else {
			ret = MSG_SYNTAX;
			goto end;
		}
		num--, args++;
	}

	if (d != 1 || t > 1) {
		ret = MSG_SYNTAX;
		goto end;
	}

	if (dom == DOMAIN_NODE) {
		query_node_ids(trg, node_sel, rsp);
	} else if (dom == DOMAIN_DESKTOP) {
		query_desktop_names(trg, desktop_sel, rsp);
	} else if (dom == DOMAIN_MONITOR) {
		query_monitor_names(trg, monitor_sel, rsp);
	} else {
		if (trg.node != NULL) {
			query_node(trg.node, rsp);
		} else if (trg.desktop != NULL) {
			query_desktop(trg.desktop, rsp);
		} else if (trg.monitor != NULL) {
			query_monitor(trg.monitor, rsp);
		} else {
			ret = MSG_SYNTAX;
			goto end;
		}
		fprintf(rsp, "\n");
	}

end:
	free(monitor_sel);
	free(desktop_sel);
	free(node_sel);

	return ret;
}

int cmd_rule(char **args, int num, FILE *rsp)
{
	if (num < 1) {
		return MSG_SYNTAX;
	}

	while (num > 0) {
		if (streq("-a", *args) || streq("--add", *args)) {
			num--, args++;
			if (num < 2) {
				return MSG_SYNTAX;
			}
			rule_t *rule = make_rule();
			char *class_name = strtok(*args, COL_TOK);
			char *instance_name = strtok(NULL, COL_TOK);
			snprintf(rule->class_name, sizeof(rule->class_name), "%s", class_name);
			snprintf(rule->instance_name, sizeof(rule->instance_name), "%s", instance_name==NULL?MATCH_ANY:instance_name);
			num--, args++;
			size_t i = 0;
			while (num > 0) {
				if (streq("-o", *args) || streq("--one-shot", *args)) {
					rule->one_shot = true;
				} else {
					for (size_t j = 0; i < sizeof(rule->effect) && j < strlen(*args); i++, j++) {
						rule->effect[i] = (*args)[j];
					}
					if (num > 1 && i < sizeof(rule->effect)) {
						rule->effect[i++] = ' ';
					}

				}
				num--, args++;
			}
			rule->effect[MIN(i, sizeof(rule->effect) - 1)] = '\0';
			add_rule(rule);
		} else if (streq("-r", *args) || streq("--remove", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			int idx;
			while (num > 0) {
				if (parse_index(*args, &idx)) {
					remove_rule_by_index(idx - 1);
				} else if (streq("tail", *args)) {
					remove_rule(rule_tail);
				} else if (streq("head", *args)) {
					remove_rule(rule_head);
				} else {
					remove_rule_by_cause(*args);
				}
				num--, args++;
			}
		} else if (streq("-l", *args) || streq("--list", *args)) {
			num--, args++;
			list_rules(rsp);
		} else {
			return MSG_SYNTAX;
		}
		num--, args++;
	}

	return MSG_SUCCESS;
}

int cmd_pointer(char **args, int num)
{
	if (num < 1) {
		return MSG_SYNTAX;
	}
	while (num > 0) {
		if (streq("-t", *args) || streq("--track", *args)) {
			num--, args++;
			if (num < 2) {
				return MSG_SYNTAX;
			}
			int x, y;
			if (sscanf(*args, "%i", &x) == 1 && sscanf(*(args + 1), "%i", &y) == 1) {
				track_pointer(x, y);
			} else {
				return MSG_FAILURE;
			}
			num--, args++;
		} else if (streq("-g", *args) || streq("--grab", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			pointer_action_t pac;
			if (parse_pointer_action(*args, &pac)) {
				grab_pointer(pac);
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-u", *args) || streq("--ungrab", *args)) {
			ungrab_pointer();
		} else {
			return MSG_SYNTAX;
		}
		num--, args++;
	}

	return MSG_SUCCESS;
}

int cmd_wm(char **args, int num, FILE *rsp)
{
	if (num < 1) {
		return MSG_SYNTAX;
	}
	while (num > 0) {
		if (streq("-d", *args) || streq("--dump-state", *args)) {
			query_tree(rsp);
			fprintf(rsp, "\n");
		} else if (streq("-l", *args) || streq("--load-state", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			if (!restore_tree(*args)) {
				return MSG_FAILURE;
			}
		} else if (streq("-a", *args) || streq("--add-monitor", *args)) {
			num--, args++;
			if (num < 2) {
				return MSG_SYNTAX;
			}
			char *name = *args;
			num--, args++;
			xcb_rectangle_t r;
			if (parse_rectangle(*args, &r)) {
				monitor_t *m = make_monitor(&r);
				snprintf(m->name, sizeof(m->name), "%s", name);
				add_monitor(m);
				add_desktop(m, make_desktop(NULL));
			} else {
				return MSG_SYNTAX;
			}
		} else if (streq("-r", *args) || streq("--remove-monitor", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			if (mon_head == mon_tail) {
				return MSG_FAILURE;
			}
			monitor_t *m = find_monitor(*args);
			if (m != NULL) {
				remove_monitor(m);
			} else {
				return MSG_FAILURE;
			}
		} else if (streq("-o", *args) || streq("--adopt-orphans", *args)) {
			adopt_orphans();
		} else if (streq("-g", *args) || streq("--get-status", *args)) {
			print_report(rsp);
		} else if (streq("-h", *args) || streq("--record-history", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			bool b;
			if (parse_bool(*args, &b)) {
				record_history = b;
			} else {
				return MSG_SYNTAX;
			}
		} else {
			return MSG_SYNTAX;
		}
		num--, args++;
	}

	return MSG_SUCCESS;
}

int cmd_config(char **args, int num, FILE *rsp)
{
	if (num < 1) {
		return MSG_SYNTAX;
	}

	coordinates_t ref = {mon, mon->desk, mon->desk->focus};
	coordinates_t trg = {NULL, NULL, NULL};

	if ((*args)[0] == OPT_CHR) {
		if (streq("-m", *args) || streq("--monitor", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			if (!monitor_from_desc(*args, &ref, &trg)) {
				return MSG_FAILURE;
			}
		} else if (streq("-d", *args) || streq("--desktop", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			if (!desktop_from_desc(*args, &ref, &trg)) {
				return MSG_FAILURE;
			}
		} else if (streq("-n", *args) || streq("--node", *args)) {
			num--, args++;
			if (num < 1) {
				return MSG_SYNTAX;
			}
			if (!node_from_desc(*args, &ref, &trg)) {
				return MSG_FAILURE;
			}
		} else {
			return MSG_SYNTAX;
		}
		num--, args++;
	}
	if (num == 2) {
		return set_setting(trg, *args, *(args + 1));
	} else if (num == 1) {
		return get_setting(trg, *args, rsp);
	} else {
		return MSG_SYNTAX;
	}
}

int cmd_subscribe(char **args, int num, FILE *rsp)
{
	int field = 0;
	if (num < 1) {
		field = SBSC_MASK_REPORT;
	} else {
		subscriber_mask_t mask;
		while (num > 0) {
			if (parse_subscriber_mask(*args, &mask)) {
				field |= mask;
			} else {
				return MSG_SYNTAX;
			}
			num--, args++;
		}
	}

	add_subscriber(rsp, field);
	return MSG_SUBSCRIBE;
}

int cmd_quit(char **args, int num)
{
	if (num > 0 && sscanf(*args, "%i", &exit_status) != 1) {
		return MSG_SYNTAX;
	}
	running = false;
	return MSG_SUCCESS;
}

int set_setting(coordinates_t loc, char *name, char *value)
{
	bool colors_changed = false;
#define DESK_WIN_DEF_SET(k, v) \
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
		DESK_WIN_DEF_SET(border_width, bw)
#undef DESK_WIN_DEF_SET
#define DESK_DEF_SET(k, v) \
		if (loc.desktop != NULL) \
			loc.desktop->k = v; \
		else if (loc.monitor != NULL) \
			return MSG_SYNTAX; \
		else \
			k = v;
	} else if (streq("window_gap", name)) {
		int wg;
		if (sscanf(value, "%i", &wg) != 1)
			return MSG_FAILURE;
		DESK_DEF_SET(window_gap, wg)
#undef DESK_DEF_SET
#define MON_DESK_SET(k, v) \
		if (loc.desktop != NULL) \
			loc.desktop->k = v; \
		else if (loc.monitor != NULL) \
			loc.monitor->k = v; \
		else \
			for (monitor_t *m = mon_head; m != NULL; m = m->next) \
				m->k = v;
	} else if (streq("top_padding", name)) {
		int tp;
		if (sscanf(value, "%i", &tp) != 1) {
			return MSG_FAILURE;
		}
		MON_DESK_SET(top_padding, tp)
	} else if (streq("right_padding", name)) {
		int rp;
		if (sscanf(value, "%i", &rp) != 1) {
			return MSG_FAILURE;
		}
		MON_DESK_SET(right_padding, rp)
	} else if (streq("bottom_padding", name)) {
		int bp;
		if (sscanf(value, "%i", &bp) != 1) {
			return MSG_FAILURE;
		}
		MON_DESK_SET(bottom_padding, bp)
	} else if (streq("left_padding", name)) {
		int lp;
		if (sscanf(value, "%i", &lp) != 1) {
			return MSG_FAILURE;
		}
		MON_DESK_SET(left_padding, lp)
#undef MON_DESK_SET
#define SET_STR(s) \
	} else if (streq(#s, name)) { \
		if (snprintf(s, sizeof(s), "%s", value) < 0) \
			return MSG_FAILURE;
	SET_STR(external_rules_command)
	SET_STR(status_prefix)
#undef SET_STR
	} else if (streq("split_ratio", name)) {
		double r;
		if (sscanf(value, "%lf", &r) == 1 && r > 0 && r < 1) {
			split_ratio = r;
		} else {
			return MSG_FAILURE;
		}
		return MSG_SUCCESS;
#define SET_COLOR(s) \
	} else if (streq(#s, name)) { \
		if (!is_hex_color(value)) { \
			return MSG_FAILURE; \
		} else { \
			snprintf(s, sizeof(s), "%s", value); \
		colors_changed = true; \
		}
	SET_COLOR(normal_border_color)
	SET_COLOR(active_border_color)
	SET_COLOR(focused_border_color)
	SET_COLOR(presel_feedback_color)
#undef SET_COLOR
	} else if (streq("initial_polarity", name)) {
		child_polarity_t p;
		if (parse_child_polarity(value, &p)) {
			initial_polarity = p;
		} else {
			return MSG_FAILURE;
		}
	} else if (streq("focus_follows_pointer", name)) {
		bool b;
		if (parse_bool(value, &b)) {
			if (b == focus_follows_pointer) {
				return MSG_SUCCESS;
			}
			focus_follows_pointer = b;
			uint32_t values[] = {CLIENT_EVENT_MASK | (focus_follows_pointer ? XCB_EVENT_MASK_ENTER_WINDOW : 0)};
			for (monitor_t *m = mon_head; m != NULL; m = m->next) {
				for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
					for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
						xcb_change_window_attributes(dpy, n->id, XCB_CW_EVENT_MASK, values);
					}
				}
			}
			if (focus_follows_pointer) {
				for (monitor_t *m = mon_head; m != NULL; m = m->next) {
					window_show(m->root);
				}
			} else {
				for (monitor_t *m = mon_head; m != NULL; m = m->next) {
					window_hide(m->root);
				}
				disable_motion_recorder();
			}
			return MSG_SUCCESS;
		} else {
			return MSG_FAILURE;
		}
#define SET_BOOL(s) \
	} else if (streq(#s, name)) { \
		if (!parse_bool(value, &s)) \
			return MSG_FAILURE;
		SET_BOOL(borderless_monocle)
		SET_BOOL(gapless_monocle)
		SET_BOOL(single_monocle)
		SET_BOOL(pointer_follows_focus)
		SET_BOOL(pointer_follows_monitor)
		SET_BOOL(history_aware_focus)
		SET_BOOL(focus_by_distance)
		SET_BOOL(ignore_ewmh_focus)
		SET_BOOL(center_pseudo_tiled)
#undef SET_BOOL
#define SET_MON_BOOL(s) \
	} else if (streq(#s, name)) { \
		if (!parse_bool(value, &s)) \
			return MSG_FAILURE; \
		if (s) \
			update_monitors();
		SET_MON_BOOL(remove_disabled_monitors)
		SET_MON_BOOL(remove_unplugged_monitors)
		SET_MON_BOOL(merge_overlapping_monitors)
#undef SET_MON_BOOL
	} else {
		return MSG_FAILURE;
	}

	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			arrange(m, d);
			if (colors_changed) {
				update_colors_in(d->root, d, m);
			}
		}
	}

	return MSG_SUCCESS;
}

int get_setting(coordinates_t loc, char *name, FILE* rsp)
{
	if (streq("split_ratio", name))
		fprintf(rsp, "%lf", split_ratio);
	else if (streq("window_gap", name))
		if (loc.desktop != NULL)
			fprintf(rsp, "%i", loc.desktop->window_gap);
		else if (loc.monitor != NULL)
			return MSG_SYNTAX;
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
	else if (streq("initial_polarity", name))
		fprintf(rsp, "%s", initial_polarity == FIRST_CHILD ? "first_child" : "second_child");
#define MON_DESK_GET(k) \
	else if (streq(#k, name)) \
		if (loc.desktop != NULL) \
			fprintf(rsp, "%i", loc.desktop->k); \
		else if (loc.monitor != NULL) \
			fprintf(rsp, "%i", loc.monitor->k); \
		else \
			return MSG_FAILURE;
	MON_DESK_GET(top_padding)
	MON_DESK_GET(right_padding)
	MON_DESK_GET(bottom_padding)
	MON_DESK_GET(left_padding)
#undef DESKGET
#define GET_COLOR(s) \
	else if (streq(#s, name)) \
		fprintf(rsp, "%s", s);
	GET_COLOR(normal_border_color)
	GET_COLOR(active_border_color)
	GET_COLOR(focused_border_color)
	GET_COLOR(presel_feedback_color)
#undef GET_COLOR
#define GET_BOOL(s) \
	else if (streq(#s, name)) \
		fprintf(rsp, "%s", BOOL_STR(s));
	GET_BOOL(borderless_monocle)
	GET_BOOL(gapless_monocle)
	GET_BOOL(single_monocle)
	GET_BOOL(focus_follows_pointer)
	GET_BOOL(pointer_follows_focus)
	GET_BOOL(pointer_follows_monitor)
	GET_BOOL(history_aware_focus)
	GET_BOOL(focus_by_distance)
	GET_BOOL(ignore_ewmh_focus)
	GET_BOOL(center_pseudo_tiled)
	GET_BOOL(remove_disabled_monitors)
	GET_BOOL(remove_unplugged_monitors)
	GET_BOOL(merge_overlapping_monitors)
#undef GET_BOOL
	else
		return MSG_FAILURE;
	fprintf(rsp, "\n");
	return MSG_SUCCESS;
}

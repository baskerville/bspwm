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
#include <stdarg.h>
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

void handle_message(char *msg, int msg_len, FILE *rsp)
{
	int cap = INIT_CAP;
	int num = 0;
	char **args = malloc(cap * sizeof(char *));

	if (args == NULL) {
		perror("Handle message: malloc");
		return;
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
				perror("Handle message: realloc");
				return;
			} else {
				args = new;
			}
		}
	}

	if (num < 1) {
		free(args);
		fail(rsp, "No arguments given.\n");
		return;
	}

	char **args_orig = args;
	process_message(args, num, rsp);
	free(args_orig);
}

void process_message(char **args, int num, FILE *rsp)
{
	int ret = SUBSCRIBE_FAILURE;

	if (streq("node", *args)) {
		cmd_node(++args, --num, rsp);
	} else if (streq("desktop", *args)) {
		cmd_desktop(++args, --num, rsp);
	} else if (streq("monitor", *args)) {
		cmd_monitor(++args, --num, rsp);
	} else if (streq("query", *args)) {
		cmd_query(++args, --num, rsp);
	} else if (streq("subscribe", *args)) {
		ret = cmd_subscribe(++args, --num, rsp);
	} else if (streq("wm", *args)) {
		cmd_wm(++args, --num, rsp);
	} else if (streq("rule", *args)) {
		cmd_rule(++args, --num, rsp);
	} else if (streq("config", *args)) {
		cmd_config(++args, --num, rsp);
	} else if (streq("quit", *args)) {
		cmd_quit(++args, --num, rsp);
	} else {
		fail(rsp, "Unknown domain or command: '%s'.\n", *args);
	}

	fflush(rsp);

	if (ret != SUBSCRIBE_SUCCESS) {
		fclose(rsp);
	}
}

void cmd_node(char **args, int num, FILE *rsp)
{
	if (num < 1) {
		fail(rsp, "node: Missing commands.\n");
		return;
	}

	coordinates_t ref = {mon, mon->desk, mon->desk->focus};
	coordinates_t trg = ref;

	if ((*args)[0] != OPT_CHR) {
		int ret;
		if ((ret = node_from_desc(*args, &ref, &trg)) == SELECTOR_OK) {
			num--, args++;
		} else {
			handle_failure(ret, "node", *args, rsp);
			return;
		}
	}

	bool changed = false;

	while (num > 0) {
		if (streq("-f", *args) || streq("--focus", *args)) {
			coordinates_t dst = trg;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				int ret;
				if ((ret = node_from_desc(*args, &trg, &dst)) != SELECTOR_OK) {
					handle_failure(ret, "node -f", *args, rsp);
					break;
				}
			}
			if (dst.node == NULL || !focus_node(dst.monitor, dst.desktop, dst.node)) {
				fail(rsp, "");
				break;
			}
		} else if (streq("-a", *args) || streq("--activate", *args)) {
			coordinates_t dst = trg;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				int ret;
				if ((ret = node_from_desc(*args, &trg, &dst)) != SELECTOR_OK) {
					handle_failure(ret, "node -a", *args, rsp);
					break;
				}
			}
			if (dst.node == NULL || !activate_node(dst.monitor, dst.desktop, dst.node)) {
				fail(rsp, "");
				break;
			}
		} else if (streq("-d", *args) || streq("--to-desktop", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			coordinates_t dst;
			int ret;
			if ((ret = desktop_from_desc(*args, &trg, &dst)) == SELECTOR_OK) {
				if (transfer_node(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.desktop, dst.desktop->focus)) {
					trg.monitor = dst.monitor;
					trg.desktop = dst.desktop;
				} else {
					fail(rsp, "");
					break;
				}
			} else {
				handle_failure(ret, "node -d", *args, rsp);
				break;
			}
		} else if (streq("-m", *args) || streq("--to-monitor", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			coordinates_t dst;
			int ret;
			if ((ret = monitor_from_desc(*args, &trg, &dst)) == SELECTOR_OK) {
				if (transfer_node(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.monitor->desk, dst.monitor->desk->focus)) {
					trg.monitor = dst.monitor;
					trg.desktop = dst.monitor->desk;
				} else {
					fail(rsp, "");
					break;
				}
			} else {
				handle_failure(ret, "node -m", *args, rsp);
				break;
			}
		} else if (streq("-n", *args) || streq("--to-node", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			coordinates_t dst;
			int ret;
			if ((ret = node_from_desc(*args, &trg, &dst)) == SELECTOR_OK) {
				if (transfer_node(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.desktop, dst.node)) {
					trg.monitor = dst.monitor;
					trg.desktop = dst.desktop;
				} else {
					fail(rsp, "");
					break;
				}
			} else {
				handle_failure(ret, "node -n", *args, rsp);
				break;
			}
		} else if (streq("-s", *args) || streq("--swap", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			coordinates_t dst;
			int ret;
			if ((ret = node_from_desc(*args, &trg, &dst)) == SELECTOR_OK) {
				if (swap_nodes(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.desktop, dst.node)) {
					trg.monitor = dst.monitor;
					trg.desktop = dst.desktop;
				} else {
					fail(rsp, "");
					break;
				}
			} else {
				handle_failure(ret, "node -s", *args, rsp);
				break;
			}
		} else if (streq("-l", *args) || streq("--layer", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			stack_layer_t lyr;
			if (parse_stack_layer(*args, &lyr)) {
				if (!set_layer(trg.monitor, trg.desktop, trg.node, lyr)) {
					fail(rsp, "");
					break;
				}
			} else {
				fail(rsp, "node %s: Invalid argument: '%s'.\n", *(args - 1), *args);
				break;
			}
		} else if (streq("-t", *args) || streq("--state", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			client_state_t cst;
			bool alternate = false;
			if ((*args)[0] == '~') {
				alternate = true;
				(*args)++;
			}
			if (parse_client_state(*args, &cst)) {
				if (alternate && trg.node != NULL && trg.node->client != NULL &&
				    trg.node->client->state == cst) {
					cst = trg.node->client->last_state;
				}
				if (!set_state(trg.monitor, trg.desktop, trg.node, cst)) {
					fail(rsp, "");
					break;
				}
				changed = true;
			} else {
				fail(rsp, "node %s: Invalid argument: '%s'.\n", *(args - 1), *args);
				break;
			}
		} else if (streq("-g", *args) || streq("--flag", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			if (trg.node == NULL) {
				fail(rsp, "");
				break;
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
					fail(rsp, "node %s: Invalid value for %s: '%s'.\n", *(args - 1), key, val);
					break;
				}
			}
			if (streq("locked", key)) {
				set_locked(trg.monitor, trg.desktop, trg.node, (a == ALTER_SET ? b : !trg.node->locked));
			} else if (streq("sticky", key)) {
				set_sticky(trg.monitor, trg.desktop, trg.node, (a == ALTER_SET ? b : !trg.node->sticky));
			} else if (streq("private", key)) {
				set_private(trg.monitor, trg.desktop, trg.node, (a == ALTER_SET ? b : !trg.node->private));
			} else {
				fail(rsp, "node %s: Invalid key: '%s'.\n", *(args - 1), key);
				break;
			}
		} else if (streq("-p", *args) || streq("--presel-dir", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			if (trg.node == NULL || trg.node->vacant) {
				fail(rsp, "");
				break;
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
						if (!IS_RECEPTACLE(trg.node)) {
							draw_presel_feedback(trg.monitor, trg.desktop, trg.node);
						}
					}
				} else {
					fail(rsp, "node %s: Invalid argument: '%s%s'.\n", *(args - 1), alternate?"~":"", *args);
					break;
				}
			}
		} else if (streq("-o", *args) || streq("--presel-ratio", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			if (trg.node == NULL || trg.node->vacant) {
				fail(rsp, "");
				break;
			}
			double rat;
			if (sscanf(*args, "%lf", &rat) != 1 || rat <= 0 || rat >= 1) {
				fail(rsp, "node %s: Invalid argument: '%s'.\n", *(args - 1), *args);
				break;
			} else {
				presel_ratio(trg.monitor, trg.desktop, trg.node, rat);
				draw_presel_feedback(trg.monitor, trg.desktop, trg.node);
			}
		} else if (streq("-v", *args) || streq("--move", *args)) {
			num--, args++;
			if (num < 2) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			int dx = 0, dy = 0;
			if (sscanf(*args, "%i", &dx) == 1) {
				num--, args++;
				if (sscanf(*args, "%i", &dy) == 1) {
					if (!move_client(&trg, dx, dy)) {
						fail(rsp, "");
						break;
					}
				} else {
					fail(rsp, "node %s: Invalid dy argument: '%s'.\n", *(args - 3), *args);
					break;
				}
			} else {
				fail(rsp, "node %s: Invalid dx argument: '%s'.\n", *(args - 2), *args);
				break;
			}
		} else if (streq("-z", *args) || streq("--resize", *args)) {
			num--, args++;
			if (num < 3) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			resize_handle_t rh;
			if (parse_resize_handle(*args, &rh)) {
				num--, args++;
				int dx = 0, dy = 0;
				if (sscanf(*args, "%i", &dx) == 1) {
					num--, args++;
					if (sscanf(*args, "%i", &dy) == 1) {
						if (!resize_client(&trg, rh, dx, dy)) {
							fail(rsp, "");
							break;
						}
					} else {
						fail(rsp, "node %s: Invalid dy argument: '%s'.\n", *(args - 3), *args);
						break;
					}
				} else {
					fail(rsp, "node %s: Invalid dx argument: '%s'.\n", *(args - 2), *args);
					break;
				}
			} else {
				fail(rsp, "node %s: Invalid resize handle argument: '%s'.\n", *(args - 1), *args);
				break;
			}
		} else if (streq("-r", *args) || streq("--ratio", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			if (trg.node == NULL) {
				fail(rsp, "");
				break;
			}
			if ((*args)[0] == '+' || (*args)[0] == '-') {
				int pix;
				if (sscanf(*args, "%i", &pix) == 1) {
					int max = (trg.node->split_type == TYPE_HORIZONTAL ? trg.node->rectangle.height : trg.node->rectangle.width);
					double rat = ((max * trg.node->split_ratio) + pix) / max;
					if (rat > 0 && rat < 1) {
						set_ratio(trg.node, rat);
					} else {
						fail(rsp, "");
						break;
					}
				} else {
					fail(rsp, "node %s: Invalid argument: '%s'.\n", *(args - 1), *args);
					break;
				}
			} else {
				double rat;
				if (sscanf(*args, "%lf", &rat) == 1 && rat > 0 && rat < 1) {
					set_ratio(trg.node, rat);
				} else {
					fail(rsp, "node %s: Invalid argument: '%s'.\n", *(args - 1), *args);
					break;
				}
			}
			changed = true;
		} else if (streq("-F", *args) || streq("--flip", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			if (trg.node == NULL) {
				fail(rsp, "");
				break;
			}
			flip_t flp;
			if (parse_flip(*args, &flp)) {
				flip_tree(trg.node, flp);
				changed = true;
			} else {
				fail(rsp, "");
				break;
			}
		} else if (streq("-R", *args) || streq("--rotate", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			if (trg.node == NULL) {
				fail(rsp, "");
				break;
			}
			int deg;
			if (parse_degree(*args, &deg)) {
				rotate_tree(trg.node, deg);
				changed = true;
			} else {
				fail(rsp, "node %s: Invalid argument: '%s'.\n", *(args - 1), *args);
				break;
			}
		} else if (streq("-E", *args) || streq("--equalize", *args)) {
			if (trg.node == NULL) {
				fail(rsp, "");
				break;
			}
			equalize_tree(trg.node);
			changed = true;
		} else if (streq("-B", *args) || streq("--balance", *args)) {
			if (trg.node == NULL) {
				fail(rsp, "");
				break;
			}
			balance_tree(trg.node);
			changed = true;
		} else if (streq("-C", *args) || streq("--circulate", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "node %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			if (trg.node == NULL) {
				fail(rsp, "");
				break;
			}
			circulate_dir_t cir;
			if (parse_circulate_direction(*args, &cir)) {
				circulate_leaves(trg.monitor, trg.desktop, trg.node, cir);
				changed = true;
			} else {
				fail(rsp, "node %s: Invalid argument: '%s'.\n", *(args - 1), *args);
				break;
			}
		} else if (streq("-i", *args) || streq("--insert-receptacle", *args)) {
			insert_receptacle(trg.monitor, trg.desktop, trg.node);
			changed = true;
		} else if (streq("-c", *args) || streq("--close", *args)) {
			if (num > 1) {
				fail(rsp, "node %s: Trailing commands.\n", *args);
				break;
			}
			if (trg.node == NULL || locked_count(trg.node) > 0) {
				fail(rsp, "");
				break;
			}
			close_node(trg.node);
			break;
		} else if (streq("-k", *args) || streq("--kill", *args)) {
			if (num > 1) {
				fail(rsp, "node %s: Trailing commands.\n", *args);
				break;
			}
			if (trg.node == NULL) {
				fail(rsp, "");
				break;
			}
			kill_node(trg.monitor, trg.desktop, trg.node);
			changed = true;
			break;
		} else {
			fail(rsp, "node: Unknown command: '%s'.\n", *args);
			break;
		}

		num--, args++;
	}

	if (changed) {
		arrange(trg.monitor, trg.desktop);
	}
}

void cmd_desktop(char **args, int num, FILE *rsp)
{
	if (num < 1) {
		fail(rsp, "desktop: Missing commands.\n");
		return;
	}

	coordinates_t ref = {mon, mon->desk, NULL};
	coordinates_t trg = ref;

	if ((*args)[0] != OPT_CHR) {
		int ret;
		if ((ret = desktop_from_desc(*args, &ref, &trg)) == SELECTOR_OK) {
			num--, args++;
		} else {
			handle_failure(ret, "desktop", *args, rsp);
			return;
		}
	}

	bool changed = false;

	while (num > 0) {
		if (streq("-f", *args) || streq("--focus", *args)) {
			coordinates_t dst = trg;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				int ret;
				if ((ret = desktop_from_desc(*args, &trg, &dst)) != SELECTOR_OK) {
					handle_failure(ret, "desktop -f", *args, rsp);
					break;
				}
			}
			focus_node(dst.monitor, dst.desktop, dst.desktop->focus);
		} else if (streq("-a", *args) || streq("--activate", *args)) {
			coordinates_t dst = trg;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				int ret;
				if ((ret = desktop_from_desc(*args, &trg, &dst)) != SELECTOR_OK) {
					handle_failure(ret, "desktop -a", *args, rsp);
					break;
				}
			}
			activate_desktop(dst.monitor, dst.desktop);
		} else if (streq("-m", *args) || streq("--to-monitor", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "desktop %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			if (trg.monitor->desk_head == trg.monitor->desk_tail) {
				fail(rsp, "");
				break;
			}
			coordinates_t dst;
			int ret;
			if ((ret = monitor_from_desc(*args, &trg, &dst)) == SELECTOR_OK) {
				if (transfer_desktop(trg.monitor, dst.monitor, trg.desktop)) {
					trg.monitor = dst.monitor;
				} else {
					fail(rsp, "");
					break;
				}
			} else {
				handle_failure(ret, "desktop -m", *args, rsp);
				break;
			}
		} else if (streq("-s", *args) || streq("--swap", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "desktop %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			coordinates_t dst;
			int ret;
			if ((ret = desktop_from_desc(*args, &trg, &dst)) == SELECTOR_OK) {
				if (swap_desktops(trg.monitor, trg.desktop, dst.monitor, dst.desktop)) {
					trg.monitor = dst.monitor;
				} else {
					fail(rsp, "");
					break;
				}
			} else {
				handle_failure(ret, "desktop -s", *args, rsp);
				break;
			}
		} else if (streq("-b", *args) || streq("--bubble", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "desktop %s: Not enough arguments.\n", *(args - 1));
				break;
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
				fail(rsp, "desktop %s: Invalid argument: '%s'.\n", *(args - 1), *args);
				break;
			}
		} else if (streq("-l", *args) || streq("--layout", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "desktop %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			layout_t lyt;
			cycle_dir_t cyc;
			if (parse_cycle_direction(*args, &cyc)) {
				change_layout(trg.monitor, trg.desktop, (trg.desktop->layout + 1) % 2);
			} else if (parse_layout(*args, &lyt)) {
				change_layout(trg.monitor, trg.desktop, lyt);
			} else {
				fail(rsp, "desktop %s: Invalid argument: '%s'.\n", *(args - 1), *args);
				break;
			}
		} else if (streq("-n", *args) || streq("--rename", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "desktop %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			rename_desktop(trg.monitor, trg.desktop, *args);
		} else if (streq("-r", *args) || streq("--remove", *args)) {
			if (num > 1) {
				fail(rsp, "desktop %s: Trailing commands.\n", *args);
				break;
			}
			if (trg.desktop->root == NULL &&
			    trg.monitor->desk_head != trg.monitor->desk_tail) {
				remove_desktop(trg.monitor, trg.desktop);
				return;
			} else {
				fail(rsp, "");
				break;
			}
		} else {
			fail(rsp, "desktop: Unknown command: '%s'.\n", *args);
			break;
		}
		num--, args++;
	}

	if (changed) {
		arrange(trg.monitor, trg.desktop);
	}
}

void cmd_monitor(char **args, int num, FILE *rsp)
{
	if (num < 1) {
		fail(rsp, "monitor: Missing commands.\n");
		return;
	}

	coordinates_t ref = {mon, NULL, NULL};
	coordinates_t trg = ref;

	if ((*args)[0] != OPT_CHR) {
		int ret;
		if ((ret = monitor_from_desc(*args, &ref, &trg)) == SELECTOR_OK) {
			num--, args++;
		} else {
			handle_failure(ret, "monitor", *args, rsp);
			return;
		}
	}

	while (num > 0) {
		if (streq("-f", *args) || streq("--focus", *args)) {
			coordinates_t dst = trg;
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				int ret;
				if ((ret = monitor_from_desc(*args, &trg, &dst)) != SELECTOR_OK) {
					handle_failure(ret, "monitor -f", *args, rsp);
					fail(rsp, "");
					return;
				}
			}
			focus_node(dst.monitor, dst.monitor->desk, dst.monitor->desk->focus);
		} else if (streq("-d", *args) || streq("--reset-desktops", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "monitor %s: Not enough arguments.\n", *(args - 1));
				return;
			}
			desktop_t *d = trg.monitor->desk_head;
			while (num > 0 && d != NULL) {
				rename_desktop(trg.monitor, d, *args);
				d = d->next;
				num--, args++;
			}
			put_status(SBSC_MASK_REPORT);
			while (num > 0) {
				add_desktop(trg.monitor, make_desktop(*args, XCB_NONE));
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
				fail(rsp, "monitor %s: Not enough arguments.\n", *(args - 1));
				return;
			}
			while (num > 0) {
				add_desktop(trg.monitor, make_desktop(*args, XCB_NONE));
				num--, args++;
			}
		} else if (streq("-r", *args) || streq("--remove", *args)) {
			if (num > 1) {
				fail(rsp, "monitor %s: Trailing commands.\n", *args);
				return;
			}
			if (mon_head == mon_tail) {
				fail(rsp, "");
				return;
			}
			remove_monitor(trg.monitor);
			return;
		} else if (streq("-o", *args) || streq("--order-desktops", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "monitor %s: Not enough arguments.\n", *(args - 1));
				return;
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
		} else if (streq("-g", *args) || streq("--rectangle", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "monitor %s: Not enough arguments.\n", *(args - 1));
				return;
			}
			xcb_rectangle_t r;
			if (parse_rectangle(*args, &r)) {
				update_root(trg.monitor, &r);
			} else {
				fail(rsp, "monitor %s: Invalid argument: '%s'.\n", *(args - 1), *args);
				return;
			}
		} else if (streq("-n", *args) || streq("--rename", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "monitor %s: Not enough arguments.\n", *(args - 1));
				return;
			}
			rename_monitor(trg.monitor, *args);
		} else if (streq("-s", *args) || streq("--swap", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "monitor %s: Not enough arguments.\n", *(args - 1));
				return;
			}
			coordinates_t dst;
			int ret;
			if ((ret = monitor_from_desc(*args, &trg, &dst)) == SELECTOR_OK) {
				if (!swap_monitors(trg.monitor, dst.monitor)) {
					fail(rsp, "");
					return;
				}
			} else {
				handle_failure(ret, "monitor -s", *args, rsp);
				return;
			}
		} else {
			fail(rsp, "monitor: Unknown command: '%s'.\n", *args);
			return;
		}
		num--, args++;
	}
}

void cmd_query(char **args, int num, FILE *rsp)
{
	coordinates_t ref = {mon, mon->desk, mon->desk->focus};
	coordinates_t trg = {NULL, NULL, NULL};
	monitor_select_t *monitor_sel = NULL;
	desktop_select_t *desktop_sel = NULL;
	node_select_t *node_sel = NULL;
	domain_t dom = DOMAIN_TREE;
	int d = 0, t = 0;

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
				int ret;
				if ((*args)[0] == '.') {
					free(monitor_sel);
					monitor_sel = malloc(sizeof(monitor_select_t));
					*monitor_sel = make_monitor_select();
					char *desc = copy_string(*args, strlen(*args));
					if (!parse_monitor_modifiers(desc, monitor_sel)) {
						handle_failure(SELECTOR_BAD_MODIFIERS, "query -m", *args, rsp);
						free(desc);
						goto end;
					}
					free(desc);
				} else if ((ret = monitor_from_desc(*args, &ref, &trg)) != SELECTOR_OK) {
					handle_failure(ret, "query -m", *args, rsp);
					goto end;
				}
			} else {
				trg.monitor = ref.monitor;
			}
			t++;
		} else if (streq("-d", *args) || streq("--desktop", *args)) {
			if (num > 1 && *(args + 1)[0] != OPT_CHR) {
				num--, args++;
				int ret;
				if ((*args)[0] == '.') {
					free(desktop_sel);
					desktop_sel = malloc(sizeof(desktop_select_t));
					*desktop_sel = make_desktop_select();
					char *desc = copy_string(*args, strlen(*args));
					if (!parse_desktop_modifiers(desc, desktop_sel)) {
						handle_failure(SELECTOR_BAD_MODIFIERS, "query -d", *args, rsp);
						free(desc);
						goto end;
					}
					free(desc);
				} else if ((ret = desktop_from_desc(*args, &ref, &trg)) != SELECTOR_OK) {
					handle_failure(ret, "query -d", *args, rsp);
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
				int ret;
				if ((*args)[0] == '.') {
					free(node_sel);
					node_sel = malloc(sizeof(node_select_t));
					*node_sel = make_node_select();
					char *desc = copy_string(*args, strlen(*args));
					if (!parse_node_modifiers(desc, node_sel)) {
						handle_failure(SELECTOR_BAD_MODIFIERS, "query -n", *args, rsp);
						free(desc);
						goto end;
					}
					free(desc);
				} else if ((ret = node_from_desc(*args, &ref, &trg)) != SELECTOR_OK) {
					handle_failure(ret, "query -n", *args, rsp);
					goto end;
				}
			} else {
				trg = ref;
				if (ref.node == NULL) {
					fail(rsp, "");
					goto end;
				}
			}
			t++;
		} else {
			fail(rsp, "query: Unknown option: '%s'.\n", *args);
			goto end;
		}
		num--, args++;
	}

	if (d != 1 || t > 1) {
		fail(rsp, "query: Exactly one domain and at most one constraint must be specified.\n");
		goto end;
	}

	if (dom == DOMAIN_NODE) {
		if (query_node_ids(trg, node_sel, rsp) < 1) {
			fail(rsp, "");
		}
	} else if (dom == DOMAIN_DESKTOP) {
		if (query_desktop_ids(trg, desktop_sel, rsp) < 1) {
			fail(rsp, "");
		}
	} else if (dom == DOMAIN_MONITOR) {
		if (query_monitor_ids(trg, monitor_sel, rsp) < 1) {
			fail(rsp, "");
		}
	} else {
		if (trg.node != NULL) {
			query_node(trg.node, rsp);
		} else if (trg.desktop != NULL) {
			query_desktop(trg.desktop, rsp);
		} else  if (trg.monitor != NULL) {
			query_monitor(trg.monitor, rsp);
		} else {
			fail(rsp, "");
			goto end;
		}
		fprintf(rsp, "\n");
	}

end:
	free(monitor_sel);
	free(desktop_sel);
	free(node_sel);
}

void cmd_rule(char **args, int num, FILE *rsp)
{
	if (num < 1) {
		fail(rsp, "rule: Missing commands.\n");
		return;
	}

	while (num > 0) {
		if (streq("-a", *args) || streq("--add", *args)) {
			num--, args++;
			if (num < 2) {
				fail(rsp, "rule %s: Not enough arguments.\n", *(args - 1));
				return;
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
				fail(rsp, "rule %s: Not enough arguments.\n", *(args - 1));
				return;
			}
			uint16_t idx;
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
			list_rules(rsp);
		} else {
			fail(rsp, "rule: Unknown command: '%s'.\n", *args);
			return;
		}
		num--, args++;
	}
}

void cmd_wm(char **args, int num, FILE *rsp)
{
	if (num < 1) {
		fail(rsp, "wm: Missing commands.\n");
		return;
	}

	while (num > 0) {
		if (streq("-d", *args) || streq("--dump-state", *args)) {
			query_tree(rsp);
			fprintf(rsp, "\n");
		} else if (streq("-l", *args) || streq("--load-state", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "wm %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			if (!restore_tree(*args)) {
				fail(rsp, "");
				break;
			}
		} else if (streq("-a", *args) || streq("--add-monitor", *args)) {
			num--, args++;
			if (num < 2) {
				fail(rsp, "wm %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			char *name = *args;
			num--, args++;
			xcb_rectangle_t r;
			if (parse_rectangle(*args, &r)) {
				monitor_t *m = make_monitor(&r, XCB_NONE);
				snprintf(m->name, sizeof(m->name), "%s", name);
				add_monitor(m);
				add_desktop(m, make_desktop(NULL, XCB_NONE));
			} else {
				fail(rsp, "wm %s: Invalid argument: '%s'.\n", *(args - 1), *args);
				break;
			}
		} else if (streq("-o", *args) || streq("--adopt-orphans", *args)) {
			adopt_orphans();
		} else if (streq("-g", *args) || streq("--get-status", *args)) {
			print_report(rsp);
		} else if (streq("-h", *args) || streq("--record-history", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "wm %s: Not enough arguments.\n", *(args - 1));
				break;
			}
			bool b;
			if (parse_bool(*args, &b)) {
				record_history = b;
			} else {
				fail(rsp, "wm %s: Invalid argument: '%s'.\n", *(args - 1), *args);
				break;
			}
		} else {
			fail(rsp, "wm: Unkown command: '%s'.\n", *args);
			break;
		}
		num--, args++;
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
				fail(rsp, "subscribe: Invalid argument: '%s'.\n", *args);
				return SUBSCRIBE_FAILURE;
			}
			num--, args++;
		}
	}

	add_subscriber(rsp, field);
	return SUBSCRIBE_SUCCESS;
}

void cmd_quit(char **args, int num, FILE *rsp)
{
	if (num > 0 && sscanf(*args, "%i", &exit_status) != 1) {
		fail(rsp, "%s: Invalid argument: '%s'.\n", *(args - 1), *args);
		return;
	}
	running = false;
}

void cmd_config(char **args, int num, FILE *rsp)
{
	if (num < 1) {
		fail(rsp, "config: Missing arguments.\n");
		return;
	}

	coordinates_t ref = {mon, mon->desk, mon->desk->focus};
	coordinates_t trg = {NULL, NULL, NULL};

	while (num > 0 && (*args)[0] == OPT_CHR) {
		if (streq("-m", *args) || streq("--monitor", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "config %s: Not enough arguments.\n", *(args - 1));
				return;
			}
			int ret;
			if ((ret = monitor_from_desc(*args, &ref, &trg)) != SELECTOR_OK) {
				handle_failure(ret, "config -m", *args, rsp);
				return;
			}
		} else if (streq("-d", *args) || streq("--desktop", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "config %s: Not enough arguments.\n", *(args - 1));
				return;
			}
			int ret;
			if ((ret = desktop_from_desc(*args, &ref, &trg)) != SELECTOR_OK) {
				handle_failure(ret, "config -d", *args, rsp);
				return;
			}
		} else if (streq("-n", *args) || streq("--node", *args)) {
			num--, args++;
			if (num < 1) {
				fail(rsp, "config %s: Not enough arguments.\n", *(args - 1));
				return;
			}
			int ret;
			if ((ret = node_from_desc(*args, &ref, &trg)) != SELECTOR_OK) {
				handle_failure(ret, "config -n", *args, rsp);
				return;
			}
		} else {
			fail(rsp, "config: Unknown option: '%s'.\n", *args);
			return;
		}
		num--, args++;
	}
	if (num == 2) {
		set_setting(trg, *args, *(args + 1), rsp);
	} else if (num == 1) {
		get_setting(trg, *args, rsp);
	} else {
		fail(rsp, "config: Was expecting 1 or 2 arguments, received %i.\n", num);
	}
}

void set_setting(coordinates_t loc, char *name, char *value, FILE *rsp)
{
	bool colors_changed = false;
#define SET_DEF_DEFMON_DEFDESK_WIN(k, v) \
		if (loc.node != NULL) { \
			loc.node->client->k = v; \
		} else if (loc.desktop != NULL) { \
			loc.desktop->k = v; \
			for (node_t *n = first_extrema(loc.desktop->root); n != NULL; n = next_leaf(n, loc.desktop->root)) { \
				if (n->client != NULL) { \
					n->client->k = v; \
				} \
			} \
		} else if (loc.monitor != NULL) { \
			loc.monitor->k = v; \
			for (desktop_t *d = loc.monitor->desk_head; d != NULL; d = d->next) { \
				d->k = v; \
				for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) { \
					if (n->client != NULL) { \
						n->client->k = v; \
					} \
				} \
			} \
		} else { \
			k = v; \
			for (monitor_t *m = mon_head; m != NULL; m = m->next) { \
				m->k = v; \
				for (desktop_t *d = m->desk_head; d != NULL; d = d->next) { \
					d->k = v; \
					for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) { \
						if (n->client != NULL) { \
							n->client->k = v; \
						} \
					} \
				} \
			} \
		}
	if (streq("border_width", name)) {
		unsigned int bw;
		if (sscanf(value, "%u", &bw) != 1) {
			fail(rsp, "config: %s: Invalid value: '%s'.", name, value);
			return;
		}
		SET_DEF_DEFMON_DEFDESK_WIN(border_width, bw)
#undef SET_DEF_DEFMON_DEFDESK_WIN
#define SET_DEF_DEFMON_DESK(k, v) \
		if (loc.desktop != NULL) { \
			loc.desktop->k = v; \
		} else if (loc.monitor != NULL) { \
			loc.monitor->k = v; \
			for (desktop_t *d = loc.monitor->desk_head; d != NULL; d = d->next) { \
				d->k = v; \
			} \
		} else { \
			k = v; \
			for (monitor_t *m = mon_head; m != NULL; m = m->next) { \
				m->k = v; \
				for (desktop_t *d = m->desk_head; d != NULL; d = d->next) { \
					d->k = v; \
				} \
			} \
		}
	} else if (streq("window_gap", name)) {
		int wg;
		if (sscanf(value, "%i", &wg) != 1) {
			fail(rsp, "");
			return;
		}
		SET_DEF_DEFMON_DESK(window_gap, wg)
#undef SET_DEF_DEFMON_DESK
#define SET_DEF_MON_DESK(k, v) \
		if (loc.desktop != NULL) { \
			loc.desktop->k = v; \
		} else if (loc.monitor != NULL) { \
			loc.monitor->k = v; \
		} else { \
			k = v; \
			for (monitor_t *m = mon_head; m != NULL; m = m->next) { \
				m->k = v; \
			} \
		}
	} else if (streq("top_padding", name)) {
		int tp;
		if (sscanf(value, "%i", &tp) != 1) {
			fail(rsp, "config: %s: Invalid value: '%s'.\n", name, value);
			return;
		}
		SET_DEF_MON_DESK(padding.top, tp)
	} else if (streq("right_padding", name)) {
		int rp;
		if (sscanf(value, "%i", &rp) != 1) {
			fail(rsp, "config: %s: Invalid value: '%s'.\n", name, value);
			return;
		}
		SET_DEF_MON_DESK(padding.right, rp)
	} else if (streq("bottom_padding", name)) {
		int bp;
		if (sscanf(value, "%i", &bp) != 1) {
			fail(rsp, "config: %s: Invalid value: '%s'.\n", name, value);
			return;
		}
		SET_DEF_MON_DESK(padding.bottom, bp)
	} else if (streq("left_padding", name)) {
		int lp;
		if (sscanf(value, "%i", &lp) != 1) {
			fail(rsp, "config: %s: Invalid value: '%s'.\n", name, value);
			return;
		}
		SET_DEF_MON_DESK(padding.left, lp)
#undef SET_DEF_MON_DESK
#define SET_STR(s) \
	} else if (streq(#s, name)) { \
		if (snprintf(s, sizeof(s), "%s", value) < 0) { \
			fail(rsp, ""); \
			return; \
		}
	SET_STR(external_rules_command)
	SET_STR(status_prefix)
#undef SET_STR
	} else if (streq("split_ratio", name)) {
		double r;
		if (sscanf(value, "%lf", &r) == 1 && r > 0 && r < 1) {
			split_ratio = r;
		} else {
			fail(rsp, "config: %s: Invalid value: '%s'.\n", name, value); \
			return;
		}
		return;
#define SET_COLOR(s) \
	} else if (streq(#s, name)) { \
		if (!is_hex_color(value)) { \
			fail(rsp, "config: %s: Invalid value: '%s'.\n", name, value); \
			return; \
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
			fail(rsp, "config: %s: Invalid value: '%s'.\n", name, value);
			return;
		}
	} else if (streq("pointer_modifier", name)) {
		if (parse_modifier_mask(value, &pointer_modifier)) {
			ungrab_buttons();
			grab_buttons();
		} else {
			fail(rsp, "config: %s: Invalid value: '%s'.\n", name, value);
			return;
		}
	} else if (streq("pointer_action1", name) ||
	           streq("pointer_action2", name) ||
	           streq("pointer_action3", name)) {
		int index = name[14] - '1';
		if (!parse_pointer_action(value, &pointer_actions[index])) {
			fail(rsp, "config: %s: Invalid value: '%s'.\n", name, value);
			return;
		}
	} else if (streq("click_to_focus", name)) {
		if (parse_bool(value, &click_to_focus)) {
			ungrab_buttons();
			grab_buttons();
		} else {
			fail(rsp, "config: %s: Invalid value: '%s'.\n", name, value);
			return;
		}
	} else if (streq("focus_follows_pointer", name)) {
		bool b;
		if (parse_bool(value, &b)) {
			if (b == focus_follows_pointer) {
				return;
			}
			focus_follows_pointer = b;
			uint32_t values[] = {CLIENT_EVENT_MASK | (focus_follows_pointer ? XCB_EVENT_MASK_ENTER_WINDOW : 0)};
			for (monitor_t *m = mon_head; m != NULL; m = m->next) {
				for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
					for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
						if (n->client == NULL) {
							continue;
						}
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
			return;
		} else {
			fail(rsp, "config: %s: Invalid value: '%s'.\n", name, value);
			return;
		}
#define SET_BOOL(s) \
	} else if (streq(#s, name)) { \
		if (!parse_bool(value, &s)) { \
			fail(rsp, "config: %s: Invalid value: '%s'.\n", name, value); \
			return; \
		}
		SET_BOOL(borderless_monocle)
		SET_BOOL(gapless_monocle)
		SET_BOOL(paddingless_monocle)
		SET_BOOL(single_monocle)
		SET_BOOL(pointer_follows_focus)
		SET_BOOL(pointer_follows_monitor)
		SET_BOOL(history_aware_focus)
		SET_BOOL(focus_by_distance)
		SET_BOOL(ignore_ewmh_focus)
		SET_BOOL(center_pseudo_tiled)
		SET_BOOL(honor_size_hints)
#undef SET_BOOL
#define SET_MON_BOOL(s) \
	} else if (streq(#s, name)) { \
		if (!parse_bool(value, &s)) { \
			fail(rsp, "config: %s: Invalid value: '%s'.\n", name, value); \
			return; \
		} \
		if (s) { \
			update_monitors(); \
		}
		SET_MON_BOOL(remove_disabled_monitors)
		SET_MON_BOOL(remove_unplugged_monitors)
		SET_MON_BOOL(merge_overlapping_monitors)
#undef SET_MON_BOOL
	} else {
		fail(rsp, "config: Unknown setting: '%s'.\n", name);
		return;
	}

	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			arrange(m, d);
			if (colors_changed) {
				update_colors_in(d->root, d, m);
			}
		}
	}
}

void get_setting(coordinates_t loc, char *name, FILE* rsp)
{
	if (streq("split_ratio", name)) {
		fprintf(rsp, "%lf", split_ratio);
	} else if (streq("border_width", name)) {
		if (loc.node != NULL) {
			fprintf(rsp, "%u", loc.node->client->border_width);
		} else if (loc.desktop != NULL) {
			fprintf(rsp, "%u", loc.desktop->border_width);
		} else if (loc.monitor != NULL) {
			fprintf(rsp, "%u", loc.monitor->border_width);
		} else {
			fprintf(rsp, "%u", border_width);
		}
	} else if (streq("window_gap", name)) {
		if (loc.desktop != NULL) {
			fprintf(rsp, "%i", loc.desktop->window_gap);
		} else if (loc.monitor != NULL) {
			fprintf(rsp, "%i", loc.monitor->window_gap);
		} else {
			fprintf(rsp, "%i", window_gap);
		}
#define GET_DEF_MON_DESK(k) \
		if (loc.desktop != NULL) { \
			fprintf(rsp, "%i", loc.desktop->k); \
		} else if (loc.monitor != NULL) { \
			fprintf(rsp, "%i", loc.monitor->k); \
		} else { \
			fprintf(rsp, "%i", k); \
		}
	} else if (streq("top_padding", name)) {
		GET_DEF_MON_DESK(padding.top)
	} else if (streq("right_padding", name)) {
		GET_DEF_MON_DESK(padding.right)
	} else if (streq("bottom_padding", name)) {
		GET_DEF_MON_DESK(padding.bottom)
	} else if (streq("left_padding", name)) {
		GET_DEF_MON_DESK(padding.left)
#undef GET_DEF_MON_DESK
	} else if (streq("external_rules_command", name)) {
		fprintf(rsp, "%s", external_rules_command);
	} else if (streq("status_prefix", name)) {
		fprintf(rsp, "%s", status_prefix);
	} else if (streq("initial_polarity", name)) {
		fprintf(rsp, "%s", CHILD_POL_STR(initial_polarity));
	} else if (streq("pointer_modifier", name)) {
		print_modifier_mask(pointer_modifier, rsp);
	} else if (streq("pointer_action1", name) ||
	           streq("pointer_action2", name) ||
	           streq("pointer_action3", name)) {
		int index = name[14] - '1';
		print_pointer_action(pointer_actions[index], rsp);
#define GET_COLOR(s) \
	} else if (streq(#s, name)) { \
		fprintf(rsp, "%s", s);
	GET_COLOR(normal_border_color)
	GET_COLOR(active_border_color)
	GET_COLOR(focused_border_color)
	GET_COLOR(presel_feedback_color)
#undef GET_COLOR
#define GET_BOOL(s) \
	} else if (streq(#s, name)) { \
		fprintf(rsp, "%s", BOOL_STR(s));
	GET_BOOL(borderless_monocle)
	GET_BOOL(gapless_monocle)
	GET_BOOL(paddingless_monocle)
	GET_BOOL(single_monocle)
	GET_BOOL(click_to_focus)
	GET_BOOL(focus_follows_pointer)
	GET_BOOL(pointer_follows_focus)
	GET_BOOL(pointer_follows_monitor)
	GET_BOOL(history_aware_focus)
	GET_BOOL(focus_by_distance)
	GET_BOOL(ignore_ewmh_focus)
	GET_BOOL(center_pseudo_tiled)
	GET_BOOL(honor_size_hints)
	GET_BOOL(remove_disabled_monitors)
	GET_BOOL(remove_unplugged_monitors)
	GET_BOOL(merge_overlapping_monitors)
#undef GET_BOOL
	} else {
		fail(rsp, "config: Unknown setting: '%s'.\n", name);
		return;
	}
	fprintf(rsp, "\n");
}

void handle_failure(int code, char *src, char *val, FILE *rsp)
{
	switch (code) {
		case SELECTOR_BAD_DESCRIPTOR:
			fail(rsp, "%s: Invalid descriptor found in '%s'.\n", src, val);
			break;
		case SELECTOR_BAD_MODIFIERS:
			fail(rsp, "%s: Invalid modifier found in '%s'.\n", src, val);
			break;
		case SELECTOR_INVALID:
			fail(rsp, "");
			break;
	}
}

void fail(FILE *rsp, char *fmt, ...)
{
	fprintf(rsp, FAILURE_MESSAGE);
	va_list ap;
	va_start(ap, fmt);
	vfprintf(rsp, fmt, ap);
	va_end(ap);
}

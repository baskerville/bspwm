#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "settings.h"
#include "messages.h"
#include "query.h"
#include "restore.h"
#include "common.h"
#include "types.h"
#include "bspwm.h"
#include "ewmh.h"
#include "helpers.h"
#include "window.h"
#include "events.h"
#include "tree.h"
#include "rules.h"

bool cmd_window(char **args, int num)
{
    if (num < 1)
        return false;

    coordinates_t ref = {mon, mon->desk, mon->desk->focus};
    coordinates_t trg = ref;

    if (*args[0] != OPT_CHR) {
        if (node_from_desc(*args, &ref, &trg))
            num--, args++;
        else
            return false;
    }

    if (trg.node == NULL)
        return false;

    bool dirty = false;

    while (num > 0) {
        if (streq("-f", *args) || streq("--focus", *args)) {
            coordinates_t dst = trg;
            if (num > 1 && *(args + 1)[0] != OPT_CHR) {
                num--, args++;
                if (!node_from_desc(*args, &trg, &dst))
                    return false;
            }
            focus_node(dst.monitor, dst.desktop, dst.node);
        } else if (streq("-d", *args) || streq("--to-desktop", *args)) {
            num--, args++;
            coordinates_t dst;
            if (desktop_from_desc(*args, &trg, &dst)) {
                transfer_node(trg.monitor, trg.desktop, dst.monitor, dst.desktop, trg.node);
                trg.monitor = dst.monitor;
                trg.desktop = dst.desktop;
            } else {
                return false;
            }
        } else if (streq("-m", *args) || streq("--to-monitor", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            coordinates_t dst;
            if (monitor_from_desc(*args, &trg, &dst)) {
                transfer_node(trg.monitor, trg.desktop, dst.monitor, dst.monitor->desk, trg.node);
                trg.monitor = dst.monitor;
                trg.desktop = dst.monitor->desk;
            } else {
                return false;
            }
        } else if (streq("-w", *args) || streq("--to-window", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            coordinates_t dst;
            if (node_from_desc(*args, &trg, &dst))
                transplant_node(trg.monitor, trg.desktop, trg.node, dst.node);
            else
                return false;
            dirty = true;
        } else if (streq("-s", *args) || streq("--swap", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            coordinates_t dst;
            if (node_from_desc(*args, &trg, &dst))
                swap_nodes(trg.node, dst.node);
            else
                return false;
            dirty = true;
        } else if (streq("-t", *args) || streq("--toggle", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            char *key = strtok(*args, EQL_TOK);
            char *val = strtok(NULL, EQL_TOK);
            state_alter_t a = ALTER_NONE;
            bool b;
            if (val == NULL) {
                a = ALTER_TOGGLE;
            } else {
                if (parse_bool(val, &b))
                    a = ALTER_SET;
                else
                    return false;
            }
            if (streq("fullscreen", key)) {
                set_fullscreen(trg.desktop, trg.node, (a == ALTER_SET ? b : !trg.node->client->fullscreen));
                dirty = true;
            } else if (streq("floating", key)) {
                set_floating(trg.desktop, trg.node, (a == ALTER_SET ? b : !trg.node->client->floating));
                dirty = true;
            } else if (streq("locked", key)) {
                set_locked(trg.monitor, trg.desktop, trg.node, (a == ALTER_SET ? b : !trg.node->client->locked));
            }
        } else if (streq("-p", *args) || streq("--presel", *args)) {
            num--, args++;
            if (num < 1 || !is_tiled(trg.node->client)
                    || trg.desktop->layout != LAYOUT_TILED)
                return false;
            if (streq("cancel", *args)) {
                reset_mode(&trg);
            } else {
                direction_t dir;
                if (parse_direction(*args, &dir)) {
                    double rat = trg.node->split_ratio;
                    if (num > 1 && *(args + 1)[0] != OPT_CHR) {
                        num--, args++;
                        if (sscanf(*args, "%lf", &rat) != 1 || rat <= 0 || rat >= 1)
                            return false;
                    }
                    if (auto_cancel && trg.node->split_mode == MODE_MANUAL
                            && dir == trg.node->split_dir
                            && rat == trg.node->split_ratio) {
                        reset_mode(&trg);
                    } else {
                        trg.node->split_mode = MODE_MANUAL;
                        trg.node->split_dir = dir;
                        trg.node->split_ratio = rat;
                    }
                    window_draw_border(trg.node, trg.desktop->focus == trg.node, mon == trg.monitor);
                } else {
                    return false;
                }
            }
        } else if (streq("-e", *args) || streq("--edge", *args)) {
            num--, args++;
            if (num < 2)
                return false;
            direction_t dir;
            if (!parse_direction(*args, &dir))
                return false;
            num--, args++;
            fence_move_t fmo;
            if (parse_fence_move(*args, &fmo)) {
                move_fence(trg.node, dir, fmo);
            } else {
                node_t *n = find_fence(trg.node, dir);
                double rat;
                if (n != NULL && sscanf(*args, "%lf", &rat) == 1 && rat > 0 && rat < 1)
                    n->split_ratio = rat;
                else
                    return false;
            }
            dirty = true;
        } else if (streq("-r", *args) || streq("--ratio", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            double rat;
            if (sscanf(*args, "%lf", &rat) == 1 && rat > 0 && rat < 1) {
                trg.node->split_ratio = rat;
                window_draw_border(trg.node, trg.desktop->focus == trg.node, mon == trg.monitor);
            } else {
                return false;
            }
        } else if (streq("-c", *args) || streq("--close", *args)) {
            if (num > 1)
                return false;
            window_close(trg.node);
        } else if (streq("-k", *args) || streq("--kill", *args)) {
            if (num > 1)
                return false;
            window_kill(trg.desktop, trg.node);
            dirty = true;
        } else {
            return false;
        }
        num--, args++;
    }

    if (dirty)
        arrange(trg.monitor, trg.desktop);

    return true;
}

bool cmd_desktop(char **args, int num)
{
    if (num < 1)
        return false;

    coordinates_t ref = {mon, mon->desk, NULL};
    coordinates_t trg = ref;

    if (*args[0] != OPT_CHR) {
        if (desktop_from_desc(*args, &ref, &trg))
            num--, args++;
        else
            return false;
    }

    bool dirty = false;

    while (num > 0) {
        if (streq("-f", *args) || streq("--focus", *args)) {
            coordinates_t dst = trg;
            if (num > 1 && *(args + 1)[0] != OPT_CHR) {
                num--, args++;
                if (!desktop_from_desc(*args, &trg, &dst))
                    return false;
            }
            if (auto_alternate && dst.desktop == dst.monitor->desk && dst.monitor->last_desk != NULL)
                dst.desktop = dst.monitor->last_desk;
            focus_node(dst.monitor, dst.desktop, dst.desktop->focus);
        } else if (streq("-m", *args) || streq("--to-monitor", *args)) {
            num--, args++;
            if (num < 1 || trg.monitor->desk_head == trg.monitor->desk_tail)
                return false;
            coordinates_t dst;
            if (monitor_from_desc(*args, &trg, &dst)) {
                transfer_desktop(trg.monitor, dst.monitor, dst.desktop);
                trg.monitor = dst.monitor;
                update_current();
            } else {
                return false;
            }
        } else if (streq("-l", *args) || streq("--layout", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            layout_t lyt;
            cycle_dir_t cyc;
            if (parse_cycle_direction(*args, &cyc))
                change_layout(trg.monitor, trg.desktop, (trg.desktop->layout + 1) % 2);
            else if (parse_layout(*args, &lyt))
                change_layout(trg.monitor, trg.desktop, lyt);
            else
                return false;
        } else if (streq("-n", *args) || streq("--rename", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            strncpy(trg.desktop->name, *args, sizeof(trg.desktop->name));
            ewmh_update_desktop_names();
            put_status();
        } else if (streq("-r", *args) || streq("--rm", *args)) {
            if (trg.desktop->root == NULL
                    && trg.monitor->desk_head != trg.monitor->desk_tail) {
                remove_desktop(trg.monitor, trg.desktop);
                desktop_show(trg.monitor->desk);
                update_current();
                return true;
            } else {
                return false;
            }
        } else if (streq("-c", *args) || streq("--cancel-presel", *args)) {
            reset_mode(&trg);
        } else if (streq("-F", *args) || streq("--flip", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            flip_t flp;
            if (parse_flip(*args, &flp)) {
                flip_tree(trg.desktop->root, flp);
                dirty = true;
            }
        } else if (streq("-R", *args) || streq("--rotate", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            int rot = atoi(*args);
            while (rot < 0)
                rot += 360;
            while (rot > 359)
                rot -= 360;
            if ((rot % 90) != 0) {
                return false;
            } else {
                rotate_tree(trg.desktop->root, rot);
                dirty = true;
            }
        } else if (streq("-B", *args) || streq("--balance", *args)) {
            balance_tree(trg.desktop->root);
            dirty = true;
        } else if (streq("-C", *args) || streq("--circulate", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            circulate_dir_t cir;
            if (parse_circulate_direction(*args, &cir)) {
                circulate_leaves(trg.monitor, trg.desktop, cir);
                dirty = true;
            } else {
                return false;
            }
        } else {
            return false;
        }
        num--, args++;
    }

    if (dirty)
        arrange(trg.monitor, trg.desktop);

    return true;
}

bool cmd_monitor(char **args, int num)
{
    if (num < 1)
        return false;

    coordinates_t ref = {mon, NULL, NULL};
    coordinates_t trg = ref;

    if (*args[0] != OPT_CHR) {
        if (monitor_from_desc(*args, &ref, &trg))
            num--, args++;
        else
            return false;
    }

    while (num > 0) {
        if (streq("-f", *args) || streq("--focus", *args)) {
            coordinates_t dst = trg;
            if (num > 1 && *(args + 1)[0] != OPT_CHR) {
                num--, args++;
                if (!monitor_from_desc(*args, &trg, &dst))
                    return false;
            }
            if (auto_alternate && dst.monitor == mon && last_mon != NULL)
                dst.monitor = last_mon;
            focus_node(dst.monitor, dst.monitor->desk, dst.monitor->desk->focus);
        } else if (streq("-a", *args) || streq("--add-desktops", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            while (num > 0) {
                add_desktop(trg.monitor, make_desktop(*args));
                num--, args++;
            }
        } else if (streq("-r", *args) || streq("--remove-desktops", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            while (num > 0) {
                coordinates_t dst;
                if (locate_desktop(*args, &dst) && dst.monitor->desk_head != dst.monitor->desk_tail) {
                    remove_desktop(dst.monitor, dst.desktop);
                    desktop_show(dst.monitor->desk);
                }
                num--, args++;
            }
        } else if (streq("-p", *args) || streq("--pad", *args)) {
            num--, args++;
            if (num < 4)
                return false;
            char values[MAXLEN];
            snprintf(values, sizeof(values), "%s %s %s %s", args[0], args[1], args[2], args[3]);
            num -= 3;
            args += 3;
            if (sscanf(values, "%i %i %i %i", &trg.monitor->top_padding, &trg.monitor->right_padding, &trg.monitor->bottom_padding, &trg.monitor->left_padding) == 4)
                for (desktop_t *d = trg.monitor->desk_head; d != NULL; d = d->next)
                    arrange(trg.monitor, d);
            else
                return false;
        } else if (streq("-n", *args) || streq("--rename", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            strncpy(trg.monitor->name, *args, sizeof(trg.monitor->name));
            put_status();
        } else {
            return false;
        }
        num--, args++;
    }

    return true;
}

bool cmd_query(char **args, int num, char *rsp) {
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
        } else if (streq("-m", *args) || streq("--monitor", *args)) {
            trg.monitor = ref.monitor;
            if (num > 1 && *(args + 1)[0] != OPT_CHR) {
                num--, args++;
                if (!monitor_from_desc(*args, &ref, &trg))
                    return false;
            }
            t++;
        } else if (streq("-d", *args) || streq("--desktop", *args)) {
            trg.monitor = ref.monitor;
            trg.desktop = ref.desktop;
            if (num > 1 && *(args + 1)[0] != OPT_CHR) {
                num--, args++;
                if (!desktop_from_desc(*args, &ref, &trg))
                    return false;
            }
            t++;
        } else if (streq("-w", *args) || streq("--window", *args)) {
            trg = ref;
            if (num > 1 && *(args + 1)[0] != OPT_CHR) {
                num--, args++;
                if (!node_from_desc(*args, &ref, &trg))
                    return false;
            }
            t++;
        } else {
            return false;
        }
        num--, args++;
    }

    if (d != 1 || t > 1)
        return false;

    if (dom == DOMAIN_HISTORY)
        query_history(trg, rsp);
    else if (dom == DOMAIN_WINDOW)
        query_windows(trg, rsp);
    else
        query_monitors(trg, dom, rsp);

    return true;
}

bool cmd_rule(char **args, int num, char *rsp) {
    while (num > 0) {
        if (streq("-a", *args) || streq("--add", *args)) {
            num--, args++;
            if (num < 2)
                return false;
            rule_t *rule = make_rule();
            strncpy(rule->cause.name, *args, sizeof(rule->cause.name));
            num--, args++;
            while (num > 0) {
                if (streq("--floating", *args)) {
                    rule->effect.floating = true;
                } else if (streq("--follow", *args)) {
                    rule->effect.follow = true;
                } else if (streq("-d", *args) || streq("--desktop", *args)) {
                    num--, args++;
                    if (num < 1) {
                        free(rule);
                        return false;
                    }
                    strncpy(rule->effect.desc, *args, sizeof(rule->effect.desc));
                } else {
                    free(rule);
                    return false;
                }
                num--, args++;
            }
            add_rule(rule);
        } else if (streq("-r", *args) || streq("--rm", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            unsigned int uid;
            while (num > 0) {
                if (sscanf(*args, "%X", &uid) == 1)
                    remove_rule_by_uid(uid);
                else
                    return false;
                num--, args++;
            }
        } else if (streq("-l", *args) || streq("--list", *args)) {
            num--, args++;
            list_rules(num > 0 ? *args : NULL, rsp);
        } else {
            return false;
        }
        num--, args++;
    }

    return true;
}

bool cmd_pointer(char **args, int num) {
    while (num > 0) {
        if (streq("-t", *args) || streq("--track", *args)) {
            num--, args++;
            if (num < 2)
                return false;
            int x, y;
            if (sscanf(*args, "%i", &x) == 1 && sscanf(*(args + 1), "%i", &y) == 1)
                track_pointer(x, y);
            else
                return false;
        } else if (streq("-g", *args) || streq("--grab", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            pointer_action_t pac;
            if (parse_pointer_action(*args, &pac))
                grab_pointer(pac);
            else
                return false;
        } else {
            return false;
        }
        num--, args++;
    }

    return true;
}

bool cmd_restore(char **args, int num) {
    while (num > 0) {
        if (streq("-T", *args) || streq("--tree", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            restore_tree(*args);
        } else if (streq("-H", *args) || streq("--history", *args)) {
            num--, args++;
            if (num < 1)
                return false;
            restore_history(*args);
        } else {
            return false;
        }
        num--, args++;
    }

    return true;
}

bool cmd_control(char **args, int num) {
    while (num > 0) {
        if (streq("--adopt-orphans", *args)) {
            adopt_orphans();
        } else if (streq("--put-status", *args)) {
            put_status();
        } else if (streq("--toggle-visibility", *args)) {
            toggle_visibility();
        } else {
            return false;
        }
        num--, args++;
    }

    return true;
}

bool cmd_config(char **args, int num, char *rsp) {
    if (num == 2)
        return set_setting(*args, *(args + 1));
    else if (num == 1)
        return get_setting(*args, rsp);
    else
        return false;
}

bool cmd_quit(char **args, int num) {
    if (num > 0 && sscanf(*args, "%i", &exit_status) != 1)
        return false;
    quit();
    return true;
}

bool handle_message(char *msg, int msg_len, char *rsp)
{
    int cap = INIT_CAP;
    int num = 0;
    char **args = malloc(cap * sizeof(char *));
    if (args == NULL)
        return false;

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
                return false;
            } else {
                args = new;
            }
        }
    }

    if (num < 1) {
        free(args);
        return false;
    }

    char **args_orig = args;
    bool ret = process_message(args, num, rsp);
    free(args_orig);
    return ret;
}

bool process_message(char **args, int num, char *rsp)
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
        return cmd_control(++args, --num);
    } else if (streq("rule", *args)) {
        return cmd_rule(++args, --num, rsp);
    } else if (streq("pointer", *args)) {
        return cmd_pointer(++args, --num);
    } else if (streq("config", *args)) {
        return cmd_config(++args, --num, rsp);
    } else if (streq("quit", *args)) {
        return cmd_quit(++args, --num);
    }

    return false;
}

bool set_setting(char *name, char *value)
{
    if (streq("border_width", name)) {
        if (sscanf(value, "%u", &border_width) != 1)
            return false;
    } else if (streq("window_gap", name)) {
        if (sscanf(value, "%i", &window_gap) != 1)
            return false;
    } else if (streq("split_ratio", name)) {
        double rat;
        if (sscanf(value, "%lf", &rat) == 1 && rat > 0 && rat < 1)
            split_ratio = rat;
        else
            return false;
#define SETCOLOR(s) \
    } else if (streq(#s, name)) { \
        if (get_color(value, &s ## _pxl)) \
            strncpy(s, value, sizeof(s)); \
        else \
            return false;
    SETCOLOR(focused_border_color)
    SETCOLOR(active_border_color)
    SETCOLOR(normal_border_color)
    SETCOLOR(presel_border_color)
    SETCOLOR(focused_locked_border_color)
    SETCOLOR(active_locked_border_color)
    SETCOLOR(normal_locked_border_color)
    SETCOLOR(urgent_border_color)
#undef SETCOLOR
    } else if (streq("focus_follows_pointer", name)) {
        bool b;
        if (parse_bool(value, &b) && b != focus_follows_pointer) {
            uint32_t values[] = {(focus_follows_pointer ? CLIENT_EVENT_MASK : CLIENT_EVENT_MASK_FFP)};
            for (monitor_t *m = mon_head; m != NULL; m = m->next)
                for (desktop_t *d = m->desk_head; d != NULL; d = d->next)
                    for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
                        xcb_change_window_attributes(dpy, n->client->window, XCB_CW_EVENT_MASK, values);
            if (focus_follows_pointer)
                disable_motion_recorder();
            else
                enable_motion_recorder();
            focus_follows_pointer = b;
            return true;
        } else {
            return false;
        }
#define SETBOOL(s) \
    } else if (streq(#s, name)) { \
        if (!parse_bool(value, &s)) \
            return false;
        SETBOOL(borderless_monocle)
        SETBOOL(gapless_monocle)
        SETBOOL(pointer_follows_monitor)
        SETBOOL(adaptative_raise)
        SETBOOL(apply_shadow_property)
        SETBOOL(auto_alternate)
        SETBOOL(auto_cancel)
        SETBOOL(history_aware_focus)
#undef SETBOOL
    } else if (streq("wm_name", name)) {
        strncpy(wm_name, value, sizeof(wm_name));
        ewmh_update_wm_name();
        return true;
    } else {
        return false;
    }

    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        for (desktop_t *d = m->desk_head; d != NULL; d = d->next)
            arrange(m, d);

    return true;
}

bool get_setting(char *name, char* rsp)
{
    if (streq("border_width", name))
        snprintf(rsp, BUFSIZ, "%u", border_width);
    else if (streq("split_ratio", name))
        snprintf(rsp, BUFSIZ, "%lf", split_ratio);
    else if (streq("window_gap", name))
        snprintf(rsp, BUFSIZ, "%i", window_gap);
#define GETCOLOR(s) \
    else if (streq(#s, name)) \
        snprintf(rsp, BUFSIZ, "%s (%06X)", s, s##_pxl);
    GETCOLOR(focused_border_color)
    GETCOLOR(active_border_color)
    GETCOLOR(normal_border_color)
    GETCOLOR(presel_border_color)
    GETCOLOR(focused_locked_border_color)
    GETCOLOR(active_locked_border_color)
    GETCOLOR(normal_locked_border_color)
    GETCOLOR(urgent_border_color)
#undef GETCOLOR
#define GETBOOL(s) \
    else if (streq(#s, name)) \
        snprintf(rsp, BUFSIZ, "%s", BOOLSTR(s));
    GETBOOL(borderless_monocle)
    GETBOOL(gapless_monocle)
    GETBOOL(focus_follows_pointer)
    GETBOOL(pointer_follows_monitor)
    GETBOOL(adaptative_raise)
    GETBOOL(apply_shadow_property)
    GETBOOL(auto_alternate)
    GETBOOL(auto_cancel)
    GETBOOL(history_aware_focus)
#undef GETBOOL
    else if (streq("wm_name", name))
        snprintf(rsp, BUFSIZ, "%s", wm_name);
    else
        return false;
    return true;
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

bool parse_fence_move(char *s, fence_move_t *m)
{
    if (streq("push", s)) {
        *m = MOVE_PUSH;
        return true;
    } else if (streq("pull", s)) {
        *m = MOVE_PULL;
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

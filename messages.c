#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "settings.h"
#include "messages.h"
#include "common.h"
#include "types.h"
#include "bspwm.h"
#include "ewmh.h"
#include "helpers.h"
#include "window.h"
#include "events.h"
#include "tree.h"
#include "rules.h"

void process_message(char *msg, char *rsp)
{
    char *cmd = strtok(msg, TOK_SEP);

    if (cmd == NULL)
        return;

    if (strcmp(cmd, "get") == 0) {
        char *name = strtok(NULL, TOK_SEP);
        get_setting(name, rsp);
    } else if (strcmp(cmd, "set") == 0) {
        char *name = strtok(NULL, TOK_SEP);
        char *value = strtok(NULL, TOK_SEP);
        set_setting(name, value, rsp);
    } else if (strcmp(cmd, "list") == 0) {
        char *name = strtok(NULL, TOK_SEP);
        if (name != NULL) {
            desktop_location_t loc;
            if (locate_desktop(name, &loc))
                list(loc.desktop, loc.desktop->root, rsp, 0);
        } else {
            list(mon->desk, mon->desk->root, rsp, 0);
        }
    } else if (strcmp(cmd, "list_monitors") == 0) {
        char *arg = strtok(NULL, TOK_SEP);
        list_option_t opt;
        if (parse_list_option(arg, &opt))
            list_monitors(opt, rsp);
    } else if (strcmp(cmd, "list_desktops") == 0) {
        char *arg = strtok(NULL, TOK_SEP);
        list_option_t opt;
        if (parse_list_option(arg, &opt))
            list_desktops(mon, opt, 0, rsp);
    } else if (strcmp(cmd, "list_windows") == 0) {
        list_windows(rsp);
    } else if (strcmp(cmd, "list_history") == 0) {
        list_history(rsp);
    } else if (strcmp(cmd, "list_rules") == 0) {
        list_rules(rsp);
    } else if (strcmp(cmd, "close") == 0) {
        window_close(mon->desk->focus);
    } else if (strcmp(cmd, "kill") == 0) {
        window_kill(mon, mon->desk, mon->desk->focus);
    } else if (strcmp(cmd, "rotate") == 0) {
        char *deg = strtok(NULL, TOK_SEP);
        if (deg != NULL) {
            rotate_t r;
            if (parse_rotate(deg, &r))
                rotate_tree(mon->desk->root, r);
        }
        arrange(mon, mon->desk);
    } else if (strcmp(cmd, "flip") == 0) {
        char *flp = strtok(NULL, TOK_SEP);
        if (flp != NULL) {
            flip_t f;
            if (parse_flip(flp, &f))
                flip_tree(mon->desk->root, f);
        }
        arrange(mon, mon->desk);
    } else if (strcmp(cmd, "balance") == 0) {
        balance_tree(mon->desk->root);
        arrange(mon, mon->desk);
    } else if (strcmp(cmd, "grab_pointer") == 0) {
        char *pac = strtok(NULL, TOK_SEP);
        if (pac != NULL) {
            pointer_action_t a;
            if (parse_pointer_action(pac, &a))
                grab_pointer(a);
        }
    } else if (strcmp(cmd, "track_pointer") == 0) {
        char *arg1 = strtok(NULL, TOK_SEP);
        char *arg2 = strtok(NULL, TOK_SEP);
        if (arg1 == NULL || arg2 == NULL)
            return;
        int root_x, root_y;
        if (sscanf(arg1, "%i", &root_x) == 1 && sscanf(arg2, "%i", &root_y) == 1)
            track_pointer(root_x, root_y);
    } else if (strcmp(cmd, "ungrab_pointer") == 0) {
        ungrab_pointer();
    } else if (strcmp(cmd, "layout") == 0) {
        char *lyt = strtok(NULL, TOK_SEP);
        if (lyt != NULL) {
            layout_t l;
            if (parse_layout(lyt, &l)) {
                char *name = strtok(NULL, TOK_SEP);
                if (name == NULL) {
                    change_layout(mon, mon->desk, l);
                } else {
                    desktop_location_t loc;
                    do {
                        if (locate_desktop(name, &loc))
                            change_layout(loc.monitor, loc.desktop, l);
                    } while ((name = strtok(NULL, TOK_SEP)) != NULL);
                }
            }
        }
    } else if (strcmp(cmd, "cycle_layout") == 0) {
        if (mon->desk->layout == LAYOUT_MONOCLE)
            change_layout(mon, mon->desk, LAYOUT_TILED);
        else
            change_layout(mon, mon->desk, LAYOUT_MONOCLE);
    } else if (strcmp(cmd, "shift") == 0) {
        char *dir = strtok(NULL, TOK_SEP);
        if (dir != NULL) {
            direction_t d;
            if (parse_direction(dir, &d)) {
                node_t *n = nearest_neighbor(mon->desk, mon->desk->focus, d);
                if (n != NULL) {
                    swap_nodes(mon->desk->focus, n);
                    arrange(mon, mon->desk);
                } else if (monitor_focus_fallback) {
                    monitor_t *m = nearest_monitor(d);
                    if (m != NULL)
                        transfer_node(mon, mon->desk, m, m->desk, mon->desk->focus);
                }
            }
        }
    } else if (strcmp(cmd, "toggle_fullscreen") == 0) {
        toggle_fullscreen(mon->desk, mon->desk->focus);
        arrange(mon, mon->desk);
    } else if (strcmp(cmd, "toggle_floating") == 0) {
        toggle_floating(mon->desk, mon->desk->focus);
        arrange(mon, mon->desk);
    } else if (strcmp(cmd, "toggle_locked") == 0) {
        toggle_locked(mon, mon->desk, mon->desk->focus);
    } else if (strcmp(cmd, "toggle_visibility") == 0) {
        toggle_visibility();
    } else if (strcmp(cmd, "pad") == 0) {
        char *name = strtok(NULL, TOK_SEP);
        if (name != NULL) {
            monitor_t *m = find_monitor(name);
            if (m != NULL) {
                char args[BUFSIZ] = {0}, *s;
                while ((s = strtok(NULL, TOK_SEP)) != NULL) {
                    strncat(args, s, REMLEN(args));
                    strncat(args, TOK_SEP, REMLEN(args));
                }
                if (strlen(args) > 0) {
                    sscanf(args, "%i %i %i %i", &m->top_padding, &m->right_padding, &m->bottom_padding, &m->left_padding);
                    arrange(m, m->desk);
                } else {
                    snprintf(rsp, BUFSIZ, "%i %i %i %i\n", m->top_padding, m->right_padding, m->bottom_padding, m->left_padding);
                }
            }
        }
    } else if (strcmp(cmd, "ratio") == 0) {
        char *value;
        if (mon->desk->focus != NULL && (value = strtok(NULL, TOK_SEP)) != NULL &&
                sscanf(value, "%lf", &mon->desk->focus->split_ratio) == 1)
            window_draw_border(mon->desk->focus, true, true);
    } else if (strcmp(cmd, "cancel") == 0) {
        if (mon->desk->focus == NULL)
            return;
        char *opt = strtok(NULL, TOK_SEP);
        cancel_option_t o;
        if (parse_cancel_option(opt, &o))
            reset_mode(mon->desk, mon->desk->focus, o);
    } else if (strcmp(cmd, "presel") == 0) {
        if (mon->desk->focus == NULL || !is_tiled(mon->desk->focus->client) || mon->desk->layout != LAYOUT_TILED)
            return;
        char *dir = strtok(NULL, TOK_SEP);
        if (dir != NULL) {
            direction_t d;
            if (parse_direction(dir, &d)) {
                mon->desk->focus->split_mode = MODE_MANUAL;
                mon->desk->focus->split_dir = d;
                char *rat = strtok(NULL, TOK_SEP);
                if (rat != NULL)
                    sscanf(rat, "%lf", &mon->desk->focus->split_ratio);
                window_draw_border(mon->desk->focus, true, true);
            }
        }
    } else if (strcmp(cmd, "push") == 0 || strcmp(cmd, "pull") == 0) {
        char *dir = strtok(NULL, TOK_SEP);
        if (dir != NULL) {
            fence_move_t m;
            direction_t d;
            if (parse_fence_move(cmd, &m) && parse_direction(dir, &d)) {
                move_fence(mon->desk->focus, d, m);
                arrange(mon, mon->desk);
            }
        }
    } else if (strcmp(cmd, "fence_ratio") == 0) {
        char *dir = strtok(NULL, TOK_SEP);
        if (dir != NULL) {
            direction_t d;
            node_t *n;
            if (parse_direction(dir, &d) && (n = find_fence(mon->desk->focus, d)) != NULL) {
                char *value = strtok(NULL, TOK_SEP);
                if (value != NULL && sscanf(value, "%lf", &n->split_ratio) == 1)
                    arrange(mon, mon->desk);
            }
        }
    } else if (strcmp(cmd, "drop_to_monitor") == 0) {
        char *dir = strtok(NULL, TOK_SEP);
        if (dir != NULL) {
            cycle_dir_t d;
            if (parse_cycle_direction(dir, &d)) {
                monitor_t *m;
                if (d == CYCLE_NEXT)
                    m = ((mon->next == NULL ? mon_head : mon->next));
                else
                    m = ((mon->prev == NULL ? mon_tail : mon->prev));
                transfer_node(mon, mon->desk, m, m->desk, mon->desk->focus);
                char *opt = strtok(NULL, TOK_SEP);
                send_option_t o;
                if (parse_send_option(opt, &o) && o == SEND_OPTION_FOLLOW)
                    focus_node(m, m->desk, m->desk->focus);
            }
        }
    } else if (strcmp(cmd, "send_to_monitor") == 0) {
        char *name = strtok(NULL, TOK_SEP);
        if (name != NULL) {
            monitor_t *m = find_monitor(name);
            if (m != NULL && m != mon) {
                transfer_node(mon, mon->desk, m, m->desk, mon->desk->focus);
                char *opt = strtok(NULL, TOK_SEP);
                send_option_t o;
                if (parse_send_option(opt, &o) && o == SEND_OPTION_FOLLOW)
                    focus_node(m, m->desk, m->desk->focus);
            }
        }
    } else if (strcmp(cmd, "drop_to") == 0) {
        char *dir = strtok(NULL, TOK_SEP);
        if (dir != NULL) {
            cycle_dir_t c;
            if (parse_cycle_direction(dir, &c)) {
                desktop_t *d;
                if (c == CYCLE_NEXT)
                    d = ((mon->desk->next == NULL ? mon->desk_head : mon->desk->next));
                else
                    d = ((mon->desk->prev == NULL ? mon->desk_tail : mon->desk->prev));
                transfer_node(mon, mon->desk, mon, d, mon->desk->focus);
                char *opt = strtok(NULL, TOK_SEP);
                send_option_t o;
                if (parse_send_option(opt, &o) && o == SEND_OPTION_FOLLOW)
                    focus_node(mon, d, d->focus);
            }
        }
    } else if (strcmp(cmd, "send_to") == 0) {
        char *name = strtok(NULL, TOK_SEP);
        if (name != NULL) {
            desktop_location_t loc;
            if (locate_desktop(name, &loc)) {
                transfer_node(mon, mon->desk, loc.monitor, loc.desktop, mon->desk->focus);
                char *opt = strtok(NULL, TOK_SEP);
                send_option_t o;
                if (parse_send_option(opt, &o) && o == SEND_OPTION_FOLLOW)
                    focus_node(loc.monitor, loc.desktop, loc.desktop->focus);
            }
        }
    } else if (strcmp(cmd, "rename_monitor") == 0) {
        char *cur_name = strtok(NULL, TOK_SEP);
        if (cur_name != NULL) {
            monitor_t *m = find_monitor(cur_name);
            if (m != NULL) {
                char *new_name = strtok(NULL, TOK_SEP);
                if (new_name != NULL) {
                    strncpy(m->name, new_name, sizeof(m->name));
                    put_status();
                }
            }
        }
    } else if (strcmp(cmd, "rename") == 0) {
        char *cur_name = strtok(NULL, TOK_SEP);
        if (cur_name != NULL) {
            desktop_location_t loc;
            if (locate_desktop(cur_name, &loc)) {
                char *new_name = strtok(NULL, TOK_SEP);
                if (new_name != NULL) {
                    strncpy(loc.desktop->name, new_name, sizeof(loc.desktop->name));
                    ewmh_update_desktop_names();
                    put_status();
                }
            }
        }
    } else if (strcmp(cmd, "use_monitor") == 0) {
        char *name = strtok(NULL, TOK_SEP);
        if (name != NULL) {
            monitor_t *m = find_monitor(name);
            if (m != NULL) {
                if (auto_alternate && m == mon && last_mon != NULL)
                    m = last_mon;
                focus_node(m, m->desk, m->desk->focus);
            }
        }
    } else if (strcmp(cmd, "focus_monitor") == 0) {
        char *dir = strtok(NULL, TOK_SEP);
        if (dir != NULL) {
            direction_t d;
            if (parse_direction(dir, &d)) {
                monitor_t *m = nearest_monitor(d);
                if (m != NULL)
                    focus_node(m, m->desk, m->desk->focus);
            }
        }
    } else if (strcmp(cmd, "use") == 0) {
        char *name = strtok(NULL, TOK_SEP);
        if (name != NULL) {
            desktop_location_t loc;
            if (locate_desktop(name, &loc)) {
                if (auto_alternate && loc.desktop == mon->desk && mon->last_desk != NULL)
                    focus_node(mon, mon->last_desk, mon->last_desk->focus);
                else
                    focus_node(loc.monitor, loc.desktop, loc.desktop->focus);
            }
        }
    } else if (strcmp(cmd, "cycle_monitor") == 0) {
        char *dir = strtok(NULL, TOK_SEP);
        if (dir != NULL) {
            cycle_dir_t d;
            if (parse_cycle_direction(dir, &d))
                cycle_monitor(d);
        }
    } else if (strcmp(cmd, "cycle_desktop") == 0) {
        char *dir = strtok(NULL, TOK_SEP);
        if (dir != NULL) {
            cycle_dir_t d;
            if (parse_cycle_direction(dir, &d)) {
                skip_desktop_t k;
                char *skip = strtok(NULL, TOK_SEP);
                if (parse_skip_desktop(skip, &k))
                    cycle_desktop(mon, mon->desk, d, k);
            }
        }
    } else if (strcmp(cmd, "cycle") == 0) {
        if (mon->desk->focus != NULL && mon->desk->focus->client->fullscreen)
            return;
        char *dir = strtok(NULL, TOK_SEP);
        if (dir != NULL) {
            cycle_dir_t d;
            if (parse_cycle_direction(dir, &d)) {
                skip_client_t k;
                char *skip = strtok(NULL, TOK_SEP);
                if (parse_skip_client(skip, &k))
                    cycle_leaf(mon, mon->desk, mon->desk->focus, d, k);
            }
        }
    } else if (strcmp(cmd, "nearest") == 0) {
        if (mon->desk->focus != NULL && mon->desk->focus->client->fullscreen)
            return;
        char *arg = strtok(NULL, TOK_SEP);
        if (arg != NULL) {
            nearest_arg_t a;
            if (parse_nearest_argument(arg, &a)) {
                skip_client_t k;
                char *skip = strtok(NULL, TOK_SEP);
                if (parse_skip_client(skip, &k))
                    nearest_leaf(mon, mon->desk, mon->desk->focus, a, k);
            }
        }
    } else if (strcmp(cmd, "biggest") == 0) {
        node_t *n = find_biggest(mon->desk);
        if (n != NULL)
            snprintf(rsp, BUFSIZ, "0x%X", n->client->window);
    } else if (strcmp(cmd, "circulate") == 0) {
        if (mon->desk->layout == LAYOUT_MONOCLE
                || (mon->desk->focus != NULL && !is_tiled(mon->desk->focus->client)))
            return;
        char *dir = strtok(NULL, TOK_SEP);
        if (dir != NULL) {
            circulate_dir_t d;
            if (parse_circulate_direction(dir, &d))
                circulate_leaves(mon, mon->desk, d);
        }
        arrange(mon, mon->desk);
    } else if (strcmp(cmd, "rule") == 0) {
        char *name = strtok(NULL, TOK_SEP);
        if (name != NULL) {
            rule_t *rule = make_rule();
            strncpy(rule->cause.name, name, sizeof(rule->cause.name));
            char *arg = strtok(NULL, TOK_SEP);
            while (arg != NULL) {
                if (strcmp(arg, "floating") == 0) {
                    rule->effect.floating = true;
                } else if (strcmp(arg, "follow") == 0) {
                    rule->effect.follow = true;
                } else {
                    desktop_location_t loc;
                    if (locate_desktop(arg, &loc)) {
                        rule->effect.monitor = loc.monitor;
                        rule->effect.desktop = loc.desktop;
                    }
                }
                arg = strtok(NULL, TOK_SEP);
            }
            add_rule(rule);
        }
    } else if (strcmp(cmd, "remove_rule") == 0) {
        char *arg;
        unsigned int uid;
        while ((arg = strtok(NULL, TOK_SEP)) != NULL)
            if (sscanf(arg, "%X", &uid) > 0)
                remove_rule_by_uid(uid);
    } else if (strcmp(cmd, "swap") == 0) {
        char *opt = strtok(NULL, TOK_SEP);
        swap_option_t o;
        if (!parse_swap_option(opt, &o))
            return;
        node_t *last_focus = history_get(mon->desk->history, 1);
        swap_nodes(mon->desk->focus, last_focus);
        arrange(mon, mon->desk);
        if (o == SWAP_OPTION_SWAP_FOCUS)
            focus_node(mon, mon->desk, last_focus);
    } else if (strcmp(cmd, "alternate") == 0) {
        focus_node(mon, mon->desk, history_get(mon->desk->history, 1));
    } else if (strcmp(cmd, "alternate_desktop") == 0) {
        if (mon->last_desk != NULL)
            focus_node(mon, mon->last_desk, mon->last_desk->focus);
    } else if (strcmp(cmd, "alternate_monitor") == 0) {
        if (last_mon != NULL)
            focus_node(last_mon, last_mon->desk, last_mon->desk->focus);
    } else if (strcmp(cmd, "add_in") == 0) {
        char *name = strtok(NULL, TOK_SEP);
        if (name != NULL) {
            monitor_t *m = find_monitor(name);
            if (m != NULL)
                for (name = strtok(NULL, TOK_SEP); name != NULL; name = strtok(NULL, TOK_SEP))
                    add_desktop(m, make_desktop(name));
        }
    } else if (strcmp(cmd, "add") == 0) {
        for (char *name = strtok(NULL, TOK_SEP); name != NULL; name = strtok(NULL, TOK_SEP))
            add_desktop(mon, make_desktop(name));
    } else if (strcmp(cmd, "remove_desktop") == 0) {
        for (char *name = strtok(NULL, TOK_SEP); name != NULL; name = strtok(NULL, TOK_SEP)) {
            desktop_location_t loc;
            if (locate_desktop(name, &loc)) {
                if (loc.desktop->root == NULL && loc.monitor->desk_head != loc.monitor->desk_tail) {
                    remove_desktop(loc.monitor, loc.desktop);
                    desktop_show(loc.monitor->desk);
                }
            }
        }
        update_current();
    } else if (strcmp(cmd, "send_desktop_to") == 0) {
        if (mon->desk_head == mon->desk_tail)
            return;
        char *name = strtok(NULL, TOK_SEP);
        if (name != NULL) {
            monitor_t *m = find_monitor(name);
            if (m != NULL && m != mon) {
                char *opt = strtok(NULL, TOK_SEP);
                send_option_t o;
                if (!parse_send_option(opt, &o))
                    return;
                desktop_t *d = mon->desk;
                transfer_desktop(mon, m, d);
                if (o == SEND_OPTION_FOLLOW)
                    focus_node(m, d, d->focus);
                else if (o == SEND_OPTION_DONT_FOLLOW)
                    update_current();
            }
        }
    } else if (strcmp(cmd, "focus") == 0) {
        char *dir = strtok(NULL, TOK_SEP);
        if (dir != NULL) {
            direction_t d;
            if (parse_direction(dir, &d)) {
                node_t *n = nearest_neighbor(mon->desk, mon->desk->focus, d);
                if (n != NULL) {
                    focus_node(mon, mon->desk, n);
                } else if (monitor_focus_fallback) {
                    monitor_t *m = nearest_monitor(d);
                    if (m != NULL)
                        focus_node(m, m->desk, m->desk->focus);
                }
            }
        }
    } else if (strcmp(cmd, "put_status") == 0) {
        put_status();
    } else if (strcmp(cmd, "adopt_orphans") == 0) {
        adopt_orphans();
    } else if (strcmp(cmd, "restore_layout") == 0) {
        char *arg = strtok(NULL, TOK_SEP);
        restore_layout(arg);
    } else if (strcmp(cmd, "restore_history") == 0) {
        char *arg = strtok(NULL, TOK_SEP);
        restore_history(arg);
    } else if (strcmp(cmd, "quit") == 0) {
        char *arg = strtok(NULL, TOK_SEP);
        if (arg != NULL)
            sscanf(arg, "%i", &exit_status);
        quit();
    } else {
        snprintf(rsp, BUFSIZ, "unknown command: %s", cmd);
    }
}

void set_setting(char *name, char *value, char *rsp)
{
    if (name == NULL || value == NULL)
        return;

    if (strcmp(name, "border_width") == 0) {
        sscanf(value, "%u", &border_width);
    } else if (strcmp(name, "window_gap") == 0) {
        sscanf(value, "%i", &window_gap);
    } else if (strcmp(name, "split_ratio") == 0) {
        sscanf(value, "%lf", &split_ratio);
    } else if (strcmp(name, "left_padding") == 0) {
        sscanf(value, "%i", &mon->left_padding);
    } else if (strcmp(name, "right_padding") == 0) {
        sscanf(value, "%i", &mon->right_padding);
    } else if (strcmp(name, "top_padding") == 0) {
        sscanf(value, "%i", &mon->top_padding);
    } else if (strcmp(name, "bottom_padding") == 0) {
        sscanf(value, "%i", &mon->bottom_padding);
    } else if (strcmp(name, "focused_border_color") == 0) {
        strncpy(focused_border_color, value, sizeof(focused_border_color));
        focused_border_color_pxl = get_color(focused_border_color);
    } else if (strcmp(name, "active_border_color") == 0) {
        strncpy(active_border_color, value, sizeof(active_border_color));
        active_border_color_pxl = get_color(active_border_color);
    } else if (strcmp(name, "normal_border_color") == 0) {
        strncpy(normal_border_color, value, sizeof(normal_border_color));
        normal_border_color_pxl = get_color(normal_border_color);
    } else if (strcmp(name, "presel_border_color") == 0) {
        strncpy(presel_border_color, value, sizeof(presel_border_color));
        presel_border_color_pxl = get_color(presel_border_color);
    } else if (strcmp(name, "focused_locked_border_color") == 0) {
        strncpy(focused_locked_border_color, value, sizeof(focused_locked_border_color));
        focused_locked_border_color_pxl = get_color(focused_locked_border_color);
    } else if (strcmp(name, "active_locked_border_color") == 0) {
        strncpy(active_locked_border_color, value, sizeof(active_locked_border_color));
        active_locked_border_color_pxl = get_color(active_locked_border_color);
    } else if (strcmp(name, "normal_locked_border_color") == 0) {
        strncpy(normal_locked_border_color, value, sizeof(normal_locked_border_color));
        normal_locked_border_color_pxl = get_color(normal_locked_border_color);
    } else if (strcmp(name, "urgent_border_color") == 0) {
        strncpy(urgent_border_color, value, sizeof(urgent_border_color));
        urgent_border_color_pxl = get_color(urgent_border_color);
    } else if (strcmp(name, "borderless_monocle") == 0) {
        bool b;
        if (parse_bool(value, &b))
            borderless_monocle = b;
    } else if (strcmp(name, "gapless_monocle") == 0) {
        bool b;
        if (parse_bool(value, &b))
            gapless_monocle = b;
    } else if (strcmp(name, "focus_follows_pointer") == 0) {
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
        }
        return;
    } else if (strcmp(name, "pointer_follows_monitor") == 0) {
        bool b;
        if (parse_bool(value, &b))
            pointer_follows_monitor = b;
        return;
    } else if (strcmp(name, "monitor_focus_fallback") == 0) {
        bool b;
        if (parse_bool(value, &b))
            monitor_focus_fallback = b;
        return;
    } else if (strcmp(name, "adaptative_raise") == 0) {
        bool b;
        if (parse_bool(value, &b))
            adaptative_raise = b;
        return;
    } else if (strcmp(name, "apply_shadow_property") == 0) {
        bool b;
        if (parse_bool(value, &b))
            apply_shadow_property = b;
        return;
    } else if (strcmp(name, "auto_alternate") == 0) {
        bool b;
        if (parse_bool(value, &b))
            auto_alternate = b;
        return;
    } else if (strcmp(name, "focus_by_distance") == 0) {
        bool b;
        if (parse_bool(value, &b))
            focus_by_distance = b;
        return;
    } else if (strcmp(name, "history_aware_focus") == 0) {
        bool b;
        if (parse_bool(value, &b))
            history_aware_focus = b;
        return;
    } else if (strcmp(name, "wm_name") == 0) {
        strncpy(wm_name, value, sizeof(wm_name));
        ewmh_update_wm_name();
        return;
    } else {
        snprintf(rsp, BUFSIZ, "unknown setting: %s", name);
        return;
    }

    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        for (desktop_t *d = m->desk_head; d != NULL; d = d->next)
            arrange(m, d);
}

void get_setting(char *name, char* rsp)
{
    if (name == NULL)
        return;

    if (strcmp(name, "border_width") == 0)
        snprintf(rsp, BUFSIZ, "%u", border_width);
    else if (strcmp(name, "window_gap") == 0)
        snprintf(rsp, BUFSIZ, "%i", window_gap);
    else if (strcmp(name, "split_ratio") == 0)
        snprintf(rsp, BUFSIZ, "%lf", split_ratio);
    else if (strcmp(name, "left_padding") == 0)
        snprintf(rsp, BUFSIZ, "%i", mon->left_padding);
    else if (strcmp(name, "right_padding") == 0)
        snprintf(rsp, BUFSIZ, "%i", mon->right_padding);
    else if (strcmp(name, "top_padding") == 0)
        snprintf(rsp, BUFSIZ, "%i", mon->top_padding);
    else if (strcmp(name, "bottom_padding") == 0)
        snprintf(rsp, BUFSIZ, "%i", mon->bottom_padding);
    else if (strcmp(name, "focused_border_color") == 0)
        snprintf(rsp, BUFSIZ, "%s (%06X)", focused_border_color, focused_border_color_pxl);
    else if (strcmp(name, "active_border_color") == 0)
        snprintf(rsp, BUFSIZ, "%s (%06X)", active_border_color, active_border_color_pxl);
    else if (strcmp(name, "normal_border_color") == 0)
        snprintf(rsp, BUFSIZ, "%s (%06X)", normal_border_color, normal_border_color_pxl);
    else if (strcmp(name, "presel_border_color") == 0)
        snprintf(rsp, BUFSIZ, "%s (%06X)", presel_border_color, presel_border_color_pxl);
    else if (strcmp(name, "focused_locked_border_color") == 0)
        snprintf(rsp, BUFSIZ, "%s (%06X)", focused_locked_border_color, focused_locked_border_color_pxl);
    else if (strcmp(name, "active_locked_border_color") == 0)
        snprintf(rsp, BUFSIZ, "%s (%06X)", active_locked_border_color, active_locked_border_color_pxl);
    else if (strcmp(name, "normal_locked_border_color") == 0)
        snprintf(rsp, BUFSIZ, "%s (%06X)", normal_locked_border_color, normal_locked_border_color_pxl);
    else if (strcmp(name, "urgent_border_color") == 0)
        snprintf(rsp, BUFSIZ, "%s (%06X)", urgent_border_color, urgent_border_color_pxl);
    else if (strcmp(name, "borderless_monocle") == 0)
        snprintf(rsp, BUFSIZ, "%s", BOOLSTR(borderless_monocle));
    else if (strcmp(name, "gapless_monocle") == 0)
        snprintf(rsp, BUFSIZ, "%s", BOOLSTR(gapless_monocle));
    else if (strcmp(name, "focus_follows_pointer") == 0)
        snprintf(rsp, BUFSIZ, "%s", BOOLSTR(focus_follows_pointer));
    else if (strcmp(name, "pointer_follows_monitor") == 0)
        snprintf(rsp, BUFSIZ, "%s", BOOLSTR(pointer_follows_monitor));
    else if (strcmp(name, "monitor_focus_fallback") == 0)
        snprintf(rsp, BUFSIZ, "%s", BOOLSTR(monitor_focus_fallback));
    else if (strcmp(name, "adaptative_raise") == 0)
        snprintf(rsp, BUFSIZ, "%s", BOOLSTR(adaptative_raise));
    else if (strcmp(name, "apply_shadow_property") == 0)
        snprintf(rsp, BUFSIZ, "%s", BOOLSTR(apply_shadow_property));
    else if (strcmp(name, "auto_alternate") == 0)
        snprintf(rsp, BUFSIZ, "%s", BOOLSTR(auto_alternate));
    else if (strcmp(name, "focus_by_distance") == 0)
        snprintf(rsp, BUFSIZ, "%s", BOOLSTR(focus_by_distance));
    else if (strcmp(name, "history_aware_focus") == 0)
        snprintf(rsp, BUFSIZ, "%s", BOOLSTR(history_aware_focus));
    else if (strcmp(name, "wm_name") == 0)
        snprintf(rsp, BUFSIZ, "%s", wm_name);
    else
        snprintf(rsp, BUFSIZ, "unknown setting: %s", name);
}

bool parse_bool(char *value, bool *b)
{
    if (strcmp(value, "true") == 0) {
        *b = true;
        return true;
    } else if (strcmp(value, "false") == 0) {
        *b = false;
        return true;
    }
    return false;
}

bool parse_layout(char *s, layout_t *l)
{
    if (strcmp(s, "monocle") == 0) {
        *l = LAYOUT_MONOCLE;
        return true;
    } else if (strcmp(s, "tiled") == 0) {
        *l = LAYOUT_TILED;
        return true;
    }
    return false;
}

bool parse_direction(char *s, direction_t *d)
{
    if (strcmp(s, "up") == 0) {
        *d = DIR_UP;
        return true;
    } else if (strcmp(s, "down") == 0) {
        *d = DIR_DOWN;
        return true;
    } else if (strcmp(s, "left") == 0) {
        *d = DIR_LEFT;
        return true;
    } else if (strcmp(s, "right") == 0) {
        *d = DIR_RIGHT;
        return true;
    }
    return false;
}

bool parse_nearest_argument(char *s, nearest_arg_t *a)
{
    if (strcmp(s, "older") == 0) {
        *a = NEAREST_OLDER;
        return true;
    } else if (strcmp(s, "newer") == 0) {
        *a = NEAREST_NEWER;
        return true;
    }
    return false;
}

bool parse_cycle_direction(char *s, cycle_dir_t *d)
{
    if (strcmp(s, "prev") == 0) {
        *d = CYCLE_PREV;
        return true;
    } else if (strcmp(s, "next") == 0) {
        *d = CYCLE_NEXT;
        return true;
    }
    return false;
}

bool parse_circulate_direction(char *s, circulate_dir_t *d)
{
    if (strcmp(s, "forward") == 0) {
        *d = CIRCULATE_FORWARD;
        return true;
    } else if (strcmp(s, "backward") == 0) {
        *d = CIRCULATE_BACKWARD;
        return true;
    }
    return false;
}

bool parse_skip_client(char *s, skip_client_t *k)
{
    if (s == NULL) {
        *k = CLIENT_SKIP_NONE;
        return true;
    } else if (strcmp(s, "--skip-floating") == 0) {
        *k = CLIENT_SKIP_FLOATING;
        return true;
    } else if (strcmp(s, "--skip-tiled") == 0) {
        *k = CLIENT_SKIP_TILED;
        return true;
    } else if (strcmp(s, "--skip-class-equal") == 0) {
        *k = CLIENT_SKIP_CLASS_EQUAL;
        return true;
    } else if (strcmp(s, "--skip-class-differ") == 0) {
        *k = CLIENT_SKIP_CLASS_DIFFER;
        return true;
    }
    return false;
}

bool parse_skip_desktop(char *s, skip_desktop_t *k)
{
    if (s == NULL) {
        *k = DESKTOP_SKIP_NONE;
        return true;
    } else if (strcmp(s, "--skip-free") == 0) {
        *k = DESKTOP_SKIP_FREE;
        return true;
    } else if (strcmp(s, "--skip-occupied") == 0) {
        *k = DESKTOP_SKIP_OCCUPIED;
        return true;
    }
    return false;
}

bool parse_list_option(char *s, list_option_t *o)
{
    if (s == NULL) {
        *o = LIST_OPTION_VERBOSE;
        return true;
    } else if (strcmp(s, "--quiet") == 0) {
        *o = LIST_OPTION_QUIET;
        return true;
    }
    return false;
}

bool parse_send_option(char *s, send_option_t *o)
{
    if (s == NULL) {
        *o = SEND_OPTION_DONT_FOLLOW;
        return true;
    } else if (strcmp(s, "--follow") == 0) {
        *o = SEND_OPTION_FOLLOW;
        return true;
    }
    return false;
}

bool parse_swap_option(char *s, swap_option_t *o)
{
    if (s == NULL) {
        *o = SWAP_OPTION_SWAP_FOCUS;
        return true;
    } else if (strcmp(s, "--keep-focus") == 0) {
        *o = SWAP_OPTION_KEEP_FOCUS;
        return true;
    }
    return false;
}

bool parse_cancel_option(char *s, cancel_option_t *o)
{
    if (s == NULL) {
        *o = CANCEL_OPTION_FOCUSED;
        return true;
    } else if (strcmp(s, "--all") == 0) {
        *o = CANCEL_OPTION_ALL;
        return true;
    }
    return false;
}

bool parse_rotate(char *s, rotate_t *r)
{
    if (strcmp(s, "clockwise") == 0) {
        *r = ROTATE_CLOCKWISE;
        return true;
    } else if (strcmp(s, "counter_clockwise") == 0) {
        *r = ROTATE_COUNTER_CLOCKWISE;
        return true;
    } else if (strcmp(s, "full_cycle") == 0) {
        *r = ROTATE_FULL_CYCLE;
        return true;
    }
    return false;
}

bool parse_flip(char *s, flip_t *f)
{
    if (strcmp(s, "horizontal") == 0) {
        *f = FLIP_HORIZONTAL;
        return true;
    } else if (strcmp(s, "vertical") == 0) {
        *f = FLIP_VERTICAL;
        return true;
    }
    return false;
}

bool parse_fence_move(char *s, fence_move_t *m)
{
    if (strcmp(s, "push") == 0) {
        *m = MOVE_PUSH;
        return true;
    } else if (strcmp(s, "pull") == 0) {
        *m = MOVE_PULL;
        return true;
    }
    return false;
}

bool parse_pointer_action(char *s, pointer_action_t *a)
{
    if (strcmp(s, "move") == 0) {
        *a = ACTION_MOVE;
        return true;
    } else if (strcmp(s, "resize_corner") == 0) {
        *a = ACTION_RESIZE_CORNER;
        return true;
    } else if (strcmp(s, "resize_side") == 0) {
        *a = ACTION_RESIZE_SIDE;
        return true;
    } else if (strcmp(s, "focus") == 0) {
        *a = ACTION_FOCUS;
        return true;
    }
    return false;
}

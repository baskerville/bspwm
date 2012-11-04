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
#include "tree.h"

void process_message(char *msg, char *rsp)
{
    char *cmd = strtok(msg, TOKEN_SEP);

    if (cmd == NULL)
        return;

    if (strcmp(cmd, "get") == 0) {
        char *name = strtok(NULL, TOKEN_SEP);
        get_setting(name, rsp);
        return;
    } else if (strcmp(cmd, "set") == 0) {
        char *name = strtok(NULL, TOKEN_SEP);
        char *value = strtok(NULL, TOKEN_SEP);
        set_setting(name, value, rsp);
        return;
    } else if (strcmp(cmd, "list") == 0) {
        dump_tree(mon->desk, mon->desk->root, rsp, 0);
        return;
    } else if (strcmp(cmd, "list_monitors") == 0) {
        char *arg = strtok(NULL, TOKEN_SEP);
        list_option_t opt;
        if (parse_list_option(arg, &opt))
            list_monitors(opt, rsp);
        return;
    } else if (strcmp(cmd, "list_desktops") == 0) {
        char *arg = strtok(NULL, TOKEN_SEP);
        list_option_t opt;
        if (parse_list_option(arg, &opt))
            list_desktops(mon, opt, 0, rsp);
        return;
    } else if (strcmp(cmd, "list_windows") == 0) {
        list_windows(rsp);
        return;
    } else if (strcmp(cmd, "list_history") == 0) {
        list_history(mon->desk, rsp);
        return;
    } else if (strcmp(cmd, "close") == 0) {
        window_close(mon->desk->focus);
        return;
    } else if (strcmp(cmd, "kill") == 0) {
        window_kill(mon->desk, mon->desk->focus);
    } else if (strcmp(cmd, "magnetise") == 0) {
        char *cor = strtok(NULL, TOKEN_SEP);
        if (cor != NULL) {
            corner_t c;
            if (parse_corner(cor, &c)) {
                magnetise_tree(mon->desk->root, c);
            }
        }
    } else if (strcmp(cmd, "rotate") == 0) {
        char *deg = strtok(NULL, TOKEN_SEP);
        if (deg != NULL) {
            rotate_t r;
            if (parse_rotate(deg, &r)) {
                rotate_tree(mon->desk->root, r);
            }
        }
    } else if (strcmp(cmd, "layout") == 0) {
        char *lyt = strtok(NULL, TOKEN_SEP);
        if (lyt != NULL) {
            layout_t l;
            if (parse_layout(lyt, &l)) {
                mon->desk->layout = l;
            }
        }
    } else if (strcmp(cmd, "cycle_layout") == 0) {
        if (mon->desk->layout == LAYOUT_MONOCLE)
            mon->desk->layout = LAYOUT_TILED;
        else
            mon->desk->layout = LAYOUT_MONOCLE;
    } else if (strcmp(cmd, "shift") == 0) {
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            direction_t d;
            if (parse_direction(dir, &d)) {
                swap_nodes(mon->desk->focus, find_neighbor(mon->desk->focus, d));
            }
        }
    } else if (strcmp(cmd, "toggle_fullscreen") == 0) {
        if (mon->desk->focus != NULL)
            toggle_fullscreen(mon, mon->desk->focus->client);
    } else if (strcmp(cmd, "toggle_floating") == 0) {
        split_mode = MODE_AUTOMATIC;
        toggle_floating(mon->desk->focus);
    } else if (strcmp(cmd, "toggle_locked") == 0) {
        if (mon->desk->focus != NULL)
            toggle_locked(mon->desk->focus->client);
    } else if (strcmp(cmd, "ratio") == 0) {
        char *value = strtok(NULL, TOKEN_SEP);
        if (value != NULL && mon->desk->focus != NULL)
            sscanf(value, "%lf", &mon->desk->focus->split_ratio);
    } else if (strcmp(cmd, "cancel") == 0) {
        split_mode = MODE_AUTOMATIC;
        window_draw_border(mon->desk->focus, true, true);
        return;
    } else if (strcmp(cmd, "presel") == 0) {
        if (mon->desk->focus == NULL || !is_tiled(mon->desk->focus->client) || mon->desk->layout != LAYOUT_TILED)
            return;
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            direction_t d;
            if (parse_direction(dir, &d)) {
                split_mode = MODE_MANUAL;
                split_dir = d;
                window_draw_border(mon->desk->focus, true, true);
            }
        }
        return;
    } else if (strcmp(cmd, "push") == 0 || strcmp(cmd, "pull") == 0) {
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            fence_move_t m;
            direction_t d;
            if (parse_fence_move(cmd, &m) && parse_direction(dir, &d)) {
                move_fence(mon->desk->focus, d, m);
            }
        }
    } else if (strcmp(cmd, "send_to_monitor") == 0) {
        char *name = strtok(NULL, TOKEN_SEP);
        if (name != NULL) {
            monitor_t *m = find_monitor(name);
            if (m != NULL && m != mon) {
                transfer_node(mon, mon->desk, m, m->desk, mon->desk->focus);
                arrange(m, m->desk);
            }
        }
    } else if (strcmp(cmd, "send_to") == 0) {
        char *name = strtok(NULL, TOKEN_SEP);
        if (name != NULL) {
            desktop_location_t loc;
            if (locate_desktop(name, &loc)) {
                transfer_node(mon, mon->desk, loc.monitor, loc.desktop, mon->desk->focus);
                if (mon != loc.monitor && loc.monitor->desk == loc.desktop)
                    arrange(loc.monitor, loc.desktop);
            }
        }
    } else if (strcmp(cmd, "rename_monitor") == 0) {
        char *cur_name = strtok(NULL, TOKEN_SEP);
        if (cur_name != NULL) {
            monitor_t *m = find_monitor(cur_name);
            if (m != NULL) {
                char *new_name = strtok(NULL, TOKEN_SEP);
                if (new_name != NULL) {
                    strncpy(m->name, new_name, sizeof(m->name));
                }
            }
        }
    } else if (strcmp(cmd, "rename") == 0) {
        char *cur_name = strtok(NULL, TOKEN_SEP);
        if (cur_name != NULL) {
            desktop_location_t loc;
            if (locate_desktop(cur_name, &loc)) {
                char *new_name = strtok(NULL, TOKEN_SEP);
                if (new_name != NULL) {
                    strncpy(loc.desktop->name, new_name, sizeof(loc.desktop->name));
                    ewmh_update_desktop_names();
                }
            }
        }
    } else if (strcmp(cmd, "use_monitor") == 0) {
        char *name = strtok(NULL, TOKEN_SEP);
        if (name != NULL) {
            monitor_t *m = find_monitor(name);
            if (m != NULL)
                select_monitor(m);
        }
    } else if (strcmp(cmd, "use") == 0) {
        char *name = strtok(NULL, TOKEN_SEP);
        if (name != NULL) {
            desktop_location_t loc;
            if (locate_desktop(name, &loc)) {
                select_monitor(loc.monitor);
                select_desktop(loc.desktop);
            }
        }
    } else if (strcmp(cmd, "cycle_monitor") == 0) {
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            cycle_dir_t d;
            if (parse_cycle_direction(dir, &d))
                cycle_monitor(d);
        }
    } else if (strcmp(cmd, "cycle_desktop") == 0) {
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            cycle_dir_t d;
            if (parse_cycle_direction(dir, &d)) {
                skip_desktop_t k;
                char *skip = strtok(NULL, TOKEN_SEP);
                if (parse_skip_desktop(skip, &k))
                    cycle_desktop(mon, mon->desk, d, k);
            }
        }
    } else if (strcmp(cmd, "cycle") == 0) {
        if (mon->desk->focus != NULL && mon->desk->focus->client->fullscreen)
            return;
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            cycle_dir_t d;
            if (parse_cycle_direction(dir, &d)) {
                skip_client_t k;
                char *skip = strtok(NULL, TOKEN_SEP);
                if (parse_skip_client(skip, &k))
                    cycle_leaf(mon, mon->desk, mon->desk->focus, d, k);
            }
        }
        if (mon->desk->layout == LAYOUT_TILED)
            return;
    } else if (strcmp(cmd, "nearest") == 0) {
        if (mon->desk->focus != NULL && mon->desk->focus->client->fullscreen)
            return;
        char *arg = strtok(NULL, TOKEN_SEP);
        if (arg != NULL) {
            nearest_arg_t a;
            if (parse_nearest_argument(arg, &a)) {
                skip_client_t k;
                char *skip = strtok(NULL, TOKEN_SEP);
                if (parse_skip_client(skip, &k))
                    nearest_leaf(mon, mon->desk, mon->desk->focus, a, k);
            }
        }
        if (mon->desk->layout == LAYOUT_TILED)
            return;
    } else if (strcmp(cmd, "circulate") == 0) {
        if (mon->desk->layout == LAYOUT_MONOCLE
                || (mon->desk->focus != NULL && !is_tiled(mon->desk->focus->client)))
            return;
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            circulate_dir_t d;
            if (parse_circulate_direction(dir, &d))
                circulate_leaves(mon, mon->desk, d);
        }
    } else if (strcmp(cmd, "rule") == 0) {
        char *name = strtok(NULL, TOKEN_SEP);
        if (name != NULL) {
            rule_t *rule = make_rule();
            strncpy(rule->cause.name, name, sizeof(rule->cause.name));
            char *arg = strtok(NULL, TOKEN_SEP);
            while (arg != NULL) {
                if (strcmp(arg, "floating") == 0)
                    rule->effect.floating = true;
                arg = strtok(NULL, TOKEN_SEP);
            }
            rule->next = rule_head;
            rule_head = rule;
        }
        return;
    } else if (strcmp(cmd, "alternate") == 0) {
        node_list_t *a = mon->desk->focus_history->head->prev;
        if (a != NULL)
            focus_node(mon, mon->desk, a->node, true);
    } else if (strcmp(cmd, "alternate_desktop") == 0) {
        select_desktop(mon->last_desk);
    } else if (strcmp(cmd, "alternate_monitor") == 0) {
        select_monitor(last_mon);
    } else if (strcmp(cmd, "add_in") == 0) {
        char *name = strtok(NULL, TOKEN_SEP);
        if (name != NULL) {
            monitor_t *m = find_monitor(name);
            if (m != NULL)
                for (name = strtok(NULL, TOKEN_SEP); name != NULL; name = strtok(NULL, TOKEN_SEP))
                    add_desktop(m, name);
        }
        return;
    } else if (strcmp(cmd, "add") == 0) {
        for (char *name = strtok(NULL, TOKEN_SEP); name != NULL; name = strtok(NULL, TOKEN_SEP))
            add_desktop(mon, name);
        return;
    } else if (strcmp(cmd, "focus") == 0) {
        if (mon->desk->focus != NULL && mon->desk->focus->client->fullscreen)
            return;
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            direction_t d;
            if (parse_direction(dir, &d)) {
                node_t *n = find_neighbor(mon->desk->focus, d);
                focus_node(mon, mon->desk, n, true);
            }
        }
        if (mon->desk->layout == LAYOUT_TILED)
            return;
    } else if (strcmp(cmd, "adopt_orphans") == 0) {
        adopt_orphans();
    } else if (strcmp(cmd, "reload") == 0) {
        load_settings();
        run_autostart();
    } else if (strcmp(cmd, "reload_autostart") == 0) {
        run_autostart();
    } else if (strcmp(cmd, "reload_settings") == 0) {
        load_settings();
    } else if (strcmp(cmd, "quit") == 0) {
        quit();
        return;
    } else {
        snprintf(rsp, BUFSIZ, "unknown command: %s", cmd);
        return;
    }

    arrange(mon, mon->desk);
}

void set_setting(char *name, char *value, char *rsp)
{
    if (name == NULL || value == NULL)
        return;

    if (strcmp(name, "inner_border_width") == 0) {
        sscanf(value, "%u", &inner_border_width);
        border_width = inner_border_width + main_border_width + outer_border_width;
    } else if (strcmp(name, "main_border_width") == 0) {
        sscanf(value, "%u", &main_border_width);
        border_width = inner_border_width + main_border_width + outer_border_width;
    } else if (strcmp(name, "outer_border_width") == 0) {
        sscanf(value, "%u", &outer_border_width);
        border_width = inner_border_width + main_border_width + outer_border_width;
    } else if (strcmp(name, "window_gap") == 0) {
        sscanf(value, "%i", &window_gap);
    } else if (strcmp(name, "left_padding") == 0) {
        sscanf(value, "%i", &left_padding);
    } else if (strcmp(name, "right_padding") == 0) {
        sscanf(value, "%i", &right_padding);
    } else if (strcmp(name, "top_padding") == 0) {
        sscanf(value, "%i", &top_padding);
    } else if (strcmp(name, "bottom_padding") == 0) {
        sscanf(value, "%i", &bottom_padding);
    } else if (strcmp(name, "focused_border_color") == 0) {
        strncpy(focused_border_color, value, sizeof(focused_border_color));
        focused_border_color_pxl = get_color(focused_border_color);
    } else if (strcmp(name, "active_border_color") == 0) {
        strncpy(active_border_color, value, sizeof(active_border_color));
        active_border_color_pxl = get_color(active_border_color);
    } else if (strcmp(name, "normal_border_color") == 0) {
        strncpy(normal_border_color, value, sizeof(normal_border_color));
        normal_border_color_pxl = get_color(normal_border_color);
    } else if (strcmp(name, "inner_border_color") == 0) {
        strncpy(inner_border_color, value, sizeof(inner_border_color));
        inner_border_color_pxl = get_color(inner_border_color);
    } else if (strcmp(name, "outer_border_color") == 0) {
        strncpy(outer_border_color, value, sizeof(outer_border_color));
        outer_border_color_pxl = get_color(outer_border_color);
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
    } else if (strcmp(name, "focus_follows_mouse") == 0) {
        bool b;
        if (parse_bool(value, &b))
            focus_follows_mouse = b;
    } else if (strcmp(name, "wm_name") == 0) {
        strncpy(wm_name, value, sizeof(wm_name));
        ewmh_update_wm_name();
        return;
    } else if (strcmp(name, "button_modifier") == 0) {
        unsigned int m;
        if (parse_modifier_mask(value, &m)) {
            ungrab_buttons();
            button_modifier = m;
            grab_buttons();
        }
        return;
    } else {
        snprintf(rsp, BUFSIZ, "unknown setting: %s", name);
        return;
    }

    arrange(mon, mon->desk);
}

void get_setting(char *name, char* rsp)
{
    if (name == NULL)
        return;

    if (strcmp(name, "inner_border_width") == 0)
        snprintf(rsp, BUFSIZ, "%u", inner_border_width);
    else if (strcmp(name, "main_border_width") == 0)
        snprintf(rsp, BUFSIZ, "%u", main_border_width);
    else if (strcmp(name, "outer_border_width") == 0)
        snprintf(rsp, BUFSIZ, "%u", outer_border_width);
    else if (strcmp(name, "border_width") == 0)
        snprintf(rsp, BUFSIZ, "%u", border_width);
    else if (strcmp(name, "window_gap") == 0)
        snprintf(rsp, BUFSIZ, "%i", window_gap);
    else if (strcmp(name, "left_padding") == 0)
        snprintf(rsp, BUFSIZ, "%i", left_padding);
    else if (strcmp(name, "right_padding") == 0)
        snprintf(rsp, BUFSIZ, "%i", right_padding);
    else if (strcmp(name, "top_padding") == 0)
        snprintf(rsp, BUFSIZ, "%i", top_padding);
    else if (strcmp(name, "bottom_padding") == 0)
        snprintf(rsp, BUFSIZ, "%i", bottom_padding);
    else if (strcmp(name, "focused_border_color") == 0)
        snprintf(rsp, BUFSIZ, "%s (%06X)", focused_border_color, focused_border_color_pxl);
    else if (strcmp(name, "active_border_color") == 0)
        snprintf(rsp, BUFSIZ, "%s (%06X)", active_border_color, active_border_color_pxl);
    else if (strcmp(name, "normal_border_color") == 0)
        snprintf(rsp, BUFSIZ, "%s (%06X)", normal_border_color, normal_border_color_pxl);
    else if (strcmp(name, "inner_border_color") == 0)
        snprintf(rsp, BUFSIZ, "%s (%06X)", inner_border_color, inner_border_color_pxl);
    else if (strcmp(name, "outer_border_color") == 0)
        snprintf(rsp, BUFSIZ, "%s (%06X)", outer_border_color, outer_border_color_pxl);
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
    else if (strcmp(name, "focus_follows_mouse") == 0)
        snprintf(rsp, BUFSIZ, "%s", BOOLSTR(focus_follows_mouse));
    else if (strcmp(name, "wm_name") == 0)
        snprintf(rsp, BUFSIZ, "%s", wm_name);
    else if (strcmp(name, "button_modifier") == 0)
        print_modifier_mask(rsp, button_modifier);
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
    if (s == NULL || strcmp(s, "--verbose") == 0) {
        *o = LIST_OPTION_VERBOSE;
        return true;
    } else if (strcmp(s, "--quiet") == 0) {
        *o = LIST_OPTION_QUIET;
        return true;
    }
    return false;
}

bool parse_corner(char *s, corner_t *c)
{
    if (strcmp(s, "top_left") == 0) {
        *c = TOP_LEFT;
        return true;
    } else if (strcmp(s, "top_right") == 0) {
        *c = TOP_RIGHT;
        return true;
    } else if (strcmp(s, "bottom_left") == 0) {
        *c = BOTTOM_LEFT;
        return true;
    } else if (strcmp(s, "bottom_right") == 0) {
        *c = BOTTOM_RIGHT;
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

bool parse_modifier_mask(char *s, unsigned int *m)
{
    if (strcmp(s, "mod1") == 0) {
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

void print_modifier_mask(char *s, unsigned int m)
{
    switch(m) {
        case XCB_MOD_MASK_1:
            snprintf(s, BUFSIZ, "mod1");
            break;
        case XCB_MOD_MASK_2:
            snprintf(s, BUFSIZ, "mod2");
            break;
        case XCB_MOD_MASK_3:
            snprintf(s, BUFSIZ, "mod3");
            break;
        case XCB_MOD_MASK_4:
            snprintf(s, BUFSIZ, "mod4");
            break;
        case XCB_MOD_MASK_5:
            snprintf(s, BUFSIZ, "mod5");
            break;
    }
}

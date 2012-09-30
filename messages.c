#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "settings.h"
#include "messages.h"
#include "common.h"
#include "types.h"
#include "bspwm.h"
#include "ewmh.h"
#include "misc.h"
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
    } else if (strcmp(cmd, "dump") == 0) {
        dump_tree(desk, desk->root, rsp, 0);
        return;
    } else if (strcmp(cmd, "list") == 0) {
        list_desktops(rsp);
        return;
    } else if (strcmp(cmd, "windows") == 0) {
        list_windows(rsp);
        return;
    } else if (strcmp(cmd, "close") == 0) {
        window_close(desk->focus);
    } else if (strcmp(cmd, "kill") == 0) {
        window_kill(desk, desk->focus);
    } else if (strcmp(cmd, "magnetise") == 0) {
        char *cor = strtok(NULL, TOKEN_SEP);
        if (cor != NULL) {
            corner_t c;
            if (parse_corner(cor, &c)) {
                magnetise_tree(desk->root, c);
            }
        }
    } else if (strcmp(cmd, "rotate") == 0) {
        char *deg = strtok(NULL, TOKEN_SEP);
        if (deg != NULL) {
            rotate_t r;
            if (parse_rotate(deg, &r)) {
                rotate_tree(desk->root, r);
            }
        }
    } else if (strcmp(cmd, "layout") == 0) {
        char *lyt = strtok(NULL, TOKEN_SEP);
        if (lyt != NULL) {
            layout_t l;
            if (parse_layout(lyt, &l)) {
                desk->layout = l;
            }
        }
    } else if (strcmp(cmd, "shift") == 0) {
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            direction_t d;
            if (parse_direction(dir, &d)) {
                swap_nodes(desk->focus, find_neighbor(desk->focus, d));
            }
        }
    } else if (strcmp(cmd, "toggle_fullscreen") == 0) {
        if (desk->focus != NULL)
            toggle_fullscreen(desk->focus->client);
        return;
    } else if (strcmp(cmd, "toggle_floating") == 0) {
        split_mode = MODE_AUTOMATIC;
        toggle_floating(desk->focus);
    } else if (strcmp(cmd, "toggle_locked") == 0) {
        if (desk->focus != NULL)
            toggle_locked(desk->focus->client);
    } else if (strcmp(cmd, "ratio") == 0) {
        char *value = strtok(NULL, TOKEN_SEP);
        if (value != NULL && desk->focus != NULL)
            sscanf(value, "%lf", &desk->focus->split_ratio);
    } else if (strcmp(cmd, "cancel") == 0) {
        split_mode = MODE_AUTOMATIC;
        window_draw_border(desk->focus, true);
        return;
    } else if (strcmp(cmd, "presel") == 0) {
        if (desk->focus == NULL || !is_tiled(desk->focus->client) || desk->layout != LAYOUT_TILED)
            return;
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            direction_t d;
            if (parse_direction(dir, &d)) {
                split_mode = MODE_MANUAL;
                split_dir = d;
                window_draw_border(desk->focus, true);
            }
        }
        return;
    } else if (strcmp(cmd, "push") == 0 || strcmp(cmd, "pull") == 0) {
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            fence_move_t m;
            direction_t d;
            if (parse_fence_move(cmd, &m) && parse_direction(dir, &d)) {
                move_fence(desk->focus, d, m);
            }
        }
    } else if (strcmp(cmd, "send_to") == 0) {
        char *name = strtok(NULL, TOKEN_SEP);
        if (name != NULL) {
            desktop_t *d = find_desktop(name);
            transfer_node(desk, d, desk->focus);
        }
    } else if (strcmp(cmd, "rename") == 0) {
        char *cur_name = strtok(NULL, TOKEN_SEP);
        if (cur_name != NULL) {
            desktop_t *d = find_desktop(cur_name);
            if (d != NULL) {
                char *new_name = strtok(NULL, TOKEN_SEP);
                if (new_name != NULL) {
                    strncpy(d->name, new_name, sizeof(d->name));
                    ewmh_update_desktop_names();
                }
            }
        }
    } else if (strcmp(cmd, "use") == 0) {
        char *name = strtok(NULL, TOKEN_SEP);
        if (name != NULL) {
            desktop_t *d = find_desktop(name);
            select_desktop(d);
        }
    } else if (strcmp(cmd, "cycle_desktop") == 0) {
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            cycle_dir_t d;
            if (parse_cycle_direction(dir, &d)) {
                cycle_desktop(d);
            }
        }
    } else if (strcmp(cmd, "cycle") == 0) {
        if (desk->focus != NULL && desk->focus->client->fullscreen)
            return;
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            cycle_dir_t d;
            if (parse_cycle_direction(dir, &d)) {
                skip_client_t k;
                char *skip = strtok(NULL, TOKEN_SEP);
                if (parse_skip_client(skip, &k))
                    cycle_leaf(desk, desk->focus, d, k);
            }
        }
        return;
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
        select_desktop(last_desk);
    } else if (strcmp(cmd, "add") == 0) {
        char *name = strtok(NULL, TOKEN_SEP);
        if (name != NULL) {
            add_desktop(name);
        }
        return;
    } else if (strcmp(cmd, "focus") == 0) {
        if (desk->focus != NULL && desk->focus->client->fullscreen)
            return;
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            direction_t d;
            if (parse_direction(dir, &d)) {
                node_t *n = find_neighbor(desk->focus, d);
                focus_node(desk, n, true);
            }
        }
        return;
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
        snprintf(rsp, BUFSIZ, "unknown command: %s\n", cmd);
        return;
    }

    apply_layout(desk, desk->root, root_rect);
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
        update_root_dimensions();
    } else if (strcmp(name, "left_padding") == 0) {
        sscanf(value, "%i", &left_padding);
        update_root_dimensions();
    } else if (strcmp(name, "right_padding") == 0) {
        sscanf(value, "%i", &right_padding);
        update_root_dimensions();
    } else if (strcmp(name, "top_padding") == 0) {
        sscanf(value, "%i", &top_padding);
        update_root_dimensions();
    } else if (strcmp(name, "bottom_padding") == 0) {
        sscanf(value, "%i", &bottom_padding);
        update_root_dimensions();
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
    } else if (strcmp(name, "wm_name") == 0) {
        strncpy(wm_name, value, sizeof(wm_name));
        ewmh_update_wm_name();
        return;
    } else {
        snprintf(rsp, BUFSIZ, "unknown setting: %s\n", name);
        return;
    }

    apply_layout(desk, desk->root, root_rect);
}

void get_setting(char *name, char* rsp)
{
    if (name == NULL)
        return;

    if (strcmp(name, "inner_border_width") == 0)
        sprintf(rsp, "%u\n", inner_border_width);
    else if (strcmp(name, "main_border_width") == 0)
        sprintf(rsp, "%u\n", main_border_width);
    else if (strcmp(name, "outer_border_width") == 0)
        sprintf(rsp, "%u\n", outer_border_width);
    else if (strcmp(name, "border_width") == 0)
        sprintf(rsp, "%u\n", border_width);
    else if (strcmp(name, "window_gap") == 0)
        sprintf(rsp, "%i\n", window_gap);
    else if (strcmp(name, "left_padding") == 0)
        sprintf(rsp, "%i\n", left_padding);
    else if (strcmp(name, "right_padding") == 0)
        sprintf(rsp, "%i\n", right_padding);
    else if (strcmp(name, "top_padding") == 0)
        sprintf(rsp, "%i\n", top_padding);
    else if (strcmp(name, "bottom_padding") == 0)
        sprintf(rsp, "%i\n", bottom_padding);
    else if (strcmp(name, "active_border_color") == 0)
        sprintf(rsp, "%s (%06X)\n", active_border_color, active_border_color_pxl);
    else if (strcmp(name, "normal_border_color") == 0)
        sprintf(rsp, "%s (%06X)\n", normal_border_color, normal_border_color_pxl);
    else if (strcmp(name, "inner_border_color") == 0)
        sprintf(rsp, "%s (%06X)\n", inner_border_color, inner_border_color_pxl);
    else if (strcmp(name, "outer_border_color") == 0)
        sprintf(rsp, "%s (%06X)\n", outer_border_color, outer_border_color_pxl);
    else if (strcmp(name, "presel_border_color") == 0)
        sprintf(rsp, "%s (%06X)\n", presel_border_color, presel_border_color_pxl);
    else if (strcmp(name, "active_locked_border_color") == 0)
        sprintf(rsp, "%s (%06X)\n", active_locked_border_color, active_locked_border_color_pxl);
    else if (strcmp(name, "normal_locked_border_color") == 0)
        sprintf(rsp, "%s (%06X)\n", normal_locked_border_color, normal_locked_border_color_pxl);
    else if (strcmp(name, "urgent_border_color") == 0)
        sprintf(rsp, "%s (%06X)\n", urgent_border_color, urgent_border_color_pxl);
    else if (strcmp(name, "borderless_monocle") == 0)
        sprintf(rsp, "%s\n", BOOLSTR(borderless_monocle));
    else if (strcmp(name, "wm_name") == 0)
        sprintf(rsp, "%s\n", wm_name);
    else
        snprintf(rsp, BUFSIZ, "unknown setting: %s\n", name);
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

bool parse_skip_client(char *s, skip_client_t *k)
{
    if (s == NULL || strcmp(s, "--skip-none") == 0) {
        *k = SKIP_NONE;
        return true;
    } else if (strcmp(s, "--skip-floating") == 0) {
        *k = SKIP_FLOATING;
        return true;
    } else if (strcmp(s, "--skip-tiled") == 0) {
        *k = SKIP_TILED;
        return true;
    } else if (strcmp(s, "--skip-class-equal") == 0) {
        *k = SKIP_CLASS_EQUAL;
        return true;
    } else if (strcmp(s, "--skip-class-differ") == 0) {
        *k = SKIP_CLASS_DIFFER;
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


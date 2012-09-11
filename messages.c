#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "luautils.h"
#include "settings.h"
#include "messages.h"
#include "common.h"
#include "types.h"
#include "bspwm.h"
#include "utils.h"
#include "tree.h"

void process_message(char *msg, char *rsp)
{
    char *cmd = strtok(msg, TOKEN_SEP);

    if (cmd == NULL)
        return;

    if (strcmp(cmd, "get") == 0) {
        char *name = strtok(NULL, TOKEN_SEP);
        get_setting(name, rsp);
    } else if (strcmp(cmd, "set") == 0) {
        char *name = strtok(NULL, TOKEN_SEP);
        char *value = strtok(NULL, TOKEN_SEP);
        set_setting(name, value);
    } else if (strcmp(cmd, "dump") == 0) {
        dump_tree(desk->root, rsp, 0);
    } else if (strcmp(cmd, "layout") == 0) {
        char *arg = strtok(NULL, TOKEN_SEP);
        if (arg != NULL) {
            layout_t l;
            if (parse_layout(arg, &l)) {
                desk->layout = l;
                apply_layout(desk, desk->root);
            }
        }
    } else if (strcmp(cmd, "split_ratio") == 0) {
        char *value = strtok(NULL, TOKEN_SEP);
        split_ratio_cmd(value);
    } else if (strcmp(cmd, "presel") == 0) {
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            split_mode = MODE_MANUAL;
            split_dir = parse_direction(dir);
            draw_triple_border(desk->focus, active_border_color_pxl);
        }
    } else if (strcmp(cmd, "push") == 0 || strcmp(cmd, "pull") == 0) {
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            fence_move_t m = parse_fence_move(cmd);
            direction_t d = parse_direction(dir);
            move_fence(desk->focus, d, m);
        }
    } else if (strcmp(cmd, "move") == 0) {
        char *dir = strtok(NULL, TOKEN_SEP);
        if (dir != NULL) {
            direction_t d = parse_direction(dir);
            node_t *n = find_neighbor(desk->focus, d);
            focus_node(desk, n);
        }
    } else if (strcmp(cmd, "quit") == 0) {
        quit();
    }
}

void get_setting(char *name, char* rsp)
{
    if (name == NULL)
        return;

    if (strcmp(name, "inner_border_width") == 0)
        sprintf(rsp, "%i\n", inner_border_width);
    else if (strcmp(name, "main_border_width") == 0)
        sprintf(rsp, "%i\n", main_border_width);
    else if (strcmp(name, "outer_border_width") == 0)
        sprintf(rsp, "%i\n", outer_border_width);
    else if (strcmp(name, "border_width") == 0)
        sprintf(rsp, "%i\n", border_width);
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
    else if (strcmp(name, "normal_border_color") == 0)
        sprintf(rsp, "%s\n", normal_border_color);
    else if (strcmp(name, "active_border_color") == 0)
        sprintf(rsp, "%s\n", active_border_color);
    else if (strcmp(name, "inner_border_color") == 0)
        sprintf(rsp, "%s\n", inner_border_color);
    else if (strcmp(name, "outer_border_color") == 0)
        sprintf(rsp, "%s\n", outer_border_color);
    else if (strcmp(name, "presel_border_color") == 0)
        sprintf(rsp, "%s\n", presel_border_color);
    else if (strcmp(name, "locked_border_color") == 0)
        sprintf(rsp, "%s\n", locked_border_color);
    else if (strcmp(name, "wm_name") == 0)
        sprintf(rsp, "%s\n", wm_name);
    else if (strcmp(name, "adaptive_window_border") == 0)
        sprintf(rsp, "%s\n", BOOLSTR(adaptive_window_border));
    else if (strcmp(name, "adaptive_window_gap") == 0)
        sprintf(rsp, "%s\n", BOOLSTR(adaptive_window_gap));
}

void set_setting(char *name, char *value)
{
    if (name == NULL || value == NULL)
        return;

    if (strcmp(name, "inner_border_width") == 0) {
        sscanf(value, "%i", &inner_border_width);
        border_width = inner_border_width + main_border_width + outer_border_width;
    } else if (strcmp(name, "main_border_width") == 0) {
        sscanf(value, "%i", &main_border_width);
        border_width = inner_border_width + main_border_width + outer_border_width;
    } else if (strcmp(name, "outer_border_width") == 0) {
        sscanf(value, "%i", &outer_border_width);
        border_width = inner_border_width + main_border_width + outer_border_width;
    } else if (strcmp(name, "window_gap") == 0) {
        sscanf(value, "%i", &window_gap);
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
    } else if (strcmp(name, "normal_border_color") == 0) {
        strncpy(normal_border_color, value, sizeof(normal_border_color));
        normal_border_color_pxl = get_color(normal_border_color);
    } else if (strcmp(name, "active_border_color") == 0) {
        strncpy(active_border_color, value, sizeof(active_border_color));
        active_border_color_pxl = get_color(active_border_color);
    } else if (strcmp(name, "inner_border_color") == 0) {
        strncpy(inner_border_color, value, sizeof(inner_border_color));
        inner_border_color_pxl = get_color(inner_border_color);
    } else if (strcmp(name, "outer_border_color") == 0) {
        strncpy(outer_border_color, value, sizeof(outer_border_color));
        outer_border_color_pxl = get_color(outer_border_color);
    } else if (strcmp(name, "presel_border_color") == 0) {
        strncpy(presel_border_color, value, sizeof(presel_border_color));
        presel_border_color_pxl = get_color(presel_border_color);
    } else if (strcmp(name, "locked_border_color") == 0) {
        strncpy(locked_border_color, value, sizeof(locked_border_color));
        locked_border_color_pxl = get_color(locked_border_color);
    } else if (strcmp(name, "adaptive_window_border") == 0) {
        if (is_bool(value))
            adaptive_window_border = parse_bool(value);
    } else if (strcmp(name, "adaptive_window_gap") == 0) {
        if (is_bool(value))
            adaptive_window_gap = parse_bool(value);
    } else if (strcmp(name, "wm_name") == 0) {
        strncpy(wm_name, value, sizeof(wm_name));
        return;
    }
    apply_layout(desk, desk->root);
}

void split_ratio_cmd(char *value)
{
    if (value == NULL || desk == NULL || desk->focus == NULL)
        return;

    sscanf(value, "%lf", &desk->focus->split_ratio);
}

bool is_bool(char *value)
{
    if (value == NULL)
        return false;

    return (strcmp(value, "true") == 0 || strcmp(value, "false") == 0);
}

bool parse_bool(char *value)
{
    if (strcmp(value, "true") == 0)
        return true;
    else if (strcmp(value, "false") == 0)
        return false;
    return true;
}

bool parse_layout(char *s, layout_t *l) {
    if (strcmp(s, "monocle") == 0) {
        *l = LAYOUT_MONOCLE;
        return true;
    } else if (strcmp(s, "tiled") == 0) {
        *l = LAYOUT_TILED;
        return true;
    }
    return false;
}

direction_t parse_direction(char *dir) {
    if (strcmp(dir, "up") == 0)
        return DIR_UP;
    else if (strcmp(dir, "down") == 0)
        return DIR_DOWN;
    else if (strcmp(dir, "left") == 0)
        return DIR_LEFT;
    else if (strcmp(dir, "right") == 0)
        return DIR_RIGHT;
    return DIR_LEFT;
}

fence_move_t parse_fence_move(char *mov) {
    if (strcmp(mov, "push") == 0)
        return MOVE_PUSH;
    else if (strcmp(mov, "pull") == 0)
        return MOVE_PULL;
    return MOVE_PUSH;
}

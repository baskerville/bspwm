#define _BSD_SOURCE

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
    } else if (strcmp(cmd, "split_ratio") == 0) {
        char *value = strtok(NULL, TOKEN_SEP);
        split_ratio_cmd(value);
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
    } else if (strcmp(name, "right_padding") == 0) {
        sscanf(value, "%i", &right_padding);
    } else if (strcmp(name, "top_padding") == 0) {
        sscanf(value, "%i", &top_padding);
    } else if (strcmp(name, "bottom_padding") == 0) {
        sscanf(value, "%i", &bottom_padding);
    } else if (strcmp(name, "normal_border_color") == 0) {
        free(normal_border_color);
        normal_border_color = strdup(value);
        normal_border_color_pxl = get_color(normal_border_color);
    } else if (strcmp(name, "active_border_color") == 0) {
        free(active_border_color);
        active_border_color = strdup(value);
        active_border_color_pxl = get_color(active_border_color);
    } else if (strcmp(name, "inner_border_color") == 0) {
        free(inner_border_color);
        inner_border_color = strdup(value);
        inner_border_color_pxl = get_color(inner_border_color);
    } else if (strcmp(name, "outer_border_color") == 0) {
        free(outer_border_color);
        outer_border_color = strdup(value);
        outer_border_color_pxl = get_color(outer_border_color);
    } else if (strcmp(name, "presel_border_color") == 0) {
        free(presel_border_color);
        presel_border_color = strdup(value);
        presel_border_color_pxl = get_color(presel_border_color);
    } else if (strcmp(name, "locked_border_color") == 0) {
        free(locked_border_color);
        locked_border_color = strdup(value);
        locked_border_color_pxl = get_color(locked_border_color);
    } else if (strcmp(name, "wm_name") == 0) {
        free(wm_name);
        wm_name = strdup(value);
    } else if (strcmp(name, "adaptive_window_border") == 0) {
        if (is_bool(value))
            adaptive_window_border = parse_bool(value);
    } else if (strcmp(name, "adaptive_window_gap") == 0) {
        if (is_bool(value))
            adaptive_window_gap = parse_bool(value);
    }
}

void split_ratio_cmd(char *value)
{
    if (desk == NULL || desk->focus == NULL || value == NULL)
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

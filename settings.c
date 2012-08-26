#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "helpers.h"
#include "utils.h"
#include "luautils.h"
#include "common.h"
#include "settings.h"

void load_settings(void)
{
    lua_State *L = lua_open();
    luaopen_base(L);

    if (luaL_loadfile(L, CONFIG_FILE) == 0) {
        if (lua_pcall(L, 0, 0, 0) == 0)
            apply_settings(L);
        else
            die("error: cannot interpret configuration file\n");
    } else {
        die("error: could not load configuration file\n");
    }

    lua_close(L);
}

void apply_settings(lua_State *L)
{
    normal_border_color = string_expr(L, "normal_border_color", NORMAL_BORDER_COLOR);
    active_border_color = string_expr(L, "active_border_color", ACTIVE_BORDER_COLOR);
    inner_border_color = string_expr(L, "inner_border_color", INNER_BORDER_COLOR);
    outer_border_color = string_expr(L, "outer_border_color", OUTER_BORDER_COLOR);
    presel_border_color = string_expr(L, "presel_border_color", PRESELECT_BORDER_COLOR);
    locked_border_color = string_expr(L, "locked_border_color", LOCKED_BORDER_COLOR);

    normal_border_color_pxl = get_color(normal_border_color);
    active_border_color_pxl = get_color(active_border_color);
    inner_border_color_pxl = get_color(inner_border_color);
    outer_border_color_pxl = get_color(outer_border_color);
    presel_border_color_pxl = get_color(presel_border_color);
    locked_border_color_pxl = get_color(locked_border_color);

    adaptive_window_border = bool_expr(L, "adaptive_window_border", SMART_WINDOW_BORDER);
    adaptive_window_gap = bool_expr(L, "adaptive_window_gap", SMART_WINDOW_GAP);

    inner_border_width = int_expr(L, "inner_border_width", INNER_BORDER_WIDTH);
    main_border_width = int_expr(L, "main_border_width", MAIN_BORDER_WIDTH);
    outer_border_width = int_expr(L, "outer_border_width", OUTER_BORDER_WIDTH);

    border_width = inner_border_width + main_border_width + outer_border_width;
    window_gap = int_expr(L, "window_gap", WINDOW_GAP);
}

void get_setting(lua_State *L, char* rsp)
{
    char *name;

    if (!has_table(L, "get"))
        return;

    name = string_expr(L, "get.name", NULL);
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
    else if (strcmp(name, "adaptive_window_border") == 0)
        sprintf(rsp, "%s\n", BOOLSTR(adaptive_window_border));
    else if (strcmp(name, "adaptive_window_gap") == 0)
        sprintf(rsp, "%s\n", BOOLSTR(adaptive_window_gap));
}

void set_setting(lua_State *L)
{
    char *name, *backup;

    if (!has_table(L, "set"))
        return;

    name = string_expr(L, "set.name", NULL);

    if (name == NULL)
        return;

    if (strcmp(name, "inner_border_width") == 0) {
        inner_border_width = int_expr(L, "set.value", inner_border_width);
        border_width = inner_border_width + main_border_width + outer_border_width;
    } else if (strcmp(name, "main_border_width") == 0) {
        main_border_width = int_expr(L, "set.value", main_border_width);
        border_width = inner_border_width + main_border_width + outer_border_width;
    } else if (strcmp(name, "outer_border_width") == 0) {
        outer_border_width = int_expr(L, "set.value", outer_border_width);
        border_width = inner_border_width + main_border_width + outer_border_width;
    } else if (strcmp(name, "normal_border_color") == 0) {
        backup = strdup(normal_border_color);
        free(normal_border_color);
        normal_border_color = string_expr(L, "set.value", backup);
        normal_border_color_pxl = get_color(normal_border_color);
    } else if (strcmp(name, "active_border_color") == 0) {
        backup = strdup(active_border_color);
        free(active_border_color);
        active_border_color = string_expr(L, "set.value", backup);
        active_border_color_pxl = get_color(active_border_color);
    } else if (strcmp(name, "inner_border_color") == 0) {
        backup = strdup(inner_border_color);
        free(inner_border_color);
        inner_border_color = string_expr(L, "set.value", backup);
        inner_border_color_pxl = get_color(inner_border_color);
    } else if (strcmp(name, "outer_border_color") == 0) {
        backup = strdup(outer_border_color);
        free(outer_border_color);
        outer_border_color = string_expr(L, "set.value", backup);
        outer_border_color_pxl = get_color(outer_border_color);
    } else if (strcmp(name, "outer_border_color") == 0) {
    } else if (strcmp(name, "presel_border_color") == 0) {
        backup = strdup(presel_border_color);
        free(presel_border_color);
        presel_border_color = string_expr(L, "set.value", backup);
        presel_border_color_pxl = get_color(presel_border_color);
    } else if (strcmp(name, "locked_border_color") == 0) {
        backup = strdup(locked_border_color);
        free(locked_border_color);
        locked_border_color = string_expr(L, "set.value", backup);
        locked_border_color_pxl = get_color(locked_border_color);
    } else if (strcmp(name, "adaptive_window_border") == 0) {
        adaptive_window_border = bool_expr(L, "set.value", adaptive_window_border);
    } else if (strcmp(name, "adaptive_window_gap") == 0) {
        adaptive_window_gap = bool_expr(L, "set.value", adaptive_window_gap);
    }

    if (backup != NULL)
        free(backup);
}

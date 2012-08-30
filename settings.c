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

    wm_name = string_expr(L, "wm_name", WM_NAME);

    adaptive_window_border = bool_expr(L, "adaptive_window_border", SMART_WINDOW_BORDER);
    adaptive_window_gap = bool_expr(L, "adaptive_window_gap", SMART_WINDOW_GAP);

    inner_border_width = int_expr(L, "inner_border_width", INNER_BORDER_WIDTH);
    main_border_width = int_expr(L, "main_border_width", MAIN_BORDER_WIDTH);
    outer_border_width = int_expr(L, "outer_border_width", OUTER_BORDER_WIDTH);

    border_width = inner_border_width + main_border_width + outer_border_width;
    window_gap = int_expr(L, "window_gap", WINDOW_GAP);
}

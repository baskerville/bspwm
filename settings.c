#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "helpers.h"
#include "types.h"
#include "bspwm.h"
#include "utils.h"
#include "luautils.h"
#include "common.h"
#include "settings.h"

void load_settings(void)
{
    char path[MAXLEN];
    lua_State *L = lua_open();
    luaopen_base(L);
    /* luaL_openlibs(L); */

    snprintf(path, sizeof(path), "%s/%s/%s", getenv("XDG_CONFIG_HOME"), WM_NAME, CONFIG_FILE);

    if (luaL_loadfile(L, path) == 0) {
        if (lua_pcall(L, 0, 0, 0) == 0)
            apply_settings(L);
        else
            PUTS("error: cannot interpret configuration file");
    } else {
        PUTS("error: could not load configuration file");
    }

    lua_close(L);
}

void run_autostart(void)
{
    char path[MAXLEN];

    snprintf(path, sizeof(path), "%s/%s/%s", getenv("XDG_CONFIG_HOME"), WM_NAME, AUTOSTART_FILE);

    if (fork() != 0)
        return;

    if (dpy != NULL)
        close(xcb_get_file_descriptor(dpy));

    setsid();
    execl(path, path, NULL);
        
    PUTS("error: could not load autostart file");
    exit(EXIT_SUCCESS);
}

void apply_settings(lua_State *L)
{
    string_expr(L, normal_border_color, "normal_border_color", NORMAL_BORDER_COLOR);
    string_expr(L, active_border_color, "active_border_color", ACTIVE_BORDER_COLOR);
    string_expr(L, inner_border_color, "inner_border_color", INNER_BORDER_COLOR);
    string_expr(L, outer_border_color, "outer_border_color", OUTER_BORDER_COLOR);
    string_expr(L, presel_border_color, "presel_border_color", PRESEL_BORDER_COLOR);
    string_expr(L, active_locked_border_color, "active_locked_border_color", ACTIVE_LOCKED_BORDER_COLOR);
    string_expr(L, normal_locked_border_color, "normal_locked_border_color", NORMAL_LOCKED_BORDER_COLOR);
    string_expr(L, urgent_border_color, "urgent_border_color", URGENT_BORDER_COLOR);

    normal_border_color_pxl = get_color(normal_border_color);
    active_border_color_pxl = get_color(active_border_color);
    inner_border_color_pxl = get_color(inner_border_color);
    outer_border_color_pxl = get_color(outer_border_color);
    presel_border_color_pxl = get_color(presel_border_color);
    active_locked_border_color_pxl = get_color(active_locked_border_color);
    normal_locked_border_color_pxl = get_color(normal_locked_border_color);
    urgent_border_color_pxl = get_color(urgent_border_color);

    adaptive_window_border = bool_expr(L, "adaptive_window_border", ADAPTIVE_WINDOW_BORDER);
    string_expr(L, wm_name, "wm_name", WM_NAME);

    inner_border_width = int_expr(L, "inner_border_width", INNER_BORDER_WIDTH);
    main_border_width = int_expr(L, "main_border_width", MAIN_BORDER_WIDTH);
    outer_border_width = int_expr(L, "outer_border_width", OUTER_BORDER_WIDTH);

    border_width = inner_border_width + main_border_width + outer_border_width;

    window_gap = int_expr(L, "window_gap", WINDOW_GAP);
    left_padding = int_expr(L, "left_padding", LEFT_PADDING);
    right_padding = int_expr(L, "right_padding", RIGHT_PADDING);
    top_padding = int_expr(L, "top_padding", TOP_PADDING);
    bottom_padding = int_expr(L, "bottom_padding", BOTTOM_PADDING);
}

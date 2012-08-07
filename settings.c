#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "luautils.h"
#include "settings.h"
#include "common.h"

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
    locked_border_color = string_expr(L, "locked_border_color", LOCKED_BORDER_COLOR);
    urgent_border_color = string_expr(L, "urgent_border_color", URGENT_BORDER_COLOR);
    inner_border_color = string_expr(L, "inner_border_color", INNER_BORDER_COLOR);

    smart_surroundings = bool_expr(L, "smart_surroundings", SMART_SURROUNDINGS);

    outer_border_width = int_expr(L, "outer_border_width", OUTER_BORDER_WIDTH);
    inner_border_width = int_expr(L, "inner_border_width", INNER_BORDER_WIDTH);
    border_width = inner_border_width + outer_border_width;

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
    else if (strcmp(name, "outer_border_width") == 0)
        sprintf(rsp, "%i\n", outer_border_width);
    else if (strcmp(name, "window_gap") == 0)
        sprintf(rsp, "%i\n", window_gap);
    else if (strcmp(name, "normal_border_color") == 0)
        sprintf(rsp, "%s\n", normal_border_color);
    else if (strcmp(name, "active_border_color") == 0)
        sprintf(rsp, "%s\n", active_border_color);
    else if (strcmp(name, "locked_border_color") == 0)
        sprintf(rsp, "%s\n", locked_border_color);
    else if (strcmp(name, "urgent_border_color") == 0)
        sprintf(rsp, "%s\n", urgent_border_color);
    else if (strcmp(name, "inner_border_color") == 0)
        sprintf(rsp, "%s\n", inner_border_color);
    else if (strcmp(name, "smart_surroundings") == 0)
        sprintf(rsp, "%s\n", BOOLSTR(smart_surroundings));
}

void set_setting(lua_State *L)
{
    char *name;

    if (!has_table(L, "set"))
        return;

    name = string_expr(L, "set.name", NULL);

    if (name == NULL)
        return;

    if (strcmp(name, "inner_border_width") == 0) {
        inner_border_width = int_expr(L, "set.value", INNER_BORDER_WIDTH);
    } else if (strcmp(name, "normal_border_color") == 0) {
        free(normal_border_color);
        normal_border_color = string_expr(L, "set.value", NORMAL_BORDER_COLOR);
    } else if (strcmp(name, "inner_border_color") == 0) {
        free(inner_border_color);
        inner_border_color = string_expr(L, "set.value", INNER_BORDER_COLOR);
    }
}

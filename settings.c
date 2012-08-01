#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "luautils.h"
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
    normal_border_color = lua_stringexpr(L, "normal_border_color", NORMAL_BORDER_COLOR);
    split_ratio = lua_doubleexpr(L, "split_ratio", SPLIT_RATIO);
    smart_surroundings = lua_boolexpr(L, "smart_surroundings", SMART_SURROUNDINGS);
    outer_border_width = lua_intexpr(L, "outer_border_width", OUTER_BORDER_WIDTH);
    inner_border_width = lua_intexpr(L, "inner_border_width", INNER_BORDER_WIDTH);
    inner_border_color = lua_stringexpr(L, "inner_border_color", INNER_BORDER_COLOR);
    border_width = inner_border_width + outer_border_width;
    /* printf("split ratio: %f\n", split_ratio); */
    /* printf("outer_border_width: %i\n", outer_border_width); */
    /* printf("inner_border_color: %s\n", inner_border_color); */
    /* printf("normal_border_color: %s\n", normal_border_color); */
    /* printf("default normal_border_color: %s\n", NORMAL_BORDER_COLOR); */
    /* printf("outer_border_color: %s\n", outer_border_color); */
    /* printf("smart_surroundings: %i\n", smart_surroundings); */
}

void get_setting(lua_State *L)
{
    char *name;

    if (!lua_hastable(L, "get"))
        return;

    name = lua_stringexpr(L, "get.name", NULL);
    if (name == NULL)
        return;

    if (strcmp(name, "inner_border_width") == 0)
        printf("%i\n", inner_border_width);
    else if (strcmp(name, "normal_border_color") == 0)
        printf("%s\n", normal_border_color);
    else if (strcmp(name, "inner_border_color") == 0)
        printf("%s\n", inner_border_color);
}

void set_setting(lua_State *L)
{
    char *name;

    if (!lua_hastable(L, "set"))
        return;

    name = lua_stringexpr(L, "set.name", NULL);

    if (name == NULL)
        return;

    if (strcmp(name, "inner_border_width") == 0)
        inner_border_width = lua_intexpr(L, "set.value", INNER_BORDER_WIDTH);
    else if (strcmp(name, "normal_border_color") == 0)
        normal_border_color = lua_stringexpr(L, "set.value", NORMAL_BORDER_COLOR);
    else if (strcmp(name, "inner_border_color") == 0)
        inner_border_color = lua_stringexpr(L, "set.value", INNER_BORDER_COLOR);
}

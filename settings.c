#include <stdio.h>
#include "utils.h"
#include "luautils.h"
#include "settings.h"

void load_settings(void)
{
    lua_State *L = lua_open();
    luaopen_base(L);

    if (luaL_loadfile(L, CONFIG_FILE) == 0) {
        if (lua_pcall(L, 0, 0, 0) == 0) {
            apply_settings(L);
        } else {
            die("error: cannot interpret configuration file\n");
        }
    } else {
        die("error: could not load configuration file\n");
    }

    lua_close(L);
}

void apply_settings(lua_State *L)
{
    normal_border_color = lua_stringexpr(L, "set.normal_border_color", NORMAL_BORDER_COLOR);
    split_ratio = lua_doubleexpr(L, "set.split_ratio", SPLIT_RATIO);
    smart_surroundings = lua_boolexpr(L, "set.smart_surroundings", SMART_SURROUNDINGS);
    outer_border_width = lua_intexpr(L, "set.outer_border_width", OUTER_BORDER_WIDTH);
    inner_border_width = lua_intexpr(L, "set.inner_border_width", INNER_BORDER_WIDTH);
    border_width = inner_border_width + outer_border_width;
    printf("split ratio: %f\n", split_ratio);
    printf("outer_border_width: %s\n", normal_border_color);
    printf("outer_border_width: %i\n", outer_border_width);
    printf("smart_surroundings: %i\n", smart_surroundings);
}

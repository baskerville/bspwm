#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "utils.h"
#include "settings.h"
#include "luautils.h"

void load_settings(void)
{
    lua_State *L = lua_open();
    luaopen_base(L);

    if (luaL_loadfile(L, CONFIG_FILE) == 0) {
        if (lua_pcall(L, 0, 0, 0) == 0) {
            normal_border_color = lua_stringexpr(L, "set.normal_border_color", NORMAL_BORDER_COLOR);
            border_width = lua_intexpr(L, "set.border_width", BORDER_WIDTH);
            split_ratio = lua_doubleexpr(L, "set.split_ratio", SPLIT_RATIO);
            smart_surroundings = lua_boolexpr(L, "set.smart_surroundings", SMART_SURROUNDINGS);
        } else {
            die("error: cannot interpret configuration file\n");
        }
    } else {
        die("error: could not load configuration file\n");
    }

    lua_close(L);
}

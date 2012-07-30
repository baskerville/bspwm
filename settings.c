#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "settings.h"
#include "utils.h"
#include "luautils.h"

void load_settings(void)
{
    lua_State *ls = lua_open();
    luaopen_base(ls);

    if (luaL_loadfile(ls, CONFIG_FILE) == 0) {
        if (lua_pcall(ls, 0, 0, 0) == 0) {
            normal_border_color = lua_stringexpr(ls, "set.normal_border_color", NORMAL_BORDER_COLOR);
        } else {
            die("error: cannot interpret configuration file\n");
        }
    } else {
        die("error: could not load configuration file\n");
    }

    lua_close(ls);
}

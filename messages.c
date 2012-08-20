#include <string.h>
#include "helpers.h"
#include "luautils.h"
#include "settings.h"
#include "messages.h"

void handle_call(lua_State *L)
{
    char *name;
    name = string_expr(L, "run.name", NULL);
    if (name == NULL)
        return;
    if (strcmp(name, "quit") == 0)
        quit();
}

void process_message(char *msg, char *rsp)
{
    lua_State *L = lua_open();
    luaopen_base(L);

    if (luaL_loadstring(L, msg) == 0 && lua_pcall(L, 0, 0, 0) == 0) {
        handle_call(L);
        set_setting(L);
        get_setting(L, rsp);
    }

    lua_close(L);
}

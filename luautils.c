#define _BSD_SOURCE

#include <string.h>
#include "utils.h"
#include "luautils.h"

int eval_expr(lua_State *L, char *expr)
{
    char buf[BUFSIZ];
    sprintf(buf, "return %s", expr);
    return luaL_dostring(L, buf);
}

int has_table(lua_State *L, char *name)
{
    int result = 0;
    eval_expr(L, name);
    result = lua_istable(L, -1);
    lua_pop(L, 1);
    return result;
}

char *string_expr(lua_State *L, char *expr, char* fallback)
{
    char *result;
    if (eval_expr(L, expr) == 0) {
        if (lua_isstring(L, -1))
            result = strdup(lua_tostring(L, -1));
        else if (fallback != NULL)
            result = strdup(fallback);
        else
            result = NULL;
        lua_pop(L, 1);
    }
    return result;
}

double double_expr(lua_State *L, char *expr, double fallback)
{
    double result = fallback;
    if (eval_expr(L, expr) == 0) {
        if (lua_isnumber(L, -1))
            result = (double) lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    return result;
}

int int_expr(lua_State *L, char *expr, int fallback)
{
    int result = fallback;
    if (eval_expr(L, expr) == 0) {
        if (lua_isnumber(L, -1))
            result = (int) lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    return result;
}

bool bool_expr(lua_State *L, char *expr, bool fallback)
{
    bool result = fallback;
    if (eval_expr(L, expr) == 0) {
        if (lua_isboolean(L, -1))
            result = (bool) lua_toboolean(L, -1);
        lua_pop(L, 1);
    }
    return result;
}

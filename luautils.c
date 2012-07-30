#include "utils.h"
#include "luautils.h"

int lua_evalexpr(lua_State* L, char* expr)
{
  char buf[BUFSIZ];
  sprintf(buf, "return %s", expr);
  return luaL_dostring(L, buf);
}

char *lua_stringexpr(lua_State* L, char* expr, char* fallback)
{
  char *result = fallback;
  if (lua_evalexpr(L, expr) == 0) {
    if (lua_isstring(L, -1))
      result = (char *) lua_tostring(L, -1);
    lua_pop(L, 1);
  }
  return result;
}

double lua_doubleexpr(lua_State* L, char* expr, double fallback)
{
  double result = fallback;
  if (lua_evalexpr(L, expr) == 0) {
    if (lua_isnumber(L, -1))
      result = (double) lua_tonumber(L, -1);
    lua_pop(L, 1);
  }
  return result;
}

int lua_intexpr(lua_State* L, char* expr, int fallback)
{
  int result = fallback;
  if (lua_evalexpr(L, expr) == 0) {
    if (lua_isnumber(L, -1))
      result = (int) lua_tonumber(L, -1);
    lua_pop(L, 1);
  }
  return result;
}

bool lua_boolexpr(lua_State* L, char* expr, bool fallback)
{
  bool result = fallback;
  if (lua_evalexpr(L, expr) == 0) {
    if (lua_isboolean(L, -1))
      result = (bool) lua_toboolean(L, -1);
    lua_pop(L, 1);
  }
  return result;
}

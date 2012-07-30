#include "utils.h"
#include "luautils.h"

char *lua_stringexpr(lua_State* L, char* expr, char* def)
{
  char buf[EXPR_BUF_SIZE];
  char *result = def;
  sprintf(buf, "TMP=%s", expr);
  if (!luaL_dostring(L, buf)) {
    lua_getglobal(L, "TMP");
    if (lua_isstring(L, -1)) {
      result = (char *) lua_tostring(L, -1);
    }
    lua_pop(L, 1);
  }
  return result;
}

double lua_doubleexpr(lua_State* L, char* expr, double def)
{
  char buf[EXPR_BUF_SIZE];
  double result = def;
  sprintf(buf, "TMP=%s", expr);
  if (!luaL_dostring(L, buf)) {
    lua_getglobal(L, "TMP");
    if (lua_isnumber(L, -1)) {
      result = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
  }
  return result;
}

int lua_intexpr(lua_State* L, char* expr, int def)
{
  return (int) lua_doubleexpr(L, expr, (double) def);
}

bool lua_boolexpr(lua_State* L, char* expr, bool def)
{
  char buf[EXPR_BUF_SIZE];
  bool result = def;
  sprintf(buf, "TMP=%s", expr);
  if (!luaL_dostring(L, buf)) {
    lua_getglobal(L, "TMP");
    if (lua_isboolean(L, -1)) {
      result = (bool) lua_toboolean(L, -1);
    }
    lua_pop(L, 1);
  }
  return result;
}

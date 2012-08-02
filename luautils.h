#ifndef _LUAUTILS_H
#define _LUAUTILS_H

#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int eval_expr(lua_State*, char*);
int lua_hastable(lua_State*, char*);
char *string_expr(lua_State*, char*, char*);
double double_expr(lua_State*, char*, double);
int int_expr(lua_State*, char*, int);
bool lua_boolexpr(lua_State*, char*, bool);

#endif

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#define EXPR_BUF_SIZE   256

int lua_evalexpr(lua_State*, char*);
char* lua_stringexpr(lua_State*, char*, char*);
double lua_doubleexpr(lua_State*, char*, double);
int lua_intexpr(lua_State*, char*, int);
bool lua_boolexpr(lua_State*, char*, bool);

#ifndef _MESSAGES_H
#define _MESSAGES_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

void handle_call(lua_State*);
void process_message(char*, char*);
void quit(void);

#endif

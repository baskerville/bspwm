#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define CONFIG_FILE         ".bspwmrc"
#define OUTER_BORDER_WIDTH  1
#define INNER_BORDER_WIDTH  2

#define SPLIT_RATIO         0.5
#define SMART_SURROUNDINGS  true

#define NORMAL_BORDER_COLOR  "#333333"
#define ACTIVE_BORDER_COLOR  "#DDDDDD"
#define INNER_BORDER_COLOR   "#111111"

char *normal_border_color;
char *active_border_color;
char *inner_border_color;

int outer_border_width;
int inner_border_width;
int border_width;
bool smart_surroundings;
double split_ratio;

void load_settings(void);
void apply_settings(lua_State*);
void set_setting(lua_State*);
void get_setting(lua_State*);

#endif

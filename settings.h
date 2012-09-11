#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "helpers.h"

#define WM_NAME             "bspwm"
#define CONFIG_FILE         "bspwmrc"
#define AUTOSTART_FILE      "autostart"

#define OUTER_BORDER_WIDTH  2
#define MAIN_BORDER_WIDTH   1
#define INNER_BORDER_WIDTH  2
#define SPLIT_RATIO         0.5

#define WINDOW_GAP          6
#define TOP_PADDING         0
#define BOTTOM_PADDING      0
#define LEFT_PADDING        0
#define RIGHT_PADDING       0

#define ADAPTIVE_WINDOW_GAP     true
#define ADAPTIVE_WINDOW_BORDER  true

#define NORMAL_BORDER_COLOR  "#333333"
#define ACTIVE_BORDER_COLOR  "#DDDDDD"
#define INNER_BORDER_COLOR   "#111111"
#define OUTER_BORDER_COLOR   "#222222"
#define PRESEL_BORDER_COLOR  "#331122"
#define LOCKED_BORDER_COLOR  "#331122"

char normal_border_color[MAXLEN];
char active_border_color[MAXLEN];
char inner_border_color[MAXLEN];
char outer_border_color[MAXLEN];
char presel_border_color[MAXLEN];
char locked_border_color[MAXLEN];

char wm_name[MAXLEN];

uint32_t normal_border_color_pxl;
uint32_t active_border_color_pxl;
uint32_t inner_border_color_pxl;
uint32_t outer_border_color_pxl;
uint32_t presel_border_color_pxl;
uint32_t locked_border_color_pxl;

int inner_border_width;
int main_border_width;
int outer_border_width;

int border_width;

int window_gap;
int top_padding;
int bottom_padding;
int left_padding;
int right_padding;

bool adaptive_window_border;
bool adaptive_window_gap;

void load_settings(void);
void run_autostart(void);
void apply_settings(lua_State *);

#endif

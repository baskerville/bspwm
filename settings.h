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

#define ADAPTIVE_WINDOW_BORDER  true

#define NORMAL_BORDER_COLOR  "gray22"
#define ACTIVE_BORDER_COLOR  "lightslategray"
#define INNER_BORDER_COLOR   "darkslategray"
#define OUTER_BORDER_COLOR   "darkslategray"
#define PRESEL_BORDER_COLOR  "dimgray"
#define LOCKED_BORDER_COLOR  "slateblue"

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

unsigned int inner_border_width;
unsigned int main_border_width;
unsigned int outer_border_width;

unsigned int border_width;

int window_gap;

int top_padding;
int bottom_padding;
int left_padding;
int right_padding;

bool adaptive_window_border;

void load_settings(void);
void run_autostart(void);
void apply_settings(lua_State *);

#endif

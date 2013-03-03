#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "types.h"

#define WM_NAME             "bspwm"
#define AUTOSTART_FILE      "autostart"
#define CONFIG_HOME_ENV     "XDG_CONFIG_HOME"

#define FOCUSED_BORDER_COLOR        "#7E7F89"
#define ACTIVE_BORDER_COLOR         "#545350"
#define NORMAL_BORDER_COLOR         "#3F3E3B"
#define PRESEL_BORDER_COLOR         "#E8E8F4"
#define FOCUSED_LOCKED_BORDER_COLOR "#C7B579"
#define ACTIVE_LOCKED_BORDER_COLOR  "#545350"
#define NORMAL_LOCKED_BORDER_COLOR  "#3F3E3B"
#define URGENT_BORDER_COLOR         "#EFA29A"

#define BORDER_WIDTH   1
#define WINDOW_GAP     6
#define SPLIT_RATIO    0.5

#define BORDERLESS_MONOCLE     false
#define GAPLESS_MONOCLE        false
#define FOCUS_FOLLOWS_POINTER  false
#define ADAPTATIVE_RAISE       false
#define APPLY_SHADOW_PROPERTY  false

char focused_border_color[MAXLEN];
char active_border_color[MAXLEN];
char normal_border_color[MAXLEN];
char presel_border_color[MAXLEN];
char focused_locked_border_color[MAXLEN];
char active_locked_border_color[MAXLEN];
char normal_locked_border_color[MAXLEN];
char urgent_border_color[MAXLEN];

uint32_t focused_border_color_pxl;
uint32_t active_border_color_pxl;
uint32_t normal_border_color_pxl;
uint32_t presel_border_color_pxl;
uint32_t focused_locked_border_color_pxl;
uint32_t active_locked_border_color_pxl;
uint32_t normal_locked_border_color_pxl;
uint32_t urgent_border_color_pxl;

unsigned int border_width;
int window_gap;

bool borderless_monocle;
bool gapless_monocle;
bool focus_follows_pointer;
bool adaptative_raise;
bool apply_shadow_property;

char wm_name[MAXLEN];

void load_settings(void);
void run_autostart(void);

#endif

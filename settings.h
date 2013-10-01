#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "types.h"

#define WM_NAME             "bspwm"
#define CONFIG_NAME         WM_NAME "rc"
#define CONFIG_HOME_ENV     "XDG_CONFIG_HOME"

#define FOCUSED_BORDER_COLOR        "#7E7F89"
#define ACTIVE_BORDER_COLOR         "#545350"
#define NORMAL_BORDER_COLOR         "#3F3E3B"
#define PRESEL_BORDER_COLOR         "#E8E8F4"
#define FOCUSED_LOCKED_BORDER_COLOR "#C7B579"
#define ACTIVE_LOCKED_BORDER_COLOR  "#545350"
#define NORMAL_LOCKED_BORDER_COLOR  "#3F3E3B"
#define URGENT_BORDER_COLOR         "#EFA29A"

#define SPLIT_RATIO    0.5
#define GROWTH_FACTOR  1.1

#define HISTORY_AWARE_FOCUS      false
#define BORDERLESS_MONOCLE       false
#define GAPLESS_MONOCLE          false
#define FOCUS_FOLLOWS_POINTER    false
#define POINTER_FOLLOWS_MONITOR  false
#define AUTO_ALTERNATE           false
#define AUTO_CANCEL              false
#define APPLY_FLOATING_ATOM      false
#define HONOR_EWMH_FOCUS         false

char focused_border_color[MAXLEN];
char active_border_color[MAXLEN];
char normal_border_color[MAXLEN];
char presel_border_color[MAXLEN];
char focused_locked_border_color[MAXLEN];
char active_locked_border_color[MAXLEN];
char normal_locked_border_color[MAXLEN];
char urgent_border_color[MAXLEN];

double split_ratio;
double growth_factor;

bool borderless_monocle;
bool gapless_monocle;
bool focus_follows_pointer;
bool pointer_follows_monitor;
bool apply_floating_atom;
bool auto_alternate;
bool auto_cancel;
bool history_aware_focus;
bool honor_ewmh_focus;

void load_settings(void);
void run_config(void);

#endif

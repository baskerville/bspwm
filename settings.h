#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "types.h"

#define WM_NAME             "bspwm"
#define AUTOSTART_FILE      "autostart"
#define BUTTON_MODIFIER     XCB_MOD_MASK_4

#define FOCUSED_BORDER_COLOR        "#7D7F8A"
#define ACTIVE_BORDER_COLOR         "#7D7F8A"
#define NORMAL_BORDER_COLOR         "#3F3E3B"
#define INNER_BORDER_COLOR          "#32312E"
#define OUTER_BORDER_COLOR          "#32312E"
#define PRESEL_BORDER_COLOR         "#97AE71"
#define FOCUSED_LOCKED_BORDER_COLOR "#B6A56A"
#define ACTIVE_LOCKED_BORDER_COLOR  "#B6A56A"
#define NORMAL_LOCKED_BORDER_COLOR  "#8D7E45"
#define URGENT_BORDER_COLOR         "#DE928B"

#define INNER_BORDER_WIDTH  3
#define MAIN_BORDER_WIDTH   1
#define OUTER_BORDER_WIDTH  3

#define WINDOW_GAP          6
#define SPLIT_RATIO         0.5

#define BORDERLESS_MONOCLE   false
#define GAPLESS_MONOCLE      false
#define FOCUS_FOLLOWS_MOUSE  false
#define ADAPTATIVE_RAISE     false

char focused_border_color[MAXLEN];
char active_border_color[MAXLEN];
char normal_border_color[MAXLEN];
char inner_border_color[MAXLEN];
char outer_border_color[MAXLEN];
char presel_border_color[MAXLEN];
char focused_locked_border_color[MAXLEN];
char active_locked_border_color[MAXLEN];
char normal_locked_border_color[MAXLEN];
char urgent_border_color[MAXLEN];

uint32_t focused_border_color_pxl;
uint32_t active_border_color_pxl;
uint32_t normal_border_color_pxl;
uint32_t inner_border_color_pxl;
uint32_t outer_border_color_pxl;
uint32_t presel_border_color_pxl;
uint32_t focused_locked_border_color_pxl;
uint32_t active_locked_border_color_pxl;
uint32_t normal_locked_border_color_pxl;
uint32_t urgent_border_color_pxl;

unsigned int inner_border_width;
unsigned int main_border_width;
unsigned int outer_border_width;
unsigned int border_width;

int window_gap;

bool borderless_monocle;
bool gapless_monocle;
bool focus_follows_mouse;
bool adaptative_raise;

char wm_name[MAXLEN];
unsigned int button_modifier;

void load_settings(void);
void run_autostart(void);

#endif

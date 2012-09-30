#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "types.h"

#define WM_NAME             "bspwm"
#define CONFIG_FILE         "bspwmrc"
#define AUTOSTART_FILE      "autostart"

#define ACTIVE_BORDER_COLOR         "#7D7F8A"
#define NORMAL_BORDER_COLOR         "#3F3E3B"
#define INNER_BORDER_COLOR          "#32312E"
#define OUTER_BORDER_COLOR          "#32312E"
#define PRESEL_BORDER_COLOR         "#97AE71"
#define ACTIVE_LOCKED_BORDER_COLOR  "#B6A56A"
#define NORMAL_LOCKED_BORDER_COLOR  "#8D7E45"
#define URGENT_BORDER_COLOR         "#DE928B"

#define INNER_BORDER_WIDTH  3
#define MAIN_BORDER_WIDTH   1
#define OUTER_BORDER_WIDTH  3
#define SPLIT_RATIO         0.5

#define WINDOW_GAP          6
#define TOP_PADDING         0
#define BOTTOM_PADDING      0
#define LEFT_PADDING        0
#define RIGHT_PADDING       0

#define BORDERLESS_MONOCLE  false

char active_border_color[MAXLEN];
char normal_border_color[MAXLEN];
char inner_border_color[MAXLEN];
char outer_border_color[MAXLEN];
char presel_border_color[MAXLEN];
char active_locked_border_color[MAXLEN];
char normal_locked_border_color[MAXLEN];
char urgent_border_color[MAXLEN];

uint32_t active_border_color_pxl;
uint32_t normal_border_color_pxl;
uint32_t inner_border_color_pxl;
uint32_t outer_border_color_pxl;
uint32_t presel_border_color_pxl;
uint32_t active_locked_border_color_pxl;
uint32_t normal_locked_border_color_pxl;
uint32_t urgent_border_color_pxl;

unsigned int inner_border_width;
unsigned int main_border_width;
unsigned int outer_border_width;
unsigned int border_width;

int window_gap;
int top_padding;
int bottom_padding;
int left_padding;
int right_padding;

bool borderless_monocle;

char wm_name[MAXLEN];

void load_settings(void);
void run_autostart(void);

#endif

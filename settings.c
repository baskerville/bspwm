#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "bspwm.h"
#include "helpers.h"
#include "common.h"
#include "settings.h"

void run_autostart(void)
{
    char path[MAXLEN];

    snprintf(path, sizeof(path), "%s/%s/%s", getenv("XDG_CONFIG_HOME"), WM_NAME, AUTOSTART_FILE);

    if (fork() != 0)
        return;

    if (dpy != NULL)
        close(xcb_get_file_descriptor(dpy));

    setsid();
    execl(path, path, NULL);

    err("could not run autostart file\n");
}

void load_settings(void)
{
    strncpy(normal_border_color, NORMAL_BORDER_COLOR, sizeof(normal_border_color));
    strncpy(focused_border_color, FOCUSED_BORDER_COLOR, sizeof(focused_border_color));
    strncpy(active_border_color, ACTIVE_BORDER_COLOR, sizeof(active_border_color));
    strncpy(inner_border_color, INNER_BORDER_COLOR, sizeof(inner_border_color));
    strncpy(outer_border_color, OUTER_BORDER_COLOR, sizeof(outer_border_color));
    strncpy(presel_border_color, PRESEL_BORDER_COLOR, sizeof(presel_border_color));
    strncpy(focused_locked_border_color, FOCUSED_LOCKED_BORDER_COLOR, sizeof(focused_locked_border_color));
    strncpy(active_locked_border_color, ACTIVE_LOCKED_BORDER_COLOR, sizeof(active_locked_border_color));
    strncpy(normal_locked_border_color, NORMAL_LOCKED_BORDER_COLOR, sizeof(normal_locked_border_color));
    strncpy(urgent_border_color, URGENT_BORDER_COLOR, sizeof(urgent_border_color));

    normal_border_color_pxl = get_color(normal_border_color);
    focused_border_color_pxl = get_color(active_border_color);
    active_border_color_pxl = get_color(active_border_color);
    inner_border_color_pxl = get_color(inner_border_color);
    outer_border_color_pxl = get_color(outer_border_color);
    presel_border_color_pxl = get_color(presel_border_color);
    focused_locked_border_color_pxl = get_color(active_locked_border_color);
    active_locked_border_color_pxl = get_color(active_locked_border_color);
    normal_locked_border_color_pxl = get_color(normal_locked_border_color);
    urgent_border_color_pxl = get_color(urgent_border_color);

    strncpy(wm_name, WM_NAME, sizeof(wm_name));
    button_modifier = BUTTON_MODIFIER;

    inner_border_width = INNER_BORDER_WIDTH;
    main_border_width = MAIN_BORDER_WIDTH;
    outer_border_width = OUTER_BORDER_WIDTH;

    border_width = inner_border_width + main_border_width + outer_border_width;

    window_gap = WINDOW_GAP;
    left_padding = LEFT_PADDING;
    right_padding = RIGHT_PADDING;
    top_padding = TOP_PADDING;
    bottom_padding = BOTTOM_PADDING;

    borderless_monocle = BORDERLESS_MONOCLE;
    focus_follows_mouse = FOCUS_FOLLOWS_MOUSE;
}

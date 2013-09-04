#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "bspwm.h"
#include "helpers.h"
#include "common.h"
#include "settings.h"

void run_config(void)
{
    if (fork() == 0) {
        if (dpy != NULL)
            close(xcb_get_file_descriptor(dpy));
        if (fork() == 0) {
            setsid();
            execl(config_path, config_path, NULL);
            err("Couldn't execute the configuration file.\n");
        }
        exit(EXIT_SUCCESS);
    }

    wait(NULL);
}

void load_settings(void)
{
    strncpy(normal_border_color, NORMAL_BORDER_COLOR, sizeof(normal_border_color));
    strncpy(focused_border_color, FOCUSED_BORDER_COLOR, sizeof(focused_border_color));
    strncpy(active_border_color, ACTIVE_BORDER_COLOR, sizeof(active_border_color));
    strncpy(presel_border_color, PRESEL_BORDER_COLOR, sizeof(presel_border_color));
    strncpy(focused_locked_border_color, FOCUSED_LOCKED_BORDER_COLOR, sizeof(focused_locked_border_color));
    strncpy(active_locked_border_color, ACTIVE_LOCKED_BORDER_COLOR, sizeof(active_locked_border_color));
    strncpy(normal_locked_border_color, NORMAL_LOCKED_BORDER_COLOR, sizeof(normal_locked_border_color));
    strncpy(urgent_border_color, URGENT_BORDER_COLOR, sizeof(urgent_border_color));

    strncpy(wm_name, WM_NAME, sizeof(wm_name));

    border_width = BORDER_WIDTH;
    window_gap = WINDOW_GAP;
    split_ratio = SPLIT_RATIO;

    borderless_monocle = BORDERLESS_MONOCLE;
    gapless_monocle = GAPLESS_MONOCLE;
    focus_follows_pointer = FOCUS_FOLLOWS_POINTER;
    pointer_follows_monitor = POINTER_FOLLOWS_MONITOR;
    adaptative_raise = ADAPTATIVE_RAISE;
    apply_shadow_property = APPLY_SHADOW_PROPERTY;
    auto_alternate = AUTO_ALTERNATE;
    auto_cancel = AUTO_CANCEL;
    history_aware_focus = HISTORY_AWARE_FOCUS;
}

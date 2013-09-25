#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "bspwm.h"
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
    snprintf(normal_border_color, sizeof(normal_border_color), "%s", NORMAL_BORDER_COLOR);
    snprintf(focused_border_color, sizeof(focused_border_color), "%s", FOCUSED_BORDER_COLOR);
    snprintf(active_border_color, sizeof(active_border_color), "%s", ACTIVE_BORDER_COLOR);
    snprintf(presel_border_color, sizeof(presel_border_color), "%s", PRESEL_BORDER_COLOR);
    snprintf(focused_locked_border_color, sizeof(focused_locked_border_color), "%s", FOCUSED_LOCKED_BORDER_COLOR);
    snprintf(active_locked_border_color, sizeof(active_locked_border_color), "%s", ACTIVE_LOCKED_BORDER_COLOR);
    snprintf(normal_locked_border_color, sizeof(normal_locked_border_color), "%s", NORMAL_LOCKED_BORDER_COLOR);
    snprintf(urgent_border_color, sizeof(urgent_border_color), "%s", URGENT_BORDER_COLOR);

    border_width = BORDER_WIDTH;
    split_ratio = SPLIT_RATIO;
    growth_factor = GROWTH_FACTOR;

    borderless_monocle = BORDERLESS_MONOCLE;
    gapless_monocle = GAPLESS_MONOCLE;
    focus_follows_pointer = FOCUS_FOLLOWS_POINTER;
    pointer_follows_monitor = POINTER_FOLLOWS_MONITOR;
    apply_floating_atom = APPLY_FLOATING_ATOM;
    auto_alternate = AUTO_ALTERNATE;
    auto_cancel = AUTO_CANCEL;
    history_aware_focus = HISTORY_AWARE_FOCUS;
    honor_ewmh_focus = HONOR_EWMH_FOCUS;
}

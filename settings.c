/* Copyright (c) 2012, Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "bspwm.h"
#include "settings.h"

void run_config(void)
{
	if (fork() == 0) {
		if (dpy != NULL) {
			close(xcb_get_file_descriptor(dpy));
		}
		setsid();
		execl(config_path, config_path, NULL);
		err("Couldn't execute the configuration file.\n");
	}
}

void load_settings(void)
{
	snprintf(external_rules_command, sizeof(external_rules_command), "%s", EXTERNAL_RULES_COMMAND);
	snprintf(status_prefix, sizeof(status_prefix), "%s", STATUS_PREFIX);

	snprintf(normal_border_color, sizeof(normal_border_color), "%s", NORMAL_BORDER_COLOR);
	snprintf(active_border_color, sizeof(active_border_color), "%s", ACTIVE_BORDER_COLOR);
	snprintf(focused_border_color, sizeof(focused_border_color), "%s", FOCUSED_BORDER_COLOR);
	snprintf(presel_feedback_color, sizeof(presel_feedback_color), "%s", PRESEL_FEEDBACK_COLOR);

	window_gap = WINDOW_GAP;
	border_width = BORDER_WIDTH;
	split_ratio = SPLIT_RATIO;
	initial_polarity = FIRST_CHILD;

	borderless_monocle = BORDERLESS_MONOCLE;
	gapless_monocle = GAPLESS_MONOCLE;
	single_monocle = SINGLE_MONOCLE;
	focus_follows_pointer = FOCUS_FOLLOWS_POINTER;
	pointer_follows_focus = POINTER_FOLLOWS_FOCUS;
	pointer_follows_monitor = POINTER_FOLLOWS_MONITOR;
	history_aware_focus = HISTORY_AWARE_FOCUS;
	ignore_ewmh_focus = IGNORE_EWMH_FOCUS;
	center_pseudo_tiled = CENTER_PSEUDO_TILED;
	remove_disabled_monitors = REMOVE_DISABLED_MONITORS;
	remove_unplugged_monitors = REMOVE_UNPLUGGED_MONITORS;
	merge_overlapping_monitors = MERGE_OVERLAPPING_MONITORS;
}

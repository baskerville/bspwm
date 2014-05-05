/* Copyright (c) 2012-2014, Bastien Dejean
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
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

#include <unistd.h>
#include "bspwm.h"
#include "settings.h"

void run_config(void)
{
	if (fork() == 0) {
		if (dpy != NULL)
			close(xcb_get_file_descriptor(dpy));
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
	snprintf(focused_border_color, sizeof(focused_border_color), "%s", FOCUSED_BORDER_COLOR);
	snprintf(active_border_color, sizeof(active_border_color), "%s", ACTIVE_BORDER_COLOR);
	snprintf(presel_border_color, sizeof(presel_border_color), "%s", PRESEL_BORDER_COLOR);
	snprintf(focused_locked_border_color, sizeof(focused_locked_border_color), "%s", FOCUSED_LOCKED_BORDER_COLOR);
	snprintf(active_locked_border_color, sizeof(active_locked_border_color), "%s", ACTIVE_LOCKED_BORDER_COLOR);
	snprintf(normal_locked_border_color, sizeof(normal_locked_border_color), "%s", NORMAL_LOCKED_BORDER_COLOR);
	snprintf(focused_sticky_border_color, sizeof(focused_sticky_border_color), "%s", FOCUSED_STICKY_BORDER_COLOR);
	snprintf(active_sticky_border_color, sizeof(active_sticky_border_color), "%s", ACTIVE_STICKY_BORDER_COLOR);
	snprintf(normal_sticky_border_color, sizeof(normal_sticky_border_color), "%s", NORMAL_STICKY_BORDER_COLOR);
	snprintf(focused_private_border_color, sizeof(focused_private_border_color), "%s", FOCUSED_PRIVATE_BORDER_COLOR);
	snprintf(active_private_border_color, sizeof(active_private_border_color), "%s", ACTIVE_PRIVATE_BORDER_COLOR);
	snprintf(normal_private_border_color, sizeof(normal_private_border_color), "%s", NORMAL_PRIVATE_BORDER_COLOR);
	snprintf(urgent_border_color, sizeof(urgent_border_color), "%s", URGENT_BORDER_COLOR);

	split_ratio = SPLIT_RATIO;

	borderless_monocle = BORDERLESS_MONOCLE;
	gapless_monocle = GAPLESS_MONOCLE;
	focus_follows_pointer = FOCUS_FOLLOWS_POINTER;
	pointer_follows_monitor = POINTER_FOLLOWS_MONITOR;
	apply_floating_atom = APPLY_FLOATING_ATOM;
	auto_alternate = AUTO_ALTERNATE;
	auto_cancel = AUTO_CANCEL;
	history_aware_focus = HISTORY_AWARE_FOCUS;
	ignore_ewmh_focus = IGNORE_EWMH_FOCUS;
	remove_disabled_monitors = REMOVE_DISABLED_MONITORS;
	remove_unplugged_monitors = REMOVE_UNPLUGGED_MONITORS;
	merge_overlapping_monitors = MERGE_OVERLAPPING_MONITORS;
}

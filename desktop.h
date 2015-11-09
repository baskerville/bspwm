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

#ifndef BSPWM_DESKTOP_H
#define BSPWM_DESKTOP_H

#define DEFAULT_DESK_NAME    "Desktop"

void focus_desktop(monitor_t *m, desktop_t *d);
desktop_t *closest_desktop(monitor_t *m, desktop_t *d, cycle_dir_t dir, desktop_select_t sel);
void change_layout(monitor_t *m, desktop_t *d, layout_t l);
void transfer_desktop(monitor_t *ms, monitor_t *md, desktop_t *d);
desktop_t *make_desktop(const char *name);
void rename_desktop(monitor_t *m, desktop_t *d, const char *name);
void initialize_desktop(desktop_t *d);
void insert_desktop(monitor_t *m, desktop_t *d);
void add_desktop(monitor_t *m, desktop_t *d);
void empty_desktop(desktop_t *d);
void unlink_desktop(monitor_t *m, desktop_t *d);
void remove_desktop(monitor_t *m, desktop_t *d);
void merge_desktops(monitor_t *ms, desktop_t *ds, monitor_t *md, desktop_t *dd);
void swap_desktops(monitor_t *m1, desktop_t *d1, monitor_t *m2, desktop_t *d2);
void show_desktop(desktop_t *d);
void hide_desktop(desktop_t *d);
bool is_urgent(desktop_t *d);

#endif

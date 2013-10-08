/* * Copyright (c) 2012-2013 Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BSPWM_MONITOR_H
#define BSPWM_MONITOR_H

#define DEFAULT_MON_NAME     "MONITOR"

monitor_t *make_monitor(xcb_rectangle_t rect);
monitor_t *find_monitor(char *name);
monitor_t *get_monitor_by_id(xcb_randr_output_t id);
void fit_monitor(monitor_t *m, client_t *c);
void update_root(monitor_t *m);
void focus_monitor(monitor_t *m);
monitor_t *add_monitor(xcb_rectangle_t rect);
void remove_monitor(monitor_t *m);
void merge_monitors(monitor_t *ms, monitor_t *md);
void swap_monitors(monitor_t *m1, monitor_t *m2);
monitor_t *closest_monitor(monitor_t *m, cycle_dir_t dir, desktop_select_t sel);
monitor_t *nearest_monitor(monitor_t *m, direction_t dir, desktop_select_t sel);
bool import_monitors(void);

#endif

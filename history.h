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

#ifndef BSPWM_HISTORY_H
#define BSPWM_HISTORY_H

#include "types.h"

history_t *make_history(monitor_t *m, desktop_t *d, node_t *n);
void history_add(monitor_t *m, desktop_t *d, node_t *n);
void history_transfer_node(monitor_t *m, desktop_t *d, node_t *n);
void history_transfer_desktop(monitor_t *m, desktop_t *d);
void history_swap_nodes(monitor_t *m1, desktop_t *d1, node_t *n1, monitor_t *m2, desktop_t *d2, node_t *n2);
void history_swap_desktops(monitor_t *m1, desktop_t *d1, monitor_t *m2, desktop_t *d2);
void history_remove(desktop_t *d, node_t *n);
void empty_history(void);
node_t *history_get_node(desktop_t *d, node_t *n);
desktop_t *history_get_desktop(monitor_t *m, desktop_t *d);
monitor_t *history_get_monitor(monitor_t *m);
bool history_find_node(history_dir_t hdi, coordinates_t *ref, coordinates_t *dst, client_select_t sel);
bool history_find_desktop(history_dir_t hdi, coordinates_t *ref, coordinates_t *dst, desktop_select_t sel);
bool history_find_monitor(history_dir_t hdi, coordinates_t *ref, coordinates_t *dst, desktop_select_t sel);
int history_rank(desktop_t *d, node_t *n);

#endif

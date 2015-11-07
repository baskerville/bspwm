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

#ifndef BSPWM_SUBSCRIBE_H
#define BSPWM_SUBSCRIBE_H

typedef enum {
	SBSC_MASK_REPORT = 1 << 0,
	SBSC_MASK_MONITOR_ADD = 1 << 1,
	SBSC_MASK_MONITOR_RENAME = 1 << 2,
	SBSC_MASK_MONITOR_REMOVE = 1 << 3,
	SBSC_MASK_MONITOR_FOCUS = 1 << 4,
	SBSC_MASK_MONITOR_RESIZE = 1 << 5,
	SBSC_MASK_DESKTOP_ADD = 1 << 6,
	SBSC_MASK_DESKTOP_RENAME = 1 << 7,
	SBSC_MASK_DESKTOP_REMOVE = 1 << 8,
	SBSC_MASK_DESKTOP_SWAP = 1 << 9,
	SBSC_MASK_DESKTOP_TRANSFER = 1 << 10,
	SBSC_MASK_DESKTOP_FOCUS = 1 << 11,
	SBSC_MASK_DESKTOP_LAYOUT = 1 << 12,
	SBSC_MASK_WINDOW_MANAGE = 1 << 13,
	SBSC_MASK_WINDOW_UNMANAGE = 1 << 14,
	SBSC_MASK_WINDOW_SWAP = 1 << 15,
	SBSC_MASK_WINDOW_TRANSFER = 1 << 16,
	SBSC_MASK_WINDOW_FOCUS = 1 << 17,
	SBSC_MASK_WINDOW_ACTIVATE = 1 << 18,
	SBSC_MASK_WINDOW_GEOMETRY = 1 << 19,
	SBSC_MASK_WINDOW_STATE = 1 << 20,
	SBSC_MASK_WINDOW_FLAG = 1 << 21,
	SBSC_MASK_WINDOW_LAYER = 1 << 22,
	SBSC_MASK_MONITOR = (1 << 6) - (1 << 1),
	SBSC_MASK_DESKTOP = (1 << 13) - (1 << 6),
	SBSC_MASK_WINDOW = (1 << 23) - (1 << 13),
	SBSC_MASK_ALL = (1 << 23) - 1
} subscriber_mask_t;

subscriber_list_t *make_subscriber_list(FILE *stream, int field);
void remove_subscriber(subscriber_list_t *sb);
void add_subscriber(FILE *stream, int field);
int print_report(FILE *stream);
void put_status(subscriber_mask_t mask, ...);

#endif

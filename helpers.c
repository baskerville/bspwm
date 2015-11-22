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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "bspwm.h"

void warn(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

__attribute__((noreturn))
void err(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

char *read_string(const char *file_path, size_t *tlen)
{
	if (file_path == NULL) {
		return NULL;
	}

	int fd = open(file_path, O_RDONLY);

	if (fd == -1) {
		perror("Read file: open");
		return NULL;
	}

	char buf[BUFSIZ], *content;
	size_t len = sizeof(buf);

	if ((content = malloc(len * sizeof(char))) == NULL) {
		perror("Read file: malloc");
		return NULL;
	}

	int nb;
	*tlen = 0;

	while (true) {
		nb = read(fd, buf, sizeof(buf));
		if (nb < 0) {
			perror("Restore tree: read");
			free(content);
			return NULL;
		} else if (nb == 0) {
			break;
		} else {
			*tlen += nb;
			if (*tlen > len) {
				len *= 2;
				char *rcontent = realloc(content, len * sizeof(char));
				if (rcontent == NULL) {
					perror("Read file: realloc");
					free(content);
					return NULL;
				} else {
					content = rcontent;
				}
			}
			strncpy(content + (*tlen - nb), buf, nb);
		}
	}

	return content;
}

bool get_color(char *col, xcb_window_t win, uint32_t *pxl)
{
	xcb_colormap_t map = screen->default_colormap;
	xcb_get_window_attributes_reply_t *reply = xcb_get_window_attributes_reply(dpy, xcb_get_window_attributes(dpy, win), NULL);
	if (reply != NULL)
		map = reply->colormap;
	free(reply);

	if (col[0] == '#') {
		unsigned int red, green, blue;
		if (sscanf(col + 1, "%02x%02x%02x", &red, &green, &blue) == 3) {
			/* 2**16 - 1 == 0xffff and 0x101 * 0xij == 0xijij */
			red *= 0x101;
			green *= 0x101;
			blue *= 0x101;
			xcb_alloc_color_reply_t *reply = xcb_alloc_color_reply(dpy, xcb_alloc_color(dpy, map, red, green, blue), NULL);
			if (reply != NULL) {
				*pxl = reply->pixel;
				free(reply);
				return true;
			}
		}
	} else {
		xcb_alloc_named_color_reply_t *reply = xcb_alloc_named_color_reply(dpy, xcb_alloc_named_color(dpy, map, strlen(col), col), NULL);
		if (reply != NULL) {
			*pxl = reply->pixel;
			free(reply);
			return true;
		}
	}
	*pxl = 0;
	return false;
}

double distance(xcb_point_t a, xcb_point_t b)
{
	return hypot(a.x - b.x, a.y - b.y);
}

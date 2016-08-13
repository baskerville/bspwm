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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
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

	char buf[BUFSIZ], *content = NULL;
	size_t len = sizeof(buf);

	if ((content = calloc(len, sizeof(char))) == NULL) {
		perror("Read file: calloc");
		goto end;
	}

	int nb;
	*tlen = 0;

	while (true) {
		nb = read(fd, buf, sizeof(buf));
		if (nb < 0) {
			perror("Restore tree: read");
			free(content);
			content = NULL;
			goto end;
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
					content = NULL;
					goto end;
				} else {
					content = rcontent;
				}
			}
			strncpy(content + (*tlen - nb), buf, nb);
		}
	}

end:
	close(fd);
	return content;
}

char *copy_string(char *str, size_t len)
{
	char *cpy = calloc(1, ((len+1) * sizeof(char)));
	if (cpy == NULL) {
		perror("Copy string: calloc");
		return NULL;
	}
	strncpy(cpy, str, len);
	cpy[len] = '\0';
	return cpy;
}

/* Adapted from i3wm */
uint32_t get_color_pixel(const char *color)
{
	unsigned int red, green, blue;
	if (sscanf(color + 1, "%02x%02x%02x", &red, &green, &blue) == 3) {
		/* We set the first 8 bits high to have 100% opacity in case of a 32 bit
		 * color depth visual. */
		return (0xFF << 24) | (red << 16 | green << 8 | blue);
	} else {
		return screen->black_pixel;
	}
}

bool is_hex_color(const char *color)
{
	if (color[0] != '#' || strlen(color) != 7) {
		return false;
	}
	for (int i = 1; i < 7; i++) {
		if (!isxdigit(color[i])) {
			return false;
		}
	}
	return true;
}

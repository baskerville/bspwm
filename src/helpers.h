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

#ifndef BSPWM_HELPERS_H
#define BSPWM_HELPERS_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <float.h>

#define LENGTH(x)         (sizeof(x) / sizeof(*x))
#define MAX(A, B)         ((A) > (B) ? (A) : (B))
#define MIN(A, B)         ((A) < (B) ? (A) : (B))

/* Branch prediction hints (Tunney-style) */
#define likely(x)         __builtin_expect(!!(x), 1)
#define unlikely(x)       __builtin_expect(!!(x), 0)

#define IS_TILED(c)       (c->state == STATE_TILED || c->state == STATE_PSEUDO_TILED)
#define IS_FLOATING(c)    (c->state == STATE_FLOATING)
#define IS_FULLSCREEN(c)  (c->state == STATE_FULLSCREEN)
#define IS_RECEPTACLE(n)  (is_leaf(n) && n->client == NULL)

#define BOOL_STR(A)       ((A) ? "true" : "false")
#define ON_OFF_STR(A)     ((A) ? "on" : "off")
#define LAYOUT_STR(A)     ((A) == LAYOUT_TILED ? "tiled" : "monocle")
#define LAYOUT_CHR(A)     ((A) == LAYOUT_TILED ? 'T' : 'M')
#define CHILD_POL_STR(A)  ((A) == FIRST_CHILD ? "first_child" : "second_child")
#define AUTO_SCM_STR(A)   ((A) == SCHEME_LONGEST_SIDE ? "longest_side" : ((A) == SCHEME_ALTERNATE ? "alternate" : "spiral"))
#define TIGHTNESS_STR(A)  ((A) == TIGHTNESS_HIGH ? "high" : "low")
#define SPLIT_TYPE_STR(A) ((A) == TYPE_HORIZONTAL ? "horizontal" : "vertical")
#define SPLIT_MODE_STR(A) ((A) == MODE_AUTOMATIC ? "automatic" : "manual")
#define SPLIT_DIR_STR(A)  ((A) == DIR_NORTH ? "north" : ((A) == DIR_WEST ? "west" : ((A) == DIR_SOUTH ? "south" : "east")))
#define STATE_STR(A)      ((A) == STATE_TILED ? "tiled" : ((A) == STATE_FLOATING ? "floating" : ((A) == STATE_FULLSCREEN ? "fullscreen" : "pseudo_tiled")))
#define STATE_CHR(A)      ((A) == STATE_TILED ? 'T' : ((A) == STATE_FLOATING ? 'F' : ((A) == STATE_FULLSCREEN ? '=' : 'P')))
#define LAYER_STR(A)      ((A) == LAYER_BELOW ? "below" : ((A) == LAYER_NORMAL ? "normal" : "above"))
#define HSH_MODE_STR(A)   ((A) == HONOR_SIZE_HINTS_TILED ? "tiled" : ((A) == HONOR_SIZE_HINTS_FLOATING ? "floating" : BOOL_STR(A)))

/* X11-specific config window masks — only used in backend_x11.c */
#define XCB_CONFIG_WINDOW_X_Y               (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y)
#define XCB_CONFIG_WINDOW_WIDTH_HEIGHT      (XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT)
#define XCB_CONFIG_WINDOW_X_Y_WIDTH_HEIGHT  (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT)

#define SHOULD_HONOR_SIZE_HINTS(hsh, state) \
	((hsh) && \
	 (((hsh) == HONOR_SIZE_HINTS_YES && \
	   (state) != STATE_FULLSCREEN) || \
	  ((hsh) == HONOR_SIZE_HINTS_TILED && \
	   (state) == STATE_TILED) || \
	  ((hsh) == HONOR_SIZE_HINTS_FLOATING && \
	   ((state) == STATE_FLOATING || (state) == STATE_PSEUDO_TILED))))

#define MAXLEN    256
#define SMALEN     32
#define INIT_CAP    8
#define MAX_STRING_SIZE (1024 * 1024)

/* malloc/realloc for arrays, failing on nmemb * size overflow (cf. reallocarray) */
static inline __attribute__((warn_unused_result)) void *safe_malloc_array(size_t nmemb, size_t size) {
	if (nmemb && size > SIZE_MAX / nmemb) {
		return NULL;  /* overflow */
	}
	return malloc(nmemb * size);
}

static inline __attribute__((warn_unused_result)) void *safe_realloc_array(void *ptr, size_t nmemb, size_t size) {
	if (nmemb && size > SIZE_MAX / nmemb) {
		return NULL;  /* overflow */
	}
	return realloc(ptr, nmemb * size);
}

/* Safe doubling with overflow check */
static inline __attribute__((warn_unused_result)) bool safe_double(size_t *val) {
	if (*val > SIZE_MAX / 2) {
		return false;  /* would overflow */
	}
	*val *= 2;
	return true;
}


#define cleaned_mask(m)   (m & ~(num_lock | scroll_lock | caps_lock))
#define streq(s1, s2)     (strcmp((s1), (s2)) == 0)
/* b is evaluated once: callers pass tree walks like sticky_count(n). */
#define unsigned_subtract(a, b)        \
	do {                               \
		typeof((a) - (b)) unsigned_sub_b = (b); \
		if (unsigned_sub_b > (a)) {      \
			(a) = 0;                     \
		} else {                         \
			(a) -= unsigned_sub_b;       \
		}                                \
	} while (false)


__attribute__((warn_unused_result)) int make_socket_path(char *buf, size_t buflen, const char *host, int dn, int sn);
int make_state_path(char *buf, size_t buflen, const char *host, int dn, int sn);
__attribute__((cold, format(printf, 1, 2))) void warn(char *fmt, ...);
__attribute__((cold, format(printf, 1, 2), noreturn)) void err(char *fmt, ...);
__attribute__((warn_unused_result)) char *read_string(const char *file_path, size_t *tlen);
__attribute__((warn_unused_result)) char *copy_string(char *str, size_t len);
__attribute__((warn_unused_result)) char *mktempfifo(const char *template);
__attribute__((warn_unused_result, format(printf, 2, 3))) int asprintf(char **buf, const char *fmt, ...);
__attribute__((warn_unused_result, format(printf, 2, 0))) int vasprintf(char **buf, const char *fmt, va_list args);
__attribute__((warn_unused_result)) bool is_hex_color(const char *color);

struct tokenize_state {
	bool in_escape;
	const char *pos;
	size_t len;
};
__attribute__((warn_unused_result)) char *tokenize_with_escape(struct tokenize_state *state, const char *s, char sep);


#endif

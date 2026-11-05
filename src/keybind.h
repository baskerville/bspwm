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

#ifndef BSPWM_KEYBIND_H
#define BSPWM_KEYBIND_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_KEYBINDS  256
#define MAX_COMMAND   512

/* Modifier flags (match X11 modifier masks) */
#define KBMOD_SHIFT   (1 << 0)
#define KBMOD_CAPS    (1 << 1)
#define KBMOD_CTRL    (1 << 2)
#define KBMOD_ALT     (1 << 3)  /* Mod1 */
#define KBMOD_MOD2    (1 << 4)
#define KBMOD_MOD3    (1 << 5)
#define KBMOD_SUPER   (1 << 6)  /* Mod4 / Logo */
#define KBMOD_MOD5    (1 << 7)

typedef struct {
	uint32_t modifiers;           /* KBMOD_* flags */
	uint32_t keysym;              /* XKB keysym (e.g. XKB_KEY_Return) */
	char command[MAX_COMMAND];    /* shell command to execute */
} keybind_t;

typedef struct {
	keybind_t binds[MAX_KEYBINDS];
	int count;
} keybind_table_t;

extern keybind_table_t keybind_table;

/* Initialize the keybinding system */
void keybind_init(void);

/* Add a keybinding. Returns false if table is full. */
bool keybind_add(uint32_t modifiers, uint32_t keysym, const char *command);

/* Remove all keybindings */
void keybind_clear(void);

/* Try to match a key event. Returns the command string if matched, NULL otherwise. */
const char *keybind_match(uint32_t modifiers, uint32_t keysym);

/* Parse a keybinding string like "super + Return" into modifiers + keysym.
 * Returns true on success. */
bool keybind_parse_combo(const char *combo, uint32_t *modifiers, uint32_t *keysym);

/* Execute a shell command asynchronously (fork+exec) */
void keybind_exec(const char *command);

/* Load keybindings from a config file (sxhkd-compatible format) */
bool keybind_load_config(const char *path);

#endif

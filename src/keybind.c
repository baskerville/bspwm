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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <xkbcommon/xkbcommon.h>
#include "helpers.h"
#include "messages.h"
#include "keybind.h"

keybind_table_t keybind_table;

void keybind_init(void)
{
	keybind_table.count = 0;
	memset(keybind_table.binds, 0, sizeof(keybind_table.binds));
}

bool keybind_add(uint32_t modifiers, uint32_t keysym, const char *command)
{
	if (keybind_table.count >= MAX_KEYBINDS)
		return false;

	keybind_t *kb = &keybind_table.binds[keybind_table.count++];
	kb->modifiers = modifiers;
	kb->keysym = keysym;
	snprintf(kb->command, sizeof(kb->command), "%s", command);
	return true;
}

void keybind_clear(void)
{
	keybind_table.count = 0;
}

const char *keybind_match(uint32_t modifiers, uint32_t keysym)
{
	for (int i = 0; i < keybind_table.count; i++) {
		keybind_t *kb = &keybind_table.binds[i];
		if (kb->keysym == keysym && kb->modifiers == modifiers)
			return kb->command;
	}
	return NULL;
}

/* Parse a modifier name to flag */
static uint32_t parse_modifier(const char *name)
{
	if (strcasecmp(name, "super") == 0 || strcasecmp(name, "mod4") == 0)
		return KBMOD_SUPER;
	if (strcasecmp(name, "alt") == 0 || strcasecmp(name, "mod1") == 0)
		return KBMOD_ALT;
	if (strcasecmp(name, "ctrl") == 0 || strcasecmp(name, "control") == 0)
		return KBMOD_CTRL;
	if (strcasecmp(name, "shift") == 0)
		return KBMOD_SHIFT;
	if (strcasecmp(name, "mod2") == 0)
		return KBMOD_MOD2;
	if (strcasecmp(name, "mod3") == 0)
		return KBMOD_MOD3;
	if (strcasecmp(name, "mod5") == 0)
		return KBMOD_MOD5;
	return 0;
}

bool keybind_parse_combo(const char *combo, uint32_t *modifiers, uint32_t *keysym)
{
	*modifiers = 0;
	*keysym = 0;

	/* Work on a copy */
	char buf[256];
	snprintf(buf, sizeof(buf), "%s", combo);

	/* Split by '+' and parse each token */
	char *saveptr;
	char *token = strtok_r(buf, "+", &saveptr);
	char *last_token = NULL;

	while (token) {
		/* Trim whitespace */
		while (*token && isspace((unsigned char)*token)) token++;
		char *end = token + strlen(token) - 1;
		while (end > token && isspace((unsigned char)*end)) *end-- = '\0';

		if (*token == '\0') {
			token = strtok_r(NULL, "+", &saveptr);
			continue;
		}

		/* Try as modifier first */
		uint32_t mod = parse_modifier(token);
		if (mod) {
			*modifiers |= mod;
		} else {
			last_token = token;
		}

		token = strtok_r(NULL, "+", &saveptr);
	}

	/* mod2 is conventionally Num Lock, which the key-press path strips from
	 * the event modifiers before matching (so a binding requiring it could
	 * never fire). Reject it rather than register a dead binding. */
	if (*modifiers & KBMOD_MOD2) {
		warn("keybind: mod2 (Num Lock) cannot be a binding modifier; ignoring '%s'.\n", combo);
		return false;
	}

	/* Last non-modifier token is the keysym */
	if (last_token) {
		*keysym = xkb_keysym_from_name(last_token, XKB_KEYSYM_CASE_INSENSITIVE);
		return *keysym != XKB_KEY_NoSymbol;
	}

	return false;
}

/* Characters that give the string shell semantics we must preserve. If any is
 * present we take the fork+sh path; otherwise the command is a plain argv we can
 * run ourselves. Being conservative here only costs a fork on rare bindings. */
static bool has_shell_meta(const char *s)
{
	for (; *s != '\0'; s++) {
		switch (*s) {
		case '\'': case '"': case '$': case '`': case '\\':
		case ';': case '&': case '|': case '<': case '>':
		case '(': case ')': case '{': case '}':
		case '[': case ']': case '*': case '?': case '~':
		case '#': case '\n':
			return true;
		}
	}
	return false;
}

/* Fast path for `bspc ...` bindings: instead of fork -> sh -> bspc -> connect to
 * our own control socket -> reply -> teardown (a few ms per keypress), tokenize
 * the command and dispatch it in-process, exactly as if bspc had sent it over
 * the socket. Returns true when handled; false means "fall back to fork+sh". */
static bool keybind_exec_inproc(const char *command)
{
	/* During an interactive X11 drag, track_pointer() is a nested blocking
	 * loop and the control socket is not accept()ed until it returns, so a
	 * keybind pressed mid-drag has always been deferred to drag-release (bspc
	 * blocks in recv()). Running it in-process instead would mutate the
	 * grabbed node while move/resize is still operating on it. Preserve the
	 * defer-to-release behavior by taking the (blocking) fork+sh path. */
	extern bool grabbing;
	if (grabbing) return false;

	while (*command == ' ' || *command == '\t') command++;

	/* Must be exactly the `bspc` program, then whitespace. */
	if (strncmp(command, "bspc", 4) != 0) return false;
	if (command[4] != ' ' && command[4] != '\t') return false;
	if (has_shell_meta(command)) return false;

	const char *rest = command + 4;
	char buf[MAX_COMMAND];
	size_t len = strlen(rest);
	if (len >= sizeof(buf)) return false;
	memcpy(buf, rest, len + 1);

	#define KB_MAX_ARGS 64
	char *argv[KB_MAX_ARGS];
	int argc = 0;
	for (char *p = buf; *p != '\0'; ) {
		while (*p == ' ' || *p == '\t') *p++ = '\0';
		if (*p == '\0') break;
		if (argc >= KB_MAX_ARGS) return false;
		argv[argc++] = p;
		while (*p != '\0' && *p != ' ' && *p != '\t') p++;
	}
	if (argc < 1) return false;

	/* `subscribe` would keep the reply stream alive as a dead subscriber. */
	if (streq(argv[0], "subscribe")) return false;

	/* Route the reply to our stderr (as the real bspc's output would end up on
	 * the WM's inherited stderr), so a failing binding is not silent. Use a
	 * dup: process_message() fcloses the stream, which must not close the real
	 * stderr fd. */
	int efd = dup(STDERR_FILENO);
	if (efd >= 0) {
		/* Non-blocking: this runs on the single event-loop thread. If stderr
		 * is a pipe nobody is draining, a verbose reply must drop output
		 * rather than block the whole WM in fwrite/fclose. */
		int fl = fcntl(efd, F_GETFL);
		if (fl != -1) {
			fcntl(efd, F_SETFL, fl | O_NONBLOCK);
		}
	}
	FILE *rsp = (efd >= 0) ? fdopen(efd, "w") : NULL;
	if (rsp == NULL) {
		if (efd >= 0) close(efd);
		rsp = fopen("/dev/null", "w");
		if (rsp == NULL) return false;
	}

	process_message(argv, argc, rsp); /* consumes (fcloses) rsp */
	return true;
	#undef KB_MAX_ARGS
}

void keybind_exec(const char *command)
{
	if (!command || !*command) return;

	if (keybind_exec_inproc(command)) return;

	pid_t pid = fork();
	if (pid == 0) {
		setsid();
		execl("/bin/sh", "/bin/sh", "-c", command, (char *)NULL);
		_exit(127);
	}
}

bool keybind_load_config(const char *path)
{
	FILE *f = fopen(path, "r");
	if (!f) return false;

	char line[1024];
	char pending_combo[256] = {0};

	while (fgets(line, sizeof(line), f)) {
		/* Strip trailing newline */
		size_t len = strlen(line);
		while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
			line[--len] = '\0';

		/* Skip empty lines and comments */
		char *p = line;
		while (*p && isspace((unsigned char)*p)) p++;
		if (*p == '\0' || *p == '#')
			continue;

		if (pending_combo[0] != '\0') {
			/* This line is the command for the previous combo */
			while (*p && isspace((unsigned char)*p)) p++;

			uint32_t modifiers, keysym;
			if (keybind_parse_combo(pending_combo, &modifiers, &keysym)) {
				if (!keybind_add(modifiers, keysym, p))
					warn("keybind: table full (max %d), dropping '%s'.\n",
					     MAX_KEYBINDS, pending_combo);
			} else {
				warn("keybind: could not parse combo '%s'.\n", pending_combo);
			}
			pending_combo[0] = '\0';
		} else {
			/* Check if line contains '+' (key combo) */
			if (strchr(p, '+') != NULL) {
				snprintf(pending_combo, sizeof(pending_combo), "%s", p);
			}
		}
	}

	fclose(f);
	return true;
}

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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "types.h"
#include "desktop.h"
#include "monitor.h"
#include "settings.h"
#include "messages.h"
#include "pointer.h"
#include "events.h"
#include "common.h"
#include "window.h"
#include "history.h"
#include "ewmh.h"
#include "rule.h"
#include "restore.h"
#include "query.h"
#include "keybind.h"
#include "ipc.h"
#include "bspwm.h"
#include "backend_x11.h"

/* Global state */
int default_screen, screen_width, screen_height;
uint32_t clients_count;
uint32_t id_counter;
bspwm_wid_t root;
char config_path[MAXLEN];

monitor_t *mon;
monitor_t *mon_head;
monitor_t *mon_tail;
monitor_t *pri_mon;
history_t *history_head;
history_t *history_tail;
history_t *history_needle;
rule_t *rule_head;
rule_t *rule_tail;
stacking_list_t *stack_head;
stacking_list_t *stack_tail;
subscriber_list_t *subscribe_head;
subscriber_list_t *subscribe_tail;
pending_rule_t *pending_rule_head;
pending_rule_t *pending_rule_tail;

bspwm_wid_t meta_window;
motion_recorder_t motion_recorder;
int exit_status;
int epoll_fd;
int sock_fd = -1;
int dpy_fd = -1;

bool auto_raise;
bool sticky_still;
bool hide_sticky;
bool record_history;
volatile sig_atomic_t running;
volatile bool restart;
bool randr;

/* Copy the restart state file aside before it gets consumed and unlinked.
 * Best-effort: a failure here must never affect startup, so every error path
 * simply gives up quietly. */
static void preserve_state_file(const char *state_path)
{
	char backup_path[MAXLEN];
	int n = snprintf(backup_path, sizeof(backup_path), "%s.last", state_path);
	if (n < 0 || (size_t) n >= sizeof(backup_path)) {
		return;
	}

	FILE *src = fopen(state_path, "r");
	if (src == NULL) {
		return;
	}

	FILE *dst = fopen(backup_path, "w");
	if (dst == NULL) {
		fclose(src);
		return;
	}

	char buf[BUFSIZ];
	size_t got;
	while ((got = fread(buf, 1, sizeof(buf), src)) > 0) {
		if (fwrite(buf, 1, got, dst) != got) {
			break;
		}
	}

	fclose(dst);
	fclose(src);
}

int main(int argc, char *argv[])
{
	char socket_path[MAXLEN] = {0};
	bool socket_path_set = false;
	char state_path[MAXLEN] = {0};
	int run_level = 0;
	config_path[0] = '\0';
	struct sockaddr_un sock_address;
	char *end;
	int opt;
	bool cli_ignore_monitor_updates = false;

	while ((opt = getopt(argc, argv, "hvc:s:o:i")) != -1) {
		switch (opt) {
			case 'h':
				printf(WM_NAME " [-h|-v|-c CONFIG_PATH|-i]\n");
				exit(EXIT_SUCCESS);
				break;
			case 'v':
				printf("%s\n", VERSION);
				exit(EXIT_SUCCESS);
				break;
			case 'c':
				snprintf(config_path, sizeof(config_path), "%s", optarg);
				break;
			case 's':
				run_level |= 1;
				snprintf(state_path, sizeof(state_path), "%s", optarg);
				break;
			case 'o':
				run_level |= 2;
				sock_fd = strtol(optarg, &end, 0);
				if (*end != '\0') {
					sock_fd = -1;
				}
				break;
			case 'i':
				cli_ignore_monitor_updates = true;
				break;
		}
	}

	if (config_path[0] == '\0') {
		char *config_home = getenv(CONFIG_HOME_ENV);
		if (config_home != NULL) {
			snprintf(config_path, sizeof(config_path), "%s/%s/%s", config_home, WM_NAME, CONFIG_NAME);
		} else {
			char *home = getenv("HOME");
			if (home != NULL) {
				snprintf(config_path, sizeof(config_path), "%s/%s/%s/%s", home, ".config", WM_NAME, CONFIG_NAME);
			}
		}
	}

	if (backend_init(&default_screen) != 0) {
		err("Can't open display.\n");
	}

	if (!backend_check_connection()) {
		exit(EXIT_FAILURE);
	}

	load_settings();
	if (cli_ignore_monitor_updates) {
		ignore_monitor_updates = true;
	}
	setup();

	if (state_path[0] != '\0') {
		/* Keep a copy before consuming it. The state file is the only record
		 * of what the tree looked like across a restart, and it is exactly
		 * what you need to diagnose a restore that went wrong — but the
		 * unlink below destroys it every time, so by the time anyone notices
		 * a problem the evidence is already gone. */
		preserve_state_file(state_path);
		restore_state(state_path);
		unlink(state_path);
		adopt_orphans();
	}

	dpy_fd = backend_get_fd();

	/* Resolve the control-socket path. This must happen even on a restart
	 * (sock_fd inherited via -o) so we can verify the path still names our
	 * listening socket before reusing the inherited fd. */
	char *sp = getenv(SOCKET_ENV_VAR);
	if (sp != NULL) {
		snprintf(socket_path, sizeof(socket_path), "%s", sp);
	} else {
		char *host = NULL;
		int dn = 0, sn = 0;
		if (backend_parse_display(&host, &dn, &sn)) {
			int ret = make_socket_path(socket_path, sizeof(socket_path), host, dn, sn);
			if (ret < 0 || (size_t)ret >= sizeof(socket_path)) {
				free(host);
				err("Socket path too long.\n");
			}
		}
		free(host);
	}

	if (socket_path[0] == '\0') {
		err("Couldn't determine the socket path.\n");
	}

	sock_address.sun_family = AF_UNIX;

	size_t socket_path_len = strlen(socket_path);
	if (socket_path_len >= sizeof(sock_address.sun_path)) {
		err("Socket path too long (%zu >= %zu): %s\n",
		    socket_path_len, sizeof(sock_address.sun_path), socket_path);
	}

	strcpy(sock_address.sun_path, socket_path);

	/* Always bind a fresh socket, on both a fresh start and a restart.
	 *
	 * A restart inherits the previously bound listening socket via -o. Reusing
	 * it avoids a momentary disruption, but the inherited fd's filesystem name
	 * can be gone (swept from /tmp, or unlinked by a racing fresh bind) or —
	 * worse — shadowed by a different, stale socket that still passes an
	 * S_ISSOCK check while connect() by path reaches nothing. There is no
	 * portable way to confirm that the path still names *our* socket
	 * (getsockname reports the original bind name even after the entry is
	 * gone), so any reuse check has a hole through which clients fail silently
	 * and keybinds die. A bspc exchange is a sub-millisecond one-shot and
	 * subscribers are dropped across a re-exec regardless, so rebinding costs
	 * nothing observable and guarantees the path names the live socket. */
	if (sock_fd != -1) {
		close(sock_fd);
		sock_fd = -1;
	}

	{
		sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

		if (sock_fd == -1) {
			err("Couldn't create the socket.\n");
		}

		/* Never clobber a *live* bspwm's socket. The unlink below would
		 * otherwise remove the running WM's socket file if this instance
		 * resolved the same path — e.g. a nested or test bspwm on another
		 * display that inherited this BSPWM_SOCKET and so got past the
		 * "another WM is running" check. The victim keeps listening on a
		 * now-nameless inode and every bspc/keybind silently fails until it
		 * is restarted. If the path already names a socket we can connect to,
		 * a bspwm is alive there; refuse instead of destroying it. On our own
		 * restart the inherited socket was just closed above, so nothing is
		 * listening and this probe passes. */
		{
			int probe = socket(AF_UNIX, SOCK_STREAM, 0);
			if (probe != -1) {
				bool alive = connect(probe, (struct sockaddr *) &sock_address,
				                     sizeof(sock_address)) == 0;
				close(probe);
				if (alive) {
					err("A bspwm is already listening on %s.\n", socket_path);
				}
			}
		}

		unlink(socket_path);

		if (bind(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address)) == -1) {
			err("Couldn't bind a name to the socket.\n");
		}

		chmod(socket_path, 0600);

		if (listen(sock_fd, SOMAXCONN) == -1) {
			err("Couldn't listen to the socket.\n");
		}

		socket_path_set = true;
	}

	/* Hand the path to everything we spawn (config file, keybinds, rules), so
	 * bspc does not have to re-derive it from DISPLAY. */
	setenv(SOCKET_ENV_VAR, socket_path, true);

	fcntl(sock_fd, F_SETFD, FD_CLOEXEC | fcntl(sock_fd, F_GETFD));

	epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (epoll_fd == -1) {
		err("Couldn't create epoll instance.\n");
	}

	struct epoll_event ep_ev;
	ep_ev.events = EPOLLIN;
	ep_ev.data.fd = sock_fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ep_ev);
	ep_ev.data.fd = dpy_fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dpy_fd, &ep_ev);

	struct sigaction sigact;
	sigact.sa_handler = sig_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGPIPE, &sigact, NULL);
	run_config(run_level);
	backend_grab_keys();
	running = true;

	#define MAX_EPOLL_EVENTS 16
	struct epoll_event ep_events[MAX_EPOLL_EVENTS];

	while (running) {

		backend_flush();

		int nfds = epoll_wait(epoll_fd, ep_events, MAX_EPOLL_EVENTS, ipc_timeout());
		if (nfds == -1) {
			if (errno == EINTR)
				continue;
			break;
		}

		ipc_expire();

		for (int i = 0; i < nfds; i++) {
			int fd = ep_events[i].data.fd;

			/* Check pending rules */
			bool handled = false;
			pending_rule_t *pr = pending_rule_head;
			while (pr != NULL) {
				pending_rule_t *next = pr->next;
				if (pr->fd == fd) {
					if (manage_window(pr->win, pr->csq, pr->fd)) {
						for (event_queue_t *eq = pr->event_head; eq != NULL; eq = eq->next) {
							handle_event(eq->data);
						}
					}
					remove_pending_rule(pr);
					handled = true;
					break;
				}
				pr = next;
			}
			if (handled)
				continue;

			if (fd == sock_fd) {
				ipc_accept(sock_fd);
			} else if (ipc_handle(fd)) {
				continue;
			} else if (fd == dpy_fd) {
				xcb_generic_event_t *event;
				while ((event = xcb_poll_for_event(dpy)) != NULL) {
					handle_event(event);
					free(event);
				}
			}
		}

		/* Drain events xcb has already buffered in userspace. A handler above
		 * (pending-rule window management, or any IPC command that issues an X
		 * round-trip) pulls bytes off the socket into xcb's queue; those events
		 * then sit unprocessed because epoll won't re-fire on an empty socket,
		 * so an enter-notify/motion/focus-in stalls until the next unrelated
		 * event. Flush the queue every iteration so input never waits. */
		{
			xcb_generic_event_t *qev;
			while ((qev = xcb_poll_for_queued_event(dpy)) != NULL) {
				handle_event(qev);
				free(qev);
			}
		}

		if (!backend_check_connection()) {
			running = false;
		}

		static unsigned int prune_counter;
		if (++prune_counter >= 64) {
			prune_counter = 0;
			prune_dead_subscribers();
		}
	}

	if (restart) {
		char *host = NULL;
		int dn = 0, sn = 0;
		bool have_path = false;
		if (backend_parse_display(&host, &dn, &sn)) {
			make_state_path(state_path, sizeof(state_path), host, dn, sn);
			have_path = true;
		}
		free(host);

		/* If we can't write the state, don't re-exec with -s pointing at a
		 * file that isn't there: the new instance would come up with an empty
		 * tree and every window would be orphaned. Better to exit and let the
		 * session bring us back cleanly. (This also used to reach fclose(NULL)
		 * and crash outright when fopen failed.) */
		FILE *f = NULL;
		if (have_path) {
			/* Exclusive create, 0600: never follow a symlink or reuse a file
			 * planted at this path. The state file now lives under
			 * $XDG_RUNTIME_DIR (0700), so a stale one from a prior run of ours
			 * is the only thing we might collide with — clear it first. */
			unlink(state_path);
			int sfd = open(state_path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
			if (sfd != -1) {
				f = fdopen(sfd, "w");
				if (f == NULL) {
					close(sfd);
				}
			}
		}
		if (f == NULL) {
			warn("Can't write the state file '%s'; cancelling restart.\n",
			     have_path ? state_path : "<no display>");
			restart = false;
			exit_status = 1;
		} else {
			query_state(f);
			fclose(f);
		}
	}

	ipc_close_all();
	cleanup();
	ungrab_buttons();
	backend_destroy_window(meta_window);
	backend_destroy_window(motion_recorder.id);
	backend_destroy();

	if (restart) {
		fcntl(sock_fd, F_SETFD, ~FD_CLOEXEC & fcntl(sock_fd, F_GETFD));

		int rargc;
		for (rargc = 0; rargc < argc; rargc++) {
			if (streq("-s", argv[rargc])) {
				break;
			}
		}

		size_t len = (size_t)rargc + 5;
		char **rargv = safe_malloc_array(len, sizeof(char *));
		if (rargv != NULL) {
			for (int i = 0; i < rargc; i++) {
				rargv[i] = argv[i];
			}

			char sock_fd_arg[SMALEN];
			snprintf(sock_fd_arg, sizeof(sock_fd_arg), "%i", sock_fd);

			rargv[rargc] = "-s";
			rargv[rargc + 1] = state_path;
			rargv[rargc + 2] = "-o";
			rargv[rargc + 3] = sock_fd_arg;
			rargv[rargc + 4] = 0;

			execvp(*rargv, rargv);
			free(rargv);
		} else {
			warn("Failed to allocate restart argv\n");
		}
		exit_status = 1;
	}

	close(epoll_fd);
	if (!restart) {
		close(sock_fd);
	}
	if (socket_path_set) {
		unlink(socket_path);
	}

	return exit_status;
}

void init(void)
{
	clients_count = 0;
	id_counter = 0;
	mon = mon_head = mon_tail = pri_mon = NULL;
	history_head = history_tail = history_needle = NULL;
	rule_head = rule_tail = NULL;
	stack_head = stack_tail = NULL;
	subscribe_head = subscribe_tail = NULL;
	pending_rule_head = pending_rule_tail = NULL;
	auto_raise = sticky_still = hide_sticky = record_history = true;
	exit_status = 0;
	restart = false;
}

void setup(void)
{
	init();
	keybind_init();

	backend_ewmh_init();
	pointer_init();
	x11_setup_screen();
	root = backend_get_root();
	if (!backend_register_root_events()) {
		backend_destroy();
		err("Another window manager is already running.\n");
	}
	backend_get_screen_size(&screen_width, &screen_height);

	/* Create internal windows via backend */
	bspwm_rect_t meta_rect = {-1, -1, 1, 1};
	meta_window = backend_create_internal_window("meta", meta_rect, true);

	bspwm_rect_t mr_rect = {0, 0, 1, 1};
	motion_recorder.id = backend_create_internal_window("motion_recorder", mr_rect, true);
	motion_recorder.sequence = 0;
	motion_recorder.enabled = false;

	x11_setup_ewmh_supported();
	ewmh_set_supporting(meta_window);
	x11_setup_atoms();
	x11_setup_randr();

	if (randr && update_monitors()) {
		/* monitors set up via RandR */
	} else if (!randr || !mon_head) {
		randr = false;
		warn("Couldn't retrieve monitors via RandR.\n");
		/* Fallback: single screen */
		bspwm_rect_t rect = {0, 0, screen_width, screen_height};
		monitor_t *m = make_monitor(NULL, &rect, BSPWM_WID_NONE);
		add_monitor(m);
		add_desktop(m, make_desktop(NULL, BSPWM_WID_NONE));
	}

	ewmh_update_number_of_desktops();
	ewmh_update_desktop_names();
	ewmh_update_desktop_viewport();
	ewmh_update_current_desktop();
	clear_input_focus();
}

void cleanup(void)
{
	mon = NULL;

	while (mon_head != NULL) {
		remove_monitor(mon_head);
	}
	while (rule_head != NULL) {
		remove_rule(rule_head);
	}
	while (subscribe_head != NULL) {
		remove_subscriber(subscribe_head);
	}
	while (pending_rule_head != NULL) {
		remove_pending_rule(pending_rule_head);
	}

	empty_history();
}

void sig_handler(int sig)
{
	if (sig == SIGCHLD) {
		while (waitpid(-1, 0, WNOHANG) > 0)
			;
	} else if (sig == SIGINT || sig == SIGHUP || sig == SIGTERM) {
		running = 0;
	}
}

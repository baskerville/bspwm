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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <xcb/xinerama.h>
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
#include "bspwm.h"

xcb_connection_t *dpy;
int default_screen, screen_width, screen_height;
uint32_t clients_count;
xcb_screen_t *screen;
xcb_window_t root;
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

xcb_window_t meta_window;
motion_recorder_t motion_recorder;
xcb_atom_t WM_STATE;
xcb_atom_t WM_TAKE_FOCUS;
xcb_atom_t WM_DELETE_WINDOW;
int exit_status;

bool auto_raise;
bool sticky_still;
bool hide_sticky;
bool record_history;
bool running;
bool restart;
bool randr;

int main(int argc, char *argv[])
{
	fd_set descriptors;
	char socket_path[MAXLEN];
	char state_path[MAXLEN] = {0};
	int run_level = 0;
	config_path[0] = '\0';
	int sock_fd = -1, cli_fd, dpy_fd, max_fd, n;
	struct sockaddr_un sock_address;
	char msg[BUFSIZ] = {0};
	xcb_generic_event_t *event;
	char *end;
	int opt;

	while ((opt = getopt(argc, argv, "hvc:s:o:")) != -1) {
		switch (opt) {
			case 'h':
				printf(WM_NAME " [-h|-v|-c CONFIG_PATH]\n");
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
		}
	}

	if (config_path[0] == '\0') {
		char *config_home = getenv(CONFIG_HOME_ENV);
		if (config_home != NULL) {
			snprintf(config_path, sizeof(config_path), "%s/%s/%s", config_home, WM_NAME, CONFIG_NAME);
		} else {
			snprintf(config_path, sizeof(config_path), "%s/%s/%s/%s", getenv("HOME"), ".config", WM_NAME, CONFIG_NAME);
		}
	}

	dpy = xcb_connect(NULL, &default_screen);

	if (!check_connection(dpy)) {
		exit(EXIT_FAILURE);
	}

	load_settings();
	setup();

	if (state_path[0] != '\0') {
		restore_state(state_path);
		unlink(state_path);
	}

	dpy_fd = xcb_get_file_descriptor(dpy);

	if (sock_fd == -1) {
		char *sp = getenv(SOCKET_ENV_VAR);
		if (sp != NULL) {
			snprintf(socket_path, sizeof(socket_path), "%s", sp);
		} else {
			char *host = NULL;
			int dn = 0, sn = 0;
			if (xcb_parse_display(NULL, &host, &dn, &sn) != 0) {
				snprintf(socket_path, sizeof(socket_path), SOCKET_PATH_TPL, host, dn, sn);
			}
			free(host);
		}

		sock_address.sun_family = AF_UNIX;
		if (snprintf(sock_address.sun_path, sizeof(sock_address.sun_path), "%s", socket_path) < 0) {
			err("Couldn't write the socket path.\n");
		}

		sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

		if (sock_fd == -1) {
			err("Couldn't create the socket.\n");
		}

		unlink(socket_path);

		if (bind(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address)) == -1) {
			err("Couldn't bind a name to the socket.\n");
		}

		if (listen(sock_fd, SOMAXCONN) == -1) {
			err("Couldn't listen to the socket.\n");
		}
	}

	fcntl(sock_fd, F_SETFD, FD_CLOEXEC | fcntl(sock_fd, F_GETFD));

	signal(SIGINT, sig_handler);
	signal(SIGHUP, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGCHLD, sig_handler);
	signal(SIGPIPE, SIG_IGN);
	run_config(run_level);
	running = true;

	while (running) {

		xcb_flush(dpy);

		FD_ZERO(&descriptors);
		FD_SET(sock_fd, &descriptors);
		FD_SET(dpy_fd, &descriptors);
		max_fd = MAX(sock_fd, dpy_fd);

		for (pending_rule_t *pr = pending_rule_head; pr != NULL; pr = pr->next) {
			FD_SET(pr->fd, &descriptors);
			if (pr->fd > max_fd) {
				max_fd = pr->fd;
			}
		}

		if (select(max_fd + 1, &descriptors, NULL, NULL, NULL) > 0) {

			pending_rule_t *pr = pending_rule_head;
			while (pr != NULL) {
				pending_rule_t *next = pr->next;
				if (FD_ISSET(pr->fd, &descriptors)) {
					if (manage_window(pr->win, pr->csq, pr->fd)) {
						for (event_queue_t *eq = pr->event_head; eq != NULL; eq = eq->next) {
							handle_event(&eq->event);
						}
					}
					remove_pending_rule(pr);
				}
				pr = next;
			}

			if (FD_ISSET(sock_fd, &descriptors)) {
				cli_fd = accept(sock_fd, NULL, 0);
				if (cli_fd > 0 && (n = recv(cli_fd, msg, sizeof(msg)-1, 0)) > 0) {
					msg[n] = '\0';
					FILE *rsp = fdopen(cli_fd, "w");
					if (rsp != NULL) {
						handle_message(msg, n, rsp);
					} else {
						warn("Can't open the client socket as file.\n");
						close(cli_fd);
					}
				}
			}

			if (FD_ISSET(dpy_fd, &descriptors)) {
				while ((event = xcb_poll_for_event(dpy)) != NULL) {
					handle_event(event);
					free(event);
				}
			}

		}

		if (!check_connection(dpy)) {
			running = false;
		}

		prune_dead_subscribers();
	}

	if (restart) {
		char *host = NULL;
		int dn = 0, sn = 0;
		if (xcb_parse_display(NULL, &host, &dn, &sn) != 0) {
			snprintf(state_path, sizeof(state_path), STATE_PATH_TPL, host, dn, sn);
		}
		free(host);
		FILE *f = fopen(state_path, "w");
		query_state(f);
		fclose(f);
	}

	cleanup();
	ungrab_buttons();
	xcb_ewmh_connection_wipe(ewmh);
	xcb_destroy_window(dpy, meta_window);
	xcb_destroy_window(dpy, motion_recorder.id);
	free(ewmh);
	xcb_flush(dpy);
	xcb_disconnect(dpy);

	if (restart) {
		fcntl(sock_fd, F_SETFD, ~FD_CLOEXEC & fcntl(sock_fd, F_GETFD));

		int rargc;
		for (rargc = 0; rargc < argc; rargc++) {
			if (streq("-s", argv[rargc])) {
				break;
			}
		}

		int len = rargc + 5;
		char **rargv = malloc(len * sizeof(char *));

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

		exit_status = execvp(*rargv, rargv);
		free(rargv);
	}

	close(sock_fd);
	unlink(socket_path);

	return exit_status;
}

void init(void)
{
	clients_count = 0;
	mon = mon_head = mon_tail = pri_mon = NULL;
	history_head = history_tail = history_needle = NULL;
	rule_head = rule_tail = NULL;
	stack_head = stack_tail = NULL;
	subscribe_head = subscribe_tail = NULL;
	pending_rule_head = pending_rule_tail = NULL;
	auto_raise = sticky_still = hide_sticky = record_history = true;
	randr_base = 0;
	exit_status = 0;
	restart = false;
}

void setup(void)
{
	init();
	ewmh_init();
	pointer_init();

	screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;

	if (screen == NULL) {
		err("Can't acquire the default screen.\n");
	}

	root = screen->root;
	register_events();

	screen_width = screen->width_in_pixels;
	screen_height = screen->height_in_pixels;

	meta_window = xcb_generate_id(dpy);
	xcb_create_window(dpy, XCB_COPY_FROM_PARENT, meta_window, root, -1, -1, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT, XCB_NONE, NULL);
	xcb_icccm_set_wm_class(dpy, meta_window, sizeof(META_WINDOW_IC), META_WINDOW_IC);

	motion_recorder.id = xcb_generate_id(dpy);
	motion_recorder.sequence = 0;
	motion_recorder.enabled = false;
	uint32_t values[] = {XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_POINTER_MOTION};
	xcb_create_window(dpy, XCB_COPY_FROM_PARENT, motion_recorder.id, root, 0, 0, 1, 1, 0,
	                  XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT, XCB_CW_EVENT_MASK, values);
	xcb_icccm_set_wm_class(dpy, motion_recorder.id, sizeof(MOTION_RECORDER_IC), MOTION_RECORDER_IC);

	xcb_atom_t net_atoms[] = {ewmh->_NET_SUPPORTED,
	                          ewmh->_NET_SUPPORTING_WM_CHECK,
	                          ewmh->_NET_DESKTOP_NAMES,
	                          ewmh->_NET_DESKTOP_VIEWPORT,
	                          ewmh->_NET_NUMBER_OF_DESKTOPS,
	                          ewmh->_NET_CURRENT_DESKTOP,
	                          ewmh->_NET_CLIENT_LIST,
	                          ewmh->_NET_ACTIVE_WINDOW,
	                          ewmh->_NET_CLOSE_WINDOW,
	                          ewmh->_NET_WM_STRUT_PARTIAL,
	                          ewmh->_NET_WM_DESKTOP,
	                          ewmh->_NET_WM_STATE,
	                          ewmh->_NET_WM_STATE_HIDDEN,
	                          ewmh->_NET_WM_STATE_FULLSCREEN,
	                          ewmh->_NET_WM_STATE_BELOW,
	                          ewmh->_NET_WM_STATE_ABOVE,
	                          ewmh->_NET_WM_STATE_STICKY,
	                          ewmh->_NET_WM_STATE_DEMANDS_ATTENTION,
	                          ewmh->_NET_WM_WINDOW_TYPE,
	                          ewmh->_NET_WM_WINDOW_TYPE_DOCK,
	                          ewmh->_NET_WM_WINDOW_TYPE_DESKTOP,
	                          ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION,
	                          ewmh->_NET_WM_WINDOW_TYPE_DIALOG,
	                          ewmh->_NET_WM_WINDOW_TYPE_UTILITY,
	                          ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR};

	xcb_ewmh_set_supported(ewmh, default_screen, LENGTH(net_atoms), net_atoms);
	ewmh_set_supporting(meta_window);

#define GETATOM(a) \
	get_atom(#a, &a);
	GETATOM(WM_STATE)
	GETATOM(WM_DELETE_WINDOW)
	GETATOM(WM_TAKE_FOCUS)
#undef GETATOM

	const xcb_query_extension_reply_t *qep = xcb_get_extension_data(dpy, &xcb_randr_id);
	if (qep->present && update_monitors()) {
		randr = true;
		randr_base = qep->first_event;
		xcb_randr_select_input(dpy, root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
	} else {
		randr = false;
		warn("Couldn't retrieve monitors via RandR.\n");
		bool xinerama_is_active = false;

		if (xcb_get_extension_data(dpy, &xcb_xinerama_id)->present) {
			xcb_xinerama_is_active_reply_t *xia = xcb_xinerama_is_active_reply(dpy, xcb_xinerama_is_active(dpy), NULL);
			if (xia != NULL) {
				xinerama_is_active = xia->state;
				free(xia);
			}
		}

		if (xinerama_is_active) {
			xcb_xinerama_query_screens_reply_t *xsq = xcb_xinerama_query_screens_reply(dpy, xcb_xinerama_query_screens(dpy), NULL);
			xcb_xinerama_screen_info_t *xsi = xcb_xinerama_query_screens_screen_info(xsq);
			int n = xcb_xinerama_query_screens_screen_info_length(xsq);
			for (int i = 0; i < n; i++) {
				xcb_xinerama_screen_info_t info = xsi[i];
				xcb_rectangle_t rect = (xcb_rectangle_t) {info.x_org, info.y_org, info.width, info.height};
				monitor_t *m = make_monitor(NULL, &rect, XCB_NONE);
				add_monitor(m);
				add_desktop(m, make_desktop(NULL, XCB_NONE));
			}
			free(xsq);
		} else {
			warn("Xinerama is inactive.\n");
			xcb_rectangle_t rect = (xcb_rectangle_t) {0, 0, screen_width, screen_height};
			monitor_t *m = make_monitor(NULL, &rect, XCB_NONE);
			add_monitor(m);
			add_desktop(m, make_desktop(NULL, XCB_NONE));
		}
	}

	ewmh_update_number_of_desktops();
	ewmh_update_desktop_names();
	ewmh_update_desktop_viewport();
	ewmh_update_current_desktop();
	xcb_get_input_focus_reply_t *ifo = xcb_get_input_focus_reply(dpy, xcb_get_input_focus(dpy), NULL);
	if (ifo != NULL && (ifo->focus == XCB_INPUT_FOCUS_POINTER_ROOT || ifo->focus == XCB_NONE)) {
		clear_input_focus();
	}
	free(ifo);
}

void register_events(void)
{
	uint32_t values[] = {ROOT_EVENT_MASK};
	xcb_generic_error_t *e = xcb_request_check(dpy, xcb_change_window_attributes_checked(dpy, root, XCB_CW_EVENT_MASK, values));
	if (e != NULL) {
		free(e);
		xcb_ewmh_connection_wipe(ewmh);
		free(ewmh);
		xcb_disconnect(dpy);
		err("Another window manager is already running.\n");
	}
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

bool check_connection (xcb_connection_t *dpy)
{
	int xerr;
	if ((xerr = xcb_connection_has_error(dpy)) != 0) {
		warn("The server closed the connection: ");
		switch (xerr) {
			case XCB_CONN_ERROR:
				warn("socket, pipe or stream error.\n");
				break;
			case XCB_CONN_CLOSED_EXT_NOTSUPPORTED:
				warn("unsupported extension.\n");
				break;
			case XCB_CONN_CLOSED_MEM_INSUFFICIENT:
				warn("not enough memory.\n");
				break;
			case XCB_CONN_CLOSED_REQ_LEN_EXCEED:
				warn("request length exceeded.\n");
				break;
			case XCB_CONN_CLOSED_PARSE_ERR:
				warn("can't parse display string.\n");
				break;
			case XCB_CONN_CLOSED_INVALID_SCREEN:
				warn("invalid screen.\n");
				break;
			case XCB_CONN_CLOSED_FDPASSING_FAILED:
				warn("failed to pass FD.\n");
				break;
			default:
				warn("unknown error.\n");
				break;
		}
		return false;
	} else {
		return true;
	}
}

void sig_handler(int sig)
{
	if (sig == SIGCHLD) {
		signal(sig, sig_handler);
		while (waitpid(-1, 0, WNOHANG) > 0)
			;
	} else if (sig == SIGINT || sig == SIGHUP || sig == SIGTERM) {
		running = false;
	}
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

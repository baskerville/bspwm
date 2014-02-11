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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#ifdef __OpenBSD__
#include <sys/types.h>
#endif
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <unistd.h>
#include <xcb/xinerama.h>
#include "types.h"
#include "desktop.h"
#include "monitor.h"
#include "settings.h"
#include "messages.h"
#include "subscribe.h"
#include "events.h"
#include "common.h"
#include "window.h"
#include "history.h"
#include "stack.h"
#include "ewmh.h"
#include "rule.h"
#include "tree.h"
#include "bspwm.h"

int main(int argc, char *argv[])
{
	fd_set descriptors;
	char socket_path[MAXLEN];
	config_path[0] = '\0';
	int sock_fd, cli_fd, dpy_fd, max_fd, n;
	struct sockaddr_un sock_address;
	size_t rsp_len = 0;
	char msg[BUFSIZ] = {0};
	char rsp[BUFSIZ] = {0};
	xcb_generic_event_t *event;
	char opt;

	while ((opt = getopt(argc, argv, "hvc:")) != (char)-1) {
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
		}
	}

	if (config_path[0] == '\0') {
		char *config_home = getenv(CONFIG_HOME_ENV);
		if (config_home != NULL)
			snprintf(config_path, sizeof(config_path), "%s/%s/%s", config_home, WM_NAME, CONFIG_NAME);
		else
			snprintf(config_path, sizeof(config_path), "%s/%s/%s/%s", getenv("HOME"), ".config", WM_NAME, CONFIG_NAME);
	}

	dpy = xcb_connect(NULL, &default_screen);

	if (xcb_connection_has_error(dpy))
		err("Can't open the default display.\n");

	setup();

	dpy_fd = xcb_get_file_descriptor(dpy);

	char *sp = getenv(SOCKET_ENV_VAR);
	if (sp != NULL)
		snprintf(socket_path, sizeof(socket_path), "%s", sp);
	else
		snprintf(socket_path, sizeof(socket_path), SOCKET_PATH_TPL, getenv("DISPLAY"));

	sock_address.sun_family = AF_UNIX;
	snprintf(sock_address.sun_path, sizeof(sock_address.sun_path), "%s", socket_path);

	sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (sock_fd == -1)
		err("Couldn't create the socket.\n");

	if (bind(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address)) == -1)
		err("Couldn't bind a name to the socket.\n");

	if (listen(sock_fd, SOMAXCONN) == -1)
		err("Couldn't listen to the socket.\n");

	signal(SIGINT, sig_handler);
	signal(SIGHUP, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGCHLD, sig_handler);
	signal(SIGPIPE, SIG_IGN);
	load_settings();
	run_config();
	running = true;

	while (running) {

		xcb_flush(dpy);

		FD_ZERO(&descriptors);
		FD_SET(sock_fd, &descriptors);
		FD_SET(dpy_fd, &descriptors);
		max_fd = MAX(sock_fd, dpy_fd);
		for (pending_rule_t *pr = pending_rule_head; pr != NULL; pr = pr->next) {
			FD_SET(pr->fd, &descriptors);
			if (pr->fd > max_fd)
				max_fd = pr->fd;
		}

		if (select(max_fd + 1, &descriptors, NULL, NULL, NULL) > 0) {

			pending_rule_t *pr = pending_rule_head;
			while (pr != NULL) {
				pending_rule_t *next = pr->next;
				if (FD_ISSET(pr->fd, &descriptors)) {
					manage_window(pr->win, pr->csq, pr->fd);
					remove_pending_rule(pr);
				}
				pr = next;
			}

			if (FD_ISSET(sock_fd, &descriptors)) {
				cli_fd = accept(sock_fd, NULL, 0);
				if (cli_fd > 0 && (n = recv(cli_fd, msg, sizeof(msg), 0)) > 0) {
					msg[n] = '\0';
					if (handle_message(msg, n, rsp)) {
						rsp_len = strlen(rsp);
					} else {
						rsp[0] = MESSAGE_FAILURE;
						rsp_len = 1;
					}
					if (rsp_len == 1 && rsp[0] == MESSAGE_SUBSCRIBE) {
						add_subscriber(cli_fd);
					} else {
						send(cli_fd, rsp, rsp_len, 0);
						close(cli_fd);
					}
					rsp[0] = '\0';
				}
			}

			if (FD_ISSET(dpy_fd, &descriptors)) {
				while ((event = xcb_poll_for_event(dpy)) != NULL) {
					handle_event(event);
					free(event);
				}
			}
		}

		if (xcb_connection_has_error(dpy)) {
			warn("The server closed the connection.\n");
			running = false;
		}
	}

	cleanup();
	close(sock_fd);
	unlink(socket_path);
	xcb_ewmh_connection_wipe(ewmh);
	xcb_destroy_window(dpy, motion_recorder);
	free(ewmh);
	xcb_flush(dpy);
	xcb_disconnect(dpy);
	return exit_status;
}

void init(void)
{
	num_monitors = num_desktops = num_clients = 0;
	monitor_uid = desktop_uid = 0;
	mon = mon_head = mon_tail = pri_mon = NULL;
	history_head = history_tail = history_needle = NULL;
	rule_head = rule_tail = NULL;
	stack_head = stack_tail = NULL;
	subscribe_head = subscribe_tail = NULL;
	pending_rule_head = pending_rule_tail = NULL;
	last_motion_time = last_motion_x = last_motion_y = 0;
	visible = auto_raise = sticky_still = record_history = true;
	randr_base = 0;
	exit_status = 0;
}

void setup(void)
{
	init();
	ewmh_init();
	screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
	if (screen == NULL)
		err("Can't acquire the default screen.\n");
	root = screen->root;
	register_events();

	screen_width = screen->width_in_pixels;
	screen_height = screen->height_in_pixels;
	root_depth = screen->root_depth;

	uint32_t mask = XCB_CW_EVENT_MASK;
	uint32_t values[] = {XCB_EVENT_MASK_POINTER_MOTION};
	motion_recorder = xcb_generate_id(dpy);
	xcb_create_window(dpy, XCB_COPY_FROM_PARENT, motion_recorder, root, 0, 0, screen_width, screen_height, 0, XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT, mask, values);

	xcb_atom_t net_atoms[] = {ewmh->_NET_SUPPORTED,
	                          ewmh->_NET_SUPPORTING_WM_CHECK,
	                          ewmh->_NET_DESKTOP_NAMES,
	                          ewmh->_NET_NUMBER_OF_DESKTOPS,
	                          ewmh->_NET_CURRENT_DESKTOP,
	                          ewmh->_NET_CLIENT_LIST,
	                          ewmh->_NET_ACTIVE_WINDOW,
	                          ewmh->_NET_CLOSE_WINDOW,
	                          ewmh->_NET_WM_DESKTOP,
	                          ewmh->_NET_WM_STATE,
	                          ewmh->_NET_WM_STATE_FULLSCREEN,
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
	ewmh_set_supporting(motion_recorder);

#define GETATOM(a) \
	get_atom(#a, &a);
	GETATOM(WM_DELETE_WINDOW)
	GETATOM(WM_TAKE_FOCUS)
	GETATOM(_BSPWM_FLOATING_WINDOW)
#undef GETATOM

	const xcb_query_extension_reply_t *qep = xcb_get_extension_data(dpy, &xcb_randr_id);
	if (qep->present && import_monitors()) {
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
				monitor_t *m = add_monitor(rect);
				add_desktop(m, make_desktop(NULL));
			}
			free(xsq);
		} else {
			warn("Xinerama is inactive.\n");
			xcb_rectangle_t rect = (xcb_rectangle_t) {0, 0, screen_width, screen_height};
			monitor_t *m = add_monitor(rect);
			add_desktop(m, make_desktop(NULL));
		}
	}

	ewmh_update_number_of_desktops();
	ewmh_update_desktop_names();
	ewmh_update_current_desktop();
	frozen_pointer = make_pointer_state();
	xcb_get_input_focus_reply_t *ifo = xcb_get_input_focus_reply(dpy, xcb_get_input_focus(dpy), NULL);
	if (ifo != NULL && (ifo->focus == XCB_INPUT_FOCUS_POINTER_ROOT || ifo->focus == XCB_NONE))
		clear_input_focus();
	free(ifo);
}

void register_events(void)
{
	uint32_t values[] = {ROOT_EVENT_MASK};
	xcb_generic_error_t *e = xcb_request_check(dpy, xcb_change_window_attributes_checked(dpy, root, XCB_CW_EVENT_MASK, values));
	if (e != NULL) {
		xcb_disconnect(dpy);
		err("Another window manager is already running.\n");
	}
}

void cleanup(void)
{
	while (mon_head != NULL)
		remove_monitor(mon_head);
	while (rule_head != NULL)
		remove_rule(rule_head);
	while (stack_head != NULL)
		remove_stack(stack_head);
	while (subscribe_head != NULL)
		remove_subscriber(subscribe_head);
	while (pending_rule_head != NULL)
		remove_pending_rule(pending_rule_head);
	empty_history();
	free(frozen_pointer);
}

void put_status(void)
{
	subscriber_list_t *sb = subscribe_head;
	char *status = get_status();
	while (sb != NULL) {
		subscriber_list_t *next = sb->next;
		feed_subscriber(sb, status);
		sb = next;
	}
}

char *get_status(void)
{
	static char rsp[BUFSIZ] = "";
	int length = 0;
	length += snprintf(rsp, BUFSIZ, "%s", status_prefix);
	bool urgent = false;
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		length += snprintf(rsp+length, BUFSIZ-length, "%c%s:", (mon == m ? 'M' : 'm'), m->name);
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next, urgent = false) {
			for (node_t *n = first_extrema(d->root); n != NULL && !urgent; n = next_leaf(n, d->root))
				urgent |= n->client->urgent;
			char c = (urgent ? 'u' : (d->root == NULL ? 'f' : 'o'));
			if (m->desk == d)
				c = toupper(c);
			length += snprintf(rsp+length, BUFSIZ-length, "%c%s:", c, d->name);
		}
	}
	if (mon != NULL && mon->desk != NULL)
		length += snprintf(rsp+length, BUFSIZ-length, "L%s", (mon->desk->layout == LAYOUT_TILED ? "tiled" : "monocle"));
	snprintf(rsp+length, BUFSIZ-length, "%s", "\n");
	return rsp;
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

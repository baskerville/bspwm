#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>

#define TEST_WINDOW_IC  "test\0Test"

bool get_atom(xcb_connection_t *dpy, char *name, xcb_atom_t *atom)
{
	bool ret = true;
	xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(dpy, xcb_intern_atom(dpy, 0, strlen(name), name), NULL);
	if (reply != NULL) {
		*atom = reply->atom;
	} else {
		ret = false;
	}
	free(reply);
	return ret;
}

void check_request(xcb_connection_t *dpy, xcb_void_cookie_t cookie, char *msg)
{
	xcb_generic_error_t *err = xcb_request_check(dpy, cookie);
	if (err != NULL) {
		fprintf(stderr, "%s: error code: %u.\n", msg, err->error_code);
		xcb_disconnect(dpy);
		exit(-1);
	}
}

xcb_gc_t get_font_gc(xcb_connection_t *dpy, xcb_window_t win, const char *font_name)
{
	xcb_void_cookie_t ck;
	xcb_font_t font = xcb_generate_id(dpy);
	ck = xcb_open_font_checked(dpy, font, strlen(font_name), font_name);
	check_request(dpy, ck, "Can't open font");
	xcb_gcontext_t gc = xcb_generate_id(dpy);
	uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
	uint32_t values[] = {0xffcccccc, 0xff111111, font};
	xcb_create_gc(dpy, gc, win, mask, values);
	xcb_close_font(dpy, font);
	return gc;
}

void render_text(xcb_connection_t *dpy, xcb_window_t win, int16_t x, int16_t y)
{
	char id[11];
	xcb_void_cookie_t ck;
	snprintf(id, sizeof(id), "0x%08X", win);
	xcb_gcontext_t gc = get_font_gc(dpy, win, "-*-fixed-medium-*-*-*-18-*-*-*-*-*-*-*");
	/* Doesn't work without _checked */
	ck = xcb_image_text_8_checked(dpy, strlen(id), win, gc, x, y, id);
	check_request(dpy, ck, "Can't draw text");
	xcb_free_gc(dpy, gc);
}


int main(int argc, char **argv)
{
	char *wm_class = TEST_WINDOW_IC;
	size_t wm_class_len = sizeof (TEST_WINDOW_IC);
	bool will_free_wm_class = false;

	// test instance-name class-name
	if (argc > 2) {
		will_free_wm_class = true;
		size_t len1 = strlen(argv[1]);
		size_t len2 = strlen(argv[2]);
		// 2 null terminators
		wm_class_len = len1 + len2 + 2;
		wm_class = malloc(wm_class_len);

		if (!wm_class) return 1;

		memcpy(wm_class, argv[1], len1 + 1);
		memcpy(wm_class + len1 + 1, argv[2], len2 + 1);
	}

	xcb_connection_t *dpy = xcb_connect(NULL, NULL);
	if (dpy == NULL) {
		fprintf(stderr, "Can't connect to X.\n");
		return EXIT_FAILURE;
	}
	xcb_atom_t WM_PROTOCOLS, WM_DELETE_WINDOW;
	if (!get_atom(dpy, "WM_PROTOCOLS", &WM_PROTOCOLS) ||
	    !get_atom(dpy, "WM_DELETE_WINDOW", &WM_DELETE_WINDOW)) {
		fprintf(stderr, "Can't get required atoms.\n");
		xcb_disconnect(dpy);
		return EXIT_FAILURE;
	}
	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
	if (screen == NULL) {
		fprintf(stderr, "Can't get current screen.\n");
		xcb_disconnect(dpy);
		return EXIT_FAILURE;
	}
	xcb_window_t win = xcb_generate_id(dpy);
	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t values[] = {0xff111111, XCB_EVENT_MASK_EXPOSURE};
	xcb_create_window(dpy, XCB_COPY_FROM_PARENT, win, screen->root, 0, 0, 320, 240, 2,
	                  XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_COPY_FROM_PARENT, mask, values);
	xcb_icccm_set_wm_class(dpy, win, wm_class_len, wm_class);
	xcb_map_window(dpy, win);
	xcb_flush(dpy);
	xcb_generic_event_t *evt;
	bool running = true;
	while (running && (evt = xcb_wait_for_event(dpy)) != NULL) {
		uint8_t rt = XCB_EVENT_RESPONSE_TYPE(evt);
		if (rt == XCB_CLIENT_MESSAGE)  {
			xcb_client_message_event_t *cme = (xcb_client_message_event_t *) evt;
			if (cme->type == WM_PROTOCOLS && cme->data.data32[0] == WM_DELETE_WINDOW) {
				running = false;
			}
		} else if (rt == XCB_EXPOSE) {
			render_text(dpy, win, 12, 24);
		}
		free(evt);
	}
	xcb_destroy_window(dpy, win);
	xcb_disconnect(dpy);

	if (will_free_wm_class) {
		free(wm_class);
	}

	return EXIT_SUCCESS;
}

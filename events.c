#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "helpers.h"
#include "types.h"
#include "bspwm.h"
#include "utils.h"
#include "events.h"
#include "tree.h"

void handle_event(xcb_generic_event_t *evt)
{
    switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
        case XCB_MAP_REQUEST:
            PUTS("received a map request\n");
            map_request(evt);
            break;
        case XCB_CONFIGURE_REQUEST:
            PUTS("received a configure request\n");
            break;
        case XCB_UNGRAB_KEY:
            /* PUTS("ungrab key received"); */
            break;
        case XCB_KEY_PRESS:
            PUTS("keypress received");
            break;
        case XCB_KEY_RELEASE:
            PUTS("keyrelease received");
            break;
        case XCB_BUTTON_PRESS:
        case XCB_BUTTON_RELEASE:
            PUTS("button event");
            break;
        default:
            PRINTF("received event %i\n", XCB_EVENT_RESPONSE_TYPE(evt));
    }
}

void map_request(xcb_generic_event_t *evt)
{
    xcb_map_request_event_t *e = (xcb_map_request_event_t *) evt;
    xcb_get_window_attributes_reply_t  *wa;
    xcb_window_t win = e->window;
    wa = xcb_get_window_attributes_reply(dpy, xcb_get_window_attributes(dpy, win), NULL);

    if ((wa != NULL && wa->override_redirect) || win_to_node(win) != NULL)
        return;

    free(wa);
    bool takes_focus = true;
    client_t *c = make_client();
    c->window = win;
    num_clients++;
    node_t *focus = desk->focus;
    node_t *birth = make_node();
    birth->client = c;

    if (focus == NULL) {
        desk->root = birth;
    } else {
        node_t *dad = make_node();
        node_t *fopar = focus->parent;
        birth->parent = dad;
        switch (split_mode) {
            case MODE_AUTOMATIC:
                if (fopar == NULL) {
                    dad->first_child = birth;
                    dad->second_child = focus;
                    dad->split_type = TYPE_VERTICAL;
                    focus->parent = dad;
                    desk->root = dad;
                } else {
                    node_t *grandpa = fopar->parent;
                    dad->parent = grandpa;
                    if (grandpa != NULL) {
                        if (is_first_child(fopar))
                            grandpa->first_child = dad;
                        else
                            grandpa->second_child = dad;
                    }
                    dad->split_type = fopar->split_type;
                    dad->split_ratio = fopar->split_ratio;
                    fopar->parent = dad;
                    if (is_first_child(focus)) {
                        dad->first_child = birth;
                        dad->second_child = fopar;
                        rotate_tree(fopar, ROTATE_CLOCK_WISE);
                    } else {
                        dad->first_child = fopar;
                        dad->second_child = birth;
                        rotate_tree(fopar, ROTATE_COUNTER_CW);
                    }
                }
                break;
            case MODE_MANUAL:
                focus->parent = dad;
                switch (split_dir) {
                    case DIR_LEFT:
                    dad->split_type = TYPE_VERTICAL;
                    dad->first_child = birth;
                    dad->second_child = focus;
                    break;
                    case DIR_RIGHT:
                    dad->split_type = TYPE_VERTICAL;
                    dad->first_child = focus;
                    dad->second_child = birth;
                    break;
                    case DIR_UP:
                    dad->split_type = TYPE_HORIZONTAL;
                    dad->first_child = birth;
                    dad->second_child = focus;
                    break;
                    case DIR_DOWN:
                    dad->split_type = TYPE_HORIZONTAL;
                    dad->first_child = focus;
                    dad->second_child = birth;
                    break;
                }
                if (desk->root == focus)
                    desk->root = dad;
                split_mode = MODE_AUTOMATIC;
                break;
        }
        if (takes_focus)
            desk->focus = birth;
    }
}

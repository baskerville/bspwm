#ifndef _EVENTS_H
#define _EVENTS_H

#include <xcb/xcb.h>
#include <xcb/xcb_event.h>

uint8_t randr_base;
uint16_t last_motion_x, last_motion_y;
xcb_timestamp_t last_motion_time;

void handle_event(xcb_generic_event_t *evt);
void map_request(xcb_generic_event_t *evt);
void configure_request(xcb_generic_event_t *evt);
void destroy_notify(xcb_generic_event_t *evt);
void unmap_notify(xcb_generic_event_t *evt);
void property_notify(xcb_generic_event_t *evt);
void client_message(xcb_generic_event_t *evt);
void focus_in(xcb_generic_event_t *evt);
void enter_notify(xcb_generic_event_t *evt);
void motion_notify(xcb_generic_event_t *evt);
void handle_state(monitor_t *m, desktop_t *d, node_t *n, xcb_atom_t state, unsigned int action);

#endif

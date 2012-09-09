#ifndef _BSPS_H
#define _BSPS_H

void setup(void);
void register_events(void);
void handle_signal(int);
void handle_event(xcb_generic_event_t *);
void update_window_title(void);
void update_desktop_name(void);
void output_infos(void);
double text_width(char *);

#endif

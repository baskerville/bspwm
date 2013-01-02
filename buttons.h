#ifndef _BUTTONS_H
#define _BUTTONS_H

#define XK_Num_Lock     0xff7f
#define XK_Caps_Lock    0xffe5
#define XK_Scroll_Lock  0xff14

uint16_t num_lock;
uint16_t caps_lock;
uint16_t scroll_lock;

xcb_key_symbols_t *symbols;

void grab_buttons(void);
void ungrab_buttons(void);
void get_lock_fields(void);
int16_t modfield_from_keysym(xcb_keysym_t);

#endif

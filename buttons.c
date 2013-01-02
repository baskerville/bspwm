#include <stdlib.h>
#include <xcb/xcb_keysyms.h>
#include "settings.h"
#include "bspwm.h"
#include "helpers.h"
#include "types.h"
#include "buttons.h"

void grab_buttons(void)
{
#define GRAB(b, m)  xcb_grab_button(dpy, false, root, XCB_EVENT_MASK_BUTTON_PRESS, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, b, m)
    uint8_t buttons[] = {XCB_BUTTON_INDEX_1, XCB_BUTTON_INDEX_2, XCB_BUTTON_INDEX_3};
    for (unsigned int i = 0; i < LENGTH(buttons); i++) {
        uint8_t button = buttons[i];
        GRAB(button, button_modifier);
        if (num_lock != XCB_NO_SYMBOL && caps_lock != XCB_NO_SYMBOL && scroll_lock != XCB_NO_SYMBOL)
            GRAB(button, button_modifier | num_lock | caps_lock | scroll_lock);
        if (num_lock != XCB_NO_SYMBOL && caps_lock != XCB_NO_SYMBOL)
            GRAB(button, button_modifier | num_lock | caps_lock);
        if (caps_lock != XCB_NO_SYMBOL && scroll_lock != XCB_NO_SYMBOL)
            GRAB(button, button_modifier | caps_lock | scroll_lock);
        if (num_lock != XCB_NO_SYMBOL && scroll_lock != XCB_NO_SYMBOL)
            GRAB(button, button_modifier | num_lock | scroll_lock);
        if (num_lock != XCB_NO_SYMBOL)
            GRAB(button, button_modifier | num_lock);
        if (caps_lock != XCB_NO_SYMBOL)
            GRAB(button, button_modifier | caps_lock);
        if (scroll_lock != XCB_NO_SYMBOL)
            GRAB(button, button_modifier | scroll_lock);
    }
#undef GRAB
}

void ungrab_buttons(void)
{
    xcb_ungrab_button(dpy, XCB_BUTTON_INDEX_ANY, root, XCB_MOD_MASK_ANY);
}

void get_lock_fields(void)
{
    num_lock = modfield_from_keysym(XK_Num_Lock);
    caps_lock = XCB_MOD_MASK_LOCK;
    scroll_lock = modfield_from_keysym(XK_Scroll_Lock);
    PRINTF("lock fields %u %u %u\n", num_lock, caps_lock, scroll_lock);
}

int16_t modfield_from_keysym(xcb_keysym_t keysym)
{
    uint16_t modfield = 0;
    xcb_keycode_t *keycodes = NULL, *mod_keycodes = NULL;
    xcb_get_modifier_mapping_reply_t *reply = NULL;
    if ((keycodes = xcb_key_symbols_get_keycode(symbols, keysym)) != NULL) {
        if ((reply = xcb_get_modifier_mapping_reply(dpy, xcb_get_modifier_mapping(dpy), NULL)) != NULL) {
            if ((mod_keycodes = xcb_get_modifier_mapping_keycodes(reply)) != NULL) {
                unsigned int num_mod = xcb_get_modifier_mapping_keycodes_length(reply) / reply->keycodes_per_modifier;
                for (unsigned int i = 0; i < num_mod; i++) {
                    for (unsigned int j = 0; j < reply->keycodes_per_modifier; j++) {
                        xcb_keycode_t mk = mod_keycodes[i * reply->keycodes_per_modifier + j];
                        if (mk == XCB_NO_SYMBOL)
                            continue;
                        for (xcb_keycode_t *k = keycodes; *k != XCB_NO_SYMBOL; k++)
                            if (*k == mk)
                                modfield |= (1 << i);
                    }
                }

            }
        }
    }
    free(keycodes);
    free(reply);
    return modfield;
}

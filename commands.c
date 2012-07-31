#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "utils.h"
#include "main.h"
#include "commands.h"

void quit(void)
{
    running = false;
}

VERSION = 0.01

CC      = gcc
LIBS    = `pkg-config --libs xcb xcb-ewmh xcb-icccm lua`
CFLAGS  = -g -std=c99 -pedantic -Wall -Wextra
LDFLAGS = $(LIBS)

PREFIX    ?= /usr/local
BINPREFIX = $(PREFIX)/bin

WM_SRC = bspwm.c events.c luautils.c messages.c ewmh.c settings.c utils.c tree.c types.c rules.c window.c
CL_SRC = bspc.c

WM_OBJ = $(WM_SRC:.c=.o)
CL_OBJ = $(CL_SRC:.c=.o)

all: options clean bspwm bspc

options:
#	@echo "bspwm build options:"
#	@echo "CC      = $(CC)"
#	@echo "CFLAGS  = $(CFLAGS)"
#	@echo "LDFLAGS = $(LDFLAGS)"
#	@echo "PREFIX  = $(PREFIX)"

.c.o:
#	@echo "CC $<"
	@$(CC) $(CFLAGS) -DVERSION=\"$(VERSION)\" -c -o $@ $<

bspwm:  $(WM_OBJ)
#	@echo CC -o $@
	@$(CC) -o $@ $(WM_OBJ) $(LDFLAGS)

bspc:   $(CL_OBJ)
#	@echo CC -o $@
	@$(CC) -o $@ $(CL_OBJ) $(LDFLAGS)

clean:
#	@echo "cleaning"
	@rm -f $(WM_OBJ) $(CL_OBJ) bsp{wm,c}

install: all
#	@echo "installing executable files to $(DESTDIR)$(BINPREFIX)"
	@install -D -m 755 bspwm $(DESTDIR)$(BINPREFIX)/bspwm
	@install -D -m 755 bspc $(DESTDIR)$(BINPREFIX)/bspc

uninstall:
#	@echo "removing executable files from $(DESTDIR)$(BINPREFIX)"
	@rm -f $(DESTDIR)$(BINPREFIX)/bsp{wm,c}

.PHONY: all options clean install uninstall

VERSION = 0.5

CC      = gcc
LIBS    = -lm -lxcb -lxcb-icccm -lxcb-ewmh -lxcb-xinerama
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -I$(PREFIX)/include
CFLAGS  += -D_POSIX_C_SOURCE=200112L -DVERSION=\"$(VERSION)\"
LDFLAGS = -L$(PREFIX)/lib

PREFIX    ?= /usr/local
BINPREFIX = $(PREFIX)/bin
MANPREFIX = $(PREFIX)/share/man

WM_SRC = bspwm.c events.c messages.c ewmh.c settings.c helpers.c tree.c types.c rules.c window.c
WM_OBJ = $(WM_SRC:.c=.o)
CL_SRC = bspc.c helpers.c
CL_OBJ = $(CL_SRC:.c=.o)

all: CFLAGS += -Os
all: LDFLAGS += -s
all: bspwm bspc

debug: CFLAGS += -O0 -g -DDEBUG
debug: bspwm bspc

include Incgraph

$(WM_OBJ) $(CL_OBJ): Makefile

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

bspwm: $(WM_OBJ)
	$(CC) -o $@ $(WM_OBJ) $(LDFLAGS) $(LIBS)

bspc: $(CL_OBJ)
	$(CC) -o $@ $(CL_OBJ) $(LDFLAGS) $(LIBS)

install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp bsp{wm,c} "$(DESTDIR)$(BINPREFIX)"
	chmod 755 "$(DESTDIR)$(BINPREFIX)"/bsp{wm,c}
	mkdir -p "$(DESTDIR)$(MANPREFIX)"/man1
	cp bspwm.1 "$(DESTDIR)$(MANPREFIX)"/man1
	chmod 644 "$(DESTDIR)$(MANPREFIX)"/man1/bspwm.1

uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)"/bsp{wm,c}
	rm -f "$(DESTDIR)$(MANPREFIX)"/man1/bspwm.1

clean:
	rm -f $(WM_OBJ) $(CL_OBJ) bsp{wm,c}

.PHONY: all debug clean install uninstall

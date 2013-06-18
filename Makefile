VERSION = 0.7

CC      = gcc
LIBS    = -lm -lxcb -lxcb-icccm -lxcb-ewmh -lxcb-randr
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -I$(PREFIX)/include
CFLAGS  += -D_POSIX_C_SOURCE=200112L -DVERSION=\"$(VERSION)\"
LDFLAGS = -L$(PREFIX)/lib

PREFIX    ?= /usr/local
BINPREFIX = $(PREFIX)/bin
MANPREFIX = $(PREFIX)/share/man
CPLPREFIX = $(PREFIX)/share/bash-completion/completions

WM_SRC = bspwm.c events.c messages.c ewmh.c settings.c helpers.c tree.c types.c rules.c window.c
WM_OBJ = $(WM_SRC:.c=.o)
CL_SRC = bspc.c helpers.c
CL_OBJ = $(CL_SRC:.c=.o)

all: CFLAGS += -Os
all: LDFLAGS += -s
all: bspwm bspc

debug: CFLAGS += -O0 -g -DDEBUG
debug: bspwm bspc

include Sourcedeps

$(WM_OBJ) $(CL_OBJ): Makefile

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

bspwm: $(WM_OBJ)
	$(CC) -o $@ $(WM_OBJ) $(LDFLAGS) $(LIBS)

bspc: $(CL_OBJ)
	$(CC) -o $@ $(CL_OBJ) $(LDFLAGS) $(LIBS)

install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp -p bsp{wm,c} "$(DESTDIR)$(BINPREFIX)"
	mkdir -p "$(DESTDIR)$(MANPREFIX)"/man1
	cp -Pp bsp{wm,c}.1 "$(DESTDIR)$(MANPREFIX)"/man1
	mkdir -p "$(DESTDIR)$(CPLPREFIX)"
	cp -p bash_completion "$(DESTDIR)$(CPLPREFIX)"/bspc

uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)"/bsp{wm,c}
	rm -f "$(DESTDIR)$(MANPREFIX)"/man1/bsp{wm,c}.1
	rm -f "$(DESTDIR)$(CPLPREFIX)"/bspc

clean:
	rm -f $(WM_OBJ) $(CL_OBJ) bsp{wm,c}

.PHONY: all debug clean install uninstall

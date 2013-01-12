VERSION = 0.4

CC      = gcc
LIBS    = -lm -lxcb -lxcb-icccm -lxcb-ewmh -lxcb-xinerama
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -I$(PREFIX)/include
CFLAGS  += -D_POSIX_C_SOURCE=200112L -DVERSION=\"$(VERSION)\"
LDFLAGS = -L$(PREFIX)/lib

PREFIX    ?= /usr/local
BINPREFIX = $(PREFIX)/bin
MANPREFIX = $(PREFIX)/share/man

WM_SRC = bspwm.c events.c messages.c ewmh.c settings.c helpers.c tree.c types.c rules.c window.c
CL_SRC = bspc.c helpers.c

WM_OBJ = $(WM_SRC:.c=.o)
CL_OBJ = $(CL_SRC:.c=.o)

all: CFLAGS += -Os
all: LDFLAGS += -s
all: options bspwm bspc

debug: CFLAGS += -O0 -g -DDEBUG
debug: options bspwm bspc

options:
	@echo "bspwm build options:"
	@echo "CC      = $(CC)"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "PREFIX  = $(PREFIX)"

.c.o:
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

bspwm: $(WM_OBJ)
	@echo CC -o $@
	@$(CC) -o $@ $(WM_OBJ) $(LDFLAGS) $(LIBS)

bspc: $(CL_OBJ)
	@echo CC -o $@
	@$(CC) -o $@ $(CL_OBJ) $(LDFLAGS) $(LIBS)

clean:
	@echo "cleaning"
	@rm -f $(WM_OBJ) $(CL_OBJ) bsp{wm,c}

install:
	@echo "installing executable files to $(DESTDIR)$(BINPREFIX)"
	@mkdir -p "$(DESTDIR)$(BINPREFIX)"
	@cp -t "$(DESTDIR)$(BINPREFIX)" bsp{wm,c}
	@chmod 755 "$(DESTDIR)$(BINPREFIX)"/bsp{wm,c}
	@echo "installing manual page to $(DESTDIR)$(MANPREFIX)/man1"
	@mkdir -p "$(DESTDIR)$(MANPREFIX)"/man1
	@cp -t "$(DESTDIR)$(MANPREFIX)"/man1 bspwm.1
	@chmod 644 "$(DESTDIR)$(MANPREFIX)"/man1/bspwm.1

uninstall:
	@echo "removing executable files from $(DESTDIR)$(BINPREFIX)"
	@rm -f "$(DESTDIR)$(BINPREFIX)"/bsp{wm,c}
	@echo "removing manual page from $(DESTDIR)$(MANPREFIX)/man1"
	@rm -f "$(DESTDIR)$(MANPREFIX)"/man1/bspwm.1

.PHONY: all debug options clean install uninstall

VERSION = 0.8.9

CC      ?= gcc
CP      ?= cp --remove-destination -P -T
LIBS     = -lm -lxcb -lxcb-icccm -lxcb-ewmh -lxcb-randr -lxcb-xinerama
CFLAGS  += -std=c99 -pedantic -Wall -Wextra -I$(PREFIX)/include
CFLAGS  += -D_POSIX_C_SOURCE=200112L -DVERSION=\"$(VERSION)\"
LDFLAGS += -L$(PREFIX)/lib

PREFIX   ?= /usr/local
BINPREFIX = $(PREFIX)/bin
MANPREFIX = $(PREFIX)/share/man
BASHCPL = $(PREFIX)/share/bash-completion/completions
ZSHCPL = $(PREFIX)/share/zsh/site-functions

WM_SRC = bspwm.c helpers.c settings.c monitor.c desktop.c tree.c stack.c history.c \
	 events.c pointer.c window.c messages.c query.c restore.c rule.c ewmh.c subscribe.c
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
	$(CC) $(CFLAGS) $(OPTFLAGS) -c -o $@ $<

bspwm: $(WM_OBJ)
	$(CC) -o $@ $(WM_OBJ) $(LDFLAGS) $(LIBS)

bspc: $(CL_OBJ)
	$(CC) -o $@ $(CL_OBJ) $(LDFLAGS) $(LIBS)

install:
	mkdir -m755 -p "$(DESTDIR)$(BINPREFIX)"
	$(CP) bspwm "$(DESTDIR)$(BINPREFIX)"/bspwm
	$(CP) bspc "$(DESTDIR)$(BINPREFIX)"/bspc
	chmod 755 "$(DESTDIR)$(BINPREFIX)"/bspc \
			  "$(DESTDIR)$(BINPREFIX)"/bspwm
	mkdir -m755 -p "$(DESTDIR)$(MANPREFIX)"/man1
	$(CP) doc/bspwm.1 "$(DESTDIR)$(MANPREFIX)"/man1/bspwm.1
	$(CP) doc/bspc.1 "$(DESTDIR)$(MANPREFIX)"/man1/bspc.1
	mkdir -m755 -p "$(DESTDIR)$(BASHCPL)"
	$(CP) contrib/bash_completion "$(DESTDIR)$(BASHCPL)"/bspc
	mkdir -m755 -p "$(DESTDIR)$(ZSHCPL)"
	$(CP) contrib/zsh_completion "$(DESTDIR)$(ZSHCPL)"/_bspc
	chmod 644 "$(DESTDIR)$(MANPREFIX)"/man1/bspwm.1 \
			  "$(DESTDIR)$(MANPREFIX)"/man1/bspc.1 \
			  "$(DESTDIR)$(ZSHCPL)"/_bspc \
			  "$(DESTDIR)$(BASHCPL)"/bspc

uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)"/bspwm
	rm -f "$(DESTDIR)$(BINPREFIX)"/bspc
	rm -f "$(DESTDIR)$(MANPREFIX)"/man1/bspwm.1
	rm -f "$(DESTDIR)$(MANPREFIX)"/man1/bspc.1
	rm -f "$(DESTDIR)$(BASHCPL)"/bspc
	rm -f "$(DESTDIR)$(ZSHCPL)"/_bspc

doc:
	a2x -v -d manpage -f manpage -a revnumber=$(VERSION) doc/bspwm.1.txt

clean:
	rm -f $(WM_OBJ) $(CL_OBJ) bspwm bspc

.PHONY: all debug install uninstall doc deps clean

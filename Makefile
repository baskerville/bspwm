VERCMD  ?= git describe --tags 2> /dev/null
VERSION := $(shell $(VERCMD) || cat VERSION)

CPPFLAGS += -D_POSIX_C_SOURCE=200809L -DVERSION=\"$(VERSION)\"
CFLAGS   += -std=c99 -pedantic -Wall -Wextra -DJSMN_STRICT
LDFLAGS  ?=
LDLIBS    = $(LDFLAGS) -lm -lxcb -lxcb-util -lxcb-keysyms -lxcb-icccm -lxcb-ewmh -lxcb-randr -lxcb-xinerama -lxcb-shape

PREFIX    ?= /usr/local
BINPREFIX ?= $(PREFIX)/bin
MANPREFIX ?= $(PREFIX)/share/man
DOCPREFIX ?= $(PREFIX)/share/doc/bspwm
BASHCPL   ?= $(PREFIX)/share/bash-completion/completions
FISHCPL   ?= $(PREFIX)/share/fish/vendor_completions.d
ZSHCPL    ?= $(PREFIX)/share/zsh/site-functions

MD_DOCS    = README.md doc/CHANGELOG.md doc/CONTRIBUTING.md doc/INSTALL.md doc/MISC.md doc/TODO.md
XSESSIONS ?= $(PREFIX)/share/xsessions

WM_SRC   = bspwm.c helpers.c geometry.c jsmn.c settings.c monitor.c desktop.c tree.c stack.c history.c \
	 events.c pointer.c window.c messages.c parse.c query.c restore.c rule.c ewmh.c subscribe.c
WM_OBJ  := $(WM_SRC:.c=.o)
CLI_SRC  = bspc.c helpers.c
CLI_OBJ := $(CLI_SRC:.c=.o)

all: bspwm bspc

debug: CFLAGS += -O0 -g
debug: bspwm bspc

VPATH=src

include Sourcedeps

$(WM_OBJ) $(CLI_OBJ): Makefile

bspwm: $(WM_OBJ)

bspc: $(CLI_OBJ)

install:
	install -d "$(DESTDIR)$(BINPREFIX)"
	install -p bspwm "$(DESTDIR)$(BINPREFIX)"
	install -p bspc "$(DESTDIR)$(BINPREFIX)"
	install -d "$(DESTDIR)$(MANPREFIX)"/man1
	install -p -m 644 doc/bspwm.1 "$(DESTDIR)$(MANPREFIX)"/man1
	(cd "$(DESTDIR)$(MANPREFIX)"/man1; ln -s bspwm.1 bspc.1)
	install -d "$(DESTDIR)$(BASHCPL)"
	install -p -m 644 contrib/bash_completion "$(DESTDIR)$(BASHCPL)"/bspc
	install -d "$(DESTDIR)$(FISHCPL)"
	install -p -m 644 contrib/fish_completion "$(DESTDIR)$(FISHCPL)"/bspc.fish
	install -d "$(DESTDIR)$(ZSHCPL)"
	install -p -m 644 contrib/zsh_completion "$(DESTDIR)$(ZSHCPL)"/_bspc
	install -d "$(DESTDIR)$(DOCPREFIX)"
	install -p -m 644 $(MD_DOCS) "$(DESTDIR)$(DOCPREFIX)"
	find examples -type f -print0 | xargs -0 -I% install -D -p % "$(DESTDIR)$(DOCPREFIX)"/%
	install -d "$(DESTDIR)$(XSESSIONS)"
	install -p -m 644 contrib/freedesktop/bspwm.desktop "$(DESTDIR)$(XSESSIONS)"

uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)"/bspwm
	rm -f "$(DESTDIR)$(BINPREFIX)"/bspc
	rm -f "$(DESTDIR)$(MANPREFIX)"/man1/bspwm.1
	rm -f "$(DESTDIR)$(MANPREFIX)"/man1/bspc.1
	rm -f "$(DESTDIR)$(BASHCPL)"/bspc
	rm -f "$(DESTDIR)$(FISHCPL)"/bspc.fish
	rm -f "$(DESTDIR)$(ZSHCPL)"/_bspc
	rm -rf "$(DESTDIR)$(DOCPREFIX)"
	rm -f "$(DESTDIR)$(XSESSIONS)"/bspwm.desktop

doc:
	a2x -v -d manpage -f manpage -a revnumber=$(VERSION) doc/bspwm.1.asciidoc

clean:
	rm -f $(WM_OBJ) $(CLI_OBJ) bspwm bspc

.PHONY: all debug install uninstall doc clean

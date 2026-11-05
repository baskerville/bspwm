VERCMD  ?= git describe --tags 2> /dev/null
VERSION := $(shell $(VERCMD) || cat VERSION)

CPPFLAGS += -D_POSIX_C_SOURCE=200809L -DVERSION=\"$(VERSION)\"
CFLAGS   += -std=c23 -pedantic -Wall -Wextra -Wvla -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wnull-dereference -Wstack-protector -fstack-protector-strong -fstack-clash-protection -fcf-protection -O2 -D_FORTIFY_SOURCE=3 -DJSMN_STRICT

# Core sources — display-server independent
CORE_SRC = bspwm.c helpers.c geometry.c jsmn.c settings.c monitor.c desktop.c tree.c stack.c history.c \
	 ipc.c messages.c parse.c query.c restore.c rule.c rule_match.c subscribe.c keybind.c snap.c

# X11 backend sources
BACKEND_SRC  = backend_x11.c events.c pointer.c window.c ewmh.c magnet.c
BACKEND_LIBS = -lxcb -lxcb-util -lxcb-keysyms -lxcb-icccm -lxcb-ewmh -lxcb-randr -lxcb-xinerama -lxcb-shape -lxkbcommon
WM_BIN       = bspwm
SESSION_FILE = contrib/freedesktop/bspwm.desktop
SESSION_DIR  = $(XSESSIONS)

LDFLAGS  += -Wl,-z,relro,-z,now
LDLIBS    = -lm $(BACKEND_LIBS)

PREFIX    ?= /usr/local
BINPREFIX ?= $(PREFIX)/bin
MANPREFIX ?= $(PREFIX)/share/man
DOCPREFIX ?= $(PREFIX)/share/doc/bspwm
BASHCPL   ?= $(PREFIX)/share/bash-completion/completions
FISHCPL   ?= $(PREFIX)/share/fish/vendor_completions.d
ZSHCPL    ?= $(PREFIX)/share/zsh/site-functions

MD_DOCS    = README.md doc/CHANGELOG.md doc/CONTRIBUTING.md doc/INSTALL.md doc/MISC.md doc/TODO.md
XSESSIONS  ?= $(PREFIX)/share/xsessions
CONFPREFIX ?= $(DESTDIR)$(HOME)/.config

# Header deps come from the compiler.
OBJDIR   = build/x11
WM_SRC   = $(CORE_SRC) $(BACKEND_SRC)
WM_OBJ  := $(addprefix $(OBJDIR)/,$(WM_SRC:.c=.o))
CLI_SRC  = bspc.c helpers.c
CLI_OBJ := $(addprefix $(OBJDIR)/,$(CLI_SRC:.c=.o))

# bspc parses DISPLAY itself; it needs no X library.
CLI_LIBS =

all: $(WM_BIN) bspc

debug: CFLAGS += -Og -g
debug: $(WM_BIN) bspc

$(OBJDIR)/%.o: src/%.c Makefile
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(WM_OBJ:.o=.d) $(CLI_OBJ:.o=.d)

$(WM_BIN): $(WM_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

bspc: LDLIBS = $(CLI_LIBS)
bspc: $(CLI_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Installs the binary, session file, docs and completions.
install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp -pf $(WM_BIN) "$(DESTDIR)$(BINPREFIX)"
	cp -pf bspc "$(DESTDIR)$(BINPREFIX)"
	mkdir -p "$(DESTDIR)$(MANPREFIX)"/man1
	cp -p doc/bspwm.1 "$(DESTDIR)$(MANPREFIX)"/man1
	cp -Pp doc/bspc.1 "$(DESTDIR)$(MANPREFIX)"/man1
	mkdir -p "$(DESTDIR)$(BASHCPL)"
	cp -p contrib/bash_completion "$(DESTDIR)$(BASHCPL)"/bspc
	mkdir -p "$(DESTDIR)$(FISHCPL)"
	cp -p contrib/fish_completion "$(DESTDIR)$(FISHCPL)"/bspc.fish
	mkdir -p "$(DESTDIR)$(ZSHCPL)"
	cp -p contrib/zsh_completion "$(DESTDIR)$(ZSHCPL)"/_bspc
	mkdir -p "$(DESTDIR)$(DOCPREFIX)"
	cp -p $(MD_DOCS) "$(DESTDIR)$(DOCPREFIX)"
	mkdir -p "$(DESTDIR)$(DOCPREFIX)"/examples
	cp -pr examples/* "$(DESTDIR)$(DOCPREFIX)"/examples
	mkdir -p "$(DESTDIR)$(SESSION_DIR)"
	cp -p $(SESSION_FILE) "$(DESTDIR)$(SESSION_DIR)"

install_cfg:
	mkdir -p "$(CONFPREFIX)"/bspwm
	mkdir -p "$(CONFPREFIX)"/sxhkd
	cp -p examples/bspwmrc "$(CONFPREFIX)"/bspwm/bspwmrc
	cp -p examples/sxhkdrc "$(CONFPREFIX)"/sxhkd/sxhkdrc
	chmod +x "$(CONFPREFIX)"/bspwm/bspwmrc

uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)"/$(WM_BIN)
	rm -f "$(DESTDIR)$(BINPREFIX)"/bspc
	rm -f "$(DESTDIR)$(MANPREFIX)"/man1/bspwm.1
	rm -f "$(DESTDIR)$(MANPREFIX)"/man1/bspc.1
	rm -f "$(DESTDIR)$(BASHCPL)"/bspc
	rm -f "$(DESTDIR)$(FISHCPL)"/bspc.fish
	rm -f "$(DESTDIR)$(ZSHCPL)"/_bspc
	rm -rf "$(DESTDIR)$(DOCPREFIX)"
	rm -f "$(DESTDIR)$(SESSION_DIR)"/$(notdir $(SESSION_FILE))

test: $(WM_BIN) bspc
	@cd tests && $(MAKE) && ./run_headless

doc:
	a2x -v -d manpage -f manpage -a revnumber=$(VERSION) doc/bspwm.1.asciidoc

clean:
	rm -rf build bspwm bspc

.PHONY: all debug install install_cfg uninstall doc clean test

# CFLAGS=-Wall -g -pendantic

all:
	gcc -Wall -g -pedantic -llua -lxcb -o bspwm main.c utils.c settings.c luautils.c messages.c commands.c events.c

clean:
	rm -f bspwm

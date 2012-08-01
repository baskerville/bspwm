# CFLAGS=-Wall -g -pendantic

all:
	gcc -std=c99 -Wall -g -pedantic -llua -lxcb -o bspwm main.c utils.c settings.c luautils.c messages.c events.c

clean:
	rm -f bspwm

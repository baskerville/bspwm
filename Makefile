all:
	gcc -pedantic -Wall -llua -lxcb -o bspwm main.c utils.c settings.c luautils.c messages.c commands.c

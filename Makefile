all:
	gcc -pedantic -Wall -lxcb -o bspwm main.c utils.c

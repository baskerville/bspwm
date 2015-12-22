OUT      = test_window
CFLAGS  += -std=c99 -pedantic -Wall -Wextra
LDLIBS   = -lxcb -lxcb-icccm

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all: $(OUT)

clean:
	$(RM) $(OUT) $(OBJ)

.PHONY: all clean

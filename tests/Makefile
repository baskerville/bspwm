CFLAGS  += -std=c99 -D_POSIX_C_SOURCE=200809L -pedantic -Wall -Wextra

all: test_window test_rule_match test_magnet test_jsmn test_geometry

test_window: test_window.c
	$(CC) $(CFLAGS) -o $@ $< -lxcb -lxcb-icccm

test_rule_match: test_rule_match.c ../src/rule_match.c ../src/rule_match.h
	$(CC) $(CFLAGS) -std=c23 -o $@ test_rule_match.c ../src/rule_match.c

test_magnet: test_magnet.c ../src/magnet.c ../src/magnet.h
	$(CC) $(CFLAGS) -std=c23 -o $@ test_magnet.c ../src/magnet.c

test_jsmn: test_jsmn.c ../src/jsmn.c ../src/jsmn.h
	$(CC) $(CFLAGS) -std=c23 -DJSMN_STRICT -o $@ test_jsmn.c ../src/jsmn.c

test_geometry: test_geometry.c ../src/geometry.c ../src/geometry.h
	$(CC) $(CFLAGS) -std=c23 -o $@ test_geometry.c ../src/geometry.c

clean:
	$(RM) test_window test_rule_match test_magnet test_jsmn test_geometry *.o

.PHONY: all clean

#
# makefile for pong_game - like pong, including walls (paddle)
#

# this make-file has some subtle issues, mainly in regards to preprocessor
# stuff inside a few headers. so if you change preprocessor constants,
# do a full re-build lol.. it's not like it costs much time

CC = gcc
CC_FLAGS = -std=c99 -Wall -Wpedantic -Wextra -Werror -fsanitize=undefined
VALGRIND_FLAGS = --quiet --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=3 --error-exitcode=1

ALL_OBJECTS = game.o ball.o wall.o geometry.o

all: main

geometry.o: geometry.c geometry.h
	$(CC) $(CC_FLAGS) -c -o geometry.o geometry.c

wall.o: wall.c wall.h geometry.h
	$(CC) $(CC_FLAGS) -c -o wall.o wall.c -lncurses

ball.o: ball.c ball.h geometry.h
	$(CC) $(CC_FLAGS) -c -o ball.o ball.c -lncurses

game.o: game.c game.h geometry.h wall.h
	$(CC) $(CC_FLAGS) -c -o game.o game.c -lncurses

main: $(ALL_OBJECTS) main.c set_ticker.h
	$(CC) $(CC_FLAGS) -o pong_game main.c $(ALL_OBJECTS) -lncurses

# -c for compiling but not linking
# -g for debugging with gdb...

clean:
	rm -f *.o
	rm -f ./pong_game

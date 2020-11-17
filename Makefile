CFLAGS = -std=c99 -Wall -Wextra -Wno-cpp -Wno-unused-label -Wno-unused-command-line-argument \
		 -Wshadow -Wvla -Winline -finline-functions

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS += -g -O0 
else
	CFLAGS += -O2
endif

SAN ?= 0
ifeq ($(SAN), 1)
	CFLAGS += -fsanitize=address -fsanitize=leak -fsanitize=undefined \
				-fsanitize=bounds-strict -fsanitize=bounds-strict
endif

PROF ?= 0
ifeq ($(PROF), 1)
	CFLAGS += -g -pg
endif

SDL2=$(shell pkg-config --cflags --libs sdl2)

chip8: main.o chip8.o
	$(CC) -o chip8 $(CFLAGS) main.o chip8.o $(SDL2)

main.o: main.c util.h
	$(CC) -c $(CFLAGS) main.c util.h $(SDL2)

chip8.o: chip8.c chip8.h util.h
	$(CC) -c $(CFLAGS) chip8.c chip8.h util.h

clean:
	rm -f chip8 *.o *.gch

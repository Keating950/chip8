CFLAGS = -std=c99 -Wall -Wextra -Wno-cpp -Wno-unused-label -Wimplicit-fallthrough=2 -Wshadow -Wvla -Winline -finline-functions

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

SDL2=`pkg-config --cflags --libs sdl2`

chip8: main.o chip8.o
	$(CC) -o chip8 $(CFLAGS) $(SDL2) main.o chip8.o

main.o: main.c util.h
	$(CC) -c $(CFLAGS) $(SDL2) main.c util.h

chip8.o: chip8.c chip8.h util.h
	$(CC) -c $(CFLAGS) $(SDL2) chip8.c chip8.h util.h

clean:
	rm -f *.out *.o *.gch

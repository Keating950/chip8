DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CFLAGS = -std=c99 -Wall -g
else
    CFLAGS= -std=c99 -O2
endif

SDL2=`pkg-config --cflags --libs sdl2`


chip8: main.o chip8.o av_io.o 
	$(CC) -o chip8 $(CFLAGS) $(SDL2) main.o chip8.o av_io.o 

main.o: main.c
	$(CC) -c $(CFLAGS) $(SDL2) main.c

chip8.o: chip8.c chip8.h
	$(CC) -c $(CFLAGS) $(SDL2) chip8.c chip8.h

av_io.o: av_io.c av_io.h
	$(CC) -c $(CFLAGS) $(SDL2) av_io.c av_io.h 

clean:
	rm -f *.out *.o *.gch

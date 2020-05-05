#define ERROR_EXIT(msg)                                                   \
	do {                                                                  \
		perror(msg);                                                      \
		exit(EXIT_FAILURE);                                               \
	} while (0);

#define LEN(ARRAY) sizeof(ARRAY)/sizeof(ARRAY[0])


#define ZERO_FLOOR(N) ((N) > 0 ? (N) : 0)

#define SCREEN_WIDTH 0x40
#define SCREEN_HEIGHT 0x20
#define TIMER_HZ_NS 16666667

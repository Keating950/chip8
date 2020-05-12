#define ERROR_EXIT(msg)                                                        \
	do {                                                                       \
		perror(msg);                                                           \
		fputs("\n", stderr);                                                   \
		exit(EXIT_FAILURE);                                                    \
	} while (0)

#define LEN(ARRAY) sizeof(ARRAY) / sizeof(ARRAY[0])

#define ZERO_FLOOR(N) ((N) > 0 ? (N) : 0)

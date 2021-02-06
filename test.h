static char *text[] = {"hello", "fucking", "world!" };
static char *text2[] = {"hello", "fucking", "world22!" };

typedef struct Key {
	const void *first;
	const void *second;
} Key;

static Key keys = {
	text, text2
};

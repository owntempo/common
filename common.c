#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

void
die(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	exit(EXIT_FAILURE);
}

void *emalloc(size_t size) {
	void *p = malloc(size);
	if (!p) die("malloc:");
	return p;
}

void *ecalloc(size_t size) {
	void *p = calloc(1, size);
	if (!p) die("calloc:");
	return p;
}

void *erealloc(void *old, size_t size) {
	void *p = realloc(old, size);
	if (!p) die("realloc:");
	return p;
}

typedef struct Array_Header {
	size_t capacity;
	size_t size;
	unsigned char items[1];
} Array_Header;

#define array_header(a) ((Array_Header *)((unsigned char *)(a) - offsetof(Array_Header, items)))
#define array_size(a) ((a) ? array_header(a)->size : 0)
#define array_capacity(a) ((a) ? array_header(a)->capacity : 0)
#define array_push(a, item) (array_full(a) ? (a) = array_grow(a, sizeof(*a)) : 0, a[array_header(a)->size++] = item)
#define array_full(a) (array_size(a) == array_capacity(a))
#define array_free(a) (free(array_header(a)), (a) = 0)

void *array_grow(void *array, size_t item_size) {
	Array_Header *header = array ? array_header(array) : 0;
	size_t new_capacity = MAX(1, 2*array_capacity(array));
	header = erealloc(header, new_capacity*item_size + offsetof(Array_Header, items));
	header->capacity = new_capacity;
	header->size = array ? header->size : 0;
	return header->items;
}

void array_test(void) {
	int *a = 0;
	assert(array_size(a) == 0);
	assert(array_capacity(a) == 0);
	enum { N = 1000 };
	for (int i = 0; i < N; i++)
		array_push(a, i);
	assert(array_size(a) == N);
	for (int i = 0; i < N; i++)
		assert(a[i] == i);
#if 0
	for (int i = 0; i < 1000; i++)
		printf("%d\n", a[i]);
	printf("%zu %zu\n", array_size(a), array_capacity(a));
#endif
	array_free(a);
	assert(a == 0);
	assert(array_size(a) == 0);
}

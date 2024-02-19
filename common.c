#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct Array_Header {
	size_t capacity;
	size_t size;
	unsigned char items[0];
} Array_Header;

#define array_header(a) ((Array_Header *)((unsigned char *)(a) - offsetof(Array_Header, items)))
#define array_size(a) ((a) ? array_header(a)->size : 0)
#define array_capacity(a) ((a) ? array_header(a)->capacity : 0)
#define array_push(a, item) (array_full(a) ? (a) = array_grow(a, sizeof(*a)) : 0, a[array_header(a)->size++] = item)
#define array_full(a) (array_size(a) == array_capacity(a))
#define array_free(a) (free(array_header(a)))

void *array_grow(void *array, size_t item_size) {
	Array_Header *header = array ? array_header(array) : 0;
	size_t new_capacity = MAX(1, 2*array_capacity(array));
	header = realloc(header, new_capacity*item_size + offsetof(Array_Header, items));
	assert(header);
	header->capacity = new_capacity;
	header->size = array ? header->size : 0;
	return header->items;
}

void array_test(void) {
	int *a = 0;
	for (int i = 0; i < 1000; i++)
		array_push(a, i);
	for (int i = 0; i < 1000; i++)
		assert(a[i] == i);
#if 0
	for (int i = 0; i < 1000; i++)
	printf("%d\n", a[i]);
	printf("%zu %zu\n", array_size(a), array_capacity(a));
#endif
}

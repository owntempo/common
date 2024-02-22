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

void *emalloc(uint64_t size) {
	void *p = malloc(size);
	if (!p) die("malloc:");
	return p;
}

void *ecalloc(uint64_t size) {
	void *p = calloc(1, size);
	if (!p) die("calloc:");
	return p;
}

void *erealloc(void *old, uint64_t size) {
	void *p = realloc(old, size);
	if (!p) die("realloc:");
	return p;
}

// NOTE(gaurang): Dynamic array

typedef struct Array_Header {
	uint64_t capacity;
	uint64_t size;
	uint8_t items[1];
} Array_Header;

#define array_header(a) ((Array_Header *)((uint8_t *)(a) - offsetof(Array_Header, items)))
#define array_size(a) ((a) ? array_header(a)->size : 0)
#define array_capacity(a) ((a) ? array_header(a)->capacity : 0)
#define array_push(a, item) (array_full(a) ? (a) = array_grow(a, sizeof(*a)) : 0, a[array_header(a)->size++] = item)
#define array_full(a) (array_size(a) == array_capacity(a))
#define array_free(a) (free(array_header(a)), (a) = 0)

void *array_grow(void *array, uint64_t item_size) {
	Array_Header *header = array ? array_header(array) : 0;
	uint64_t new_capacity = MAX(1, 2*array_capacity(array));
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

// NOTE(gaurang): Hash Tables

const uint64_t HASH_UNUSED = 0xffffffffffffffffULL;
typedef struct Hash
{
	uint32_t num_buckets;
	uint32_t used_buckets;
	uint64_t *keys;
	uint64_t *values;
} Hash;

uint64_t hash_lookup(Hash *h, uint64_t k) {
	if (h->used_buckets == 0)
		return HASH_UNUSED;

	uint32_t i = k % h->num_buckets;
	while (h->keys[i] != k && h->keys[i] != HASH_UNUSED)
		i = (i + 1) % h->num_buckets;
	return h->keys[i] == HASH_UNUSED ? HASH_UNUSED : h->values[i];
}

void hash_free(Hash *h) {
	free(h->keys);
	free(h->values);
	*h = (Hash) { 0 };
}

void hash_add(Hash *h, uint64_t k, uint64_t v);
void hash_grow(Hash *h, uint64_t new_num_buckets) {
	if (new_num_buckets < 16) new_num_buckets = 16;

	Hash new_hash = {
		.keys = emalloc(new_num_buckets * sizeof(uint64_t)),
		.values = emalloc(new_num_buckets * sizeof(uint64_t)),
		.num_buckets = new_num_buckets,
	};
	memset(new_hash.keys, 0xff, sizeof(*new_hash.keys) * new_hash.num_buckets);

	for (uint32_t i = 0; i < h->num_buckets; i++)
		if (h->keys[i] != HASH_UNUSED)
			hash_add(&new_hash, h->keys[i], h->values[i]);

	hash_free(h);
	*h = new_hash;
}

void hash_add(Hash *h, uint64_t k, uint64_t v) {
	assert(k != HASH_UNUSED);
	if (2*h->used_buckets >= h->num_buckets)
		hash_grow(h, 2*h->num_buckets);
	uint32_t i = k % h->num_buckets;
	while (h->keys[i] != k && h->keys[i] != HASH_UNUSED) {
		i = (i + 1) % h->num_buckets;
	}
	if (h->keys[i] == HASH_UNUSED)
		h->used_buckets++;
	h->keys[i] = k;
	h->values[i] = v;
}

uint64_t hash_bytes(const uint8_t *p, uint64_t count) {
	// NOTE(gaurang): Fowler-Noll-Vo Hash Function [FNV-1a]
	uint64_t hash = 0xcbf29ce484222325ULL;
	for (uint64_t i = 0; i < count; i++) {
		hash ^= p[i];
		hash *= 0x00000100000001b3ULL;
	}
	return hash;
}

void hash_test(void) {
	Hash h = { 0 };
	enum { N = 1024 };
	for (int i = 1; i < N; i++)
		hash_add(&h, i, i+1);
	for (int i = 1; i < N; i++) {
		uint64_t v = hash_lookup(&h, i);
		assert(v == (uint64_t)(i+1));
	}
#if 0
	for (uint32_t i = 0; i < h.num_buckets; i++)
		printf("%llu, %llu\n", h.keys[i], h.values[i]);
#endif
	hash_free(&h);
}

// NOTE(gaurang): Intern Strings

typedef struct Intern {
	struct Intern *next;
	size_t length;
	char string[1];
} Intern;

Hash intern_lookup;
Intern **intern_pool;

const char *intern_string_range(const char *start, const char *end) {
	size_t length = end - start;
	uint64_t hash = hash_bytes((uint8_t *)start, length);
	uint64_t i = hash_lookup(&intern_lookup, hash);
	if (i != HASH_UNUSED) {
		// TODO(gaurang): Should we remove this because, collisions
		// are statistically impossible.
		for (Intern *it = intern_pool[i]; it; it = it->next)
			if (length == it->length && strncmp(start, it->string, length) == 0)
				return it->string;
	}
	Intern *new_intern = emalloc(offsetof(Intern, string) + length + 1);
	new_intern->length = length;
	memcpy(new_intern->string, start, new_intern->length);
	new_intern->string[length] = '\0';
	if (i == HASH_UNUSED) {
		array_push(intern_pool, new_intern);
		hash_add(&intern_lookup, hash, array_size(intern_pool) - 1);
	} else {
		new_intern->next = intern_pool[i];
		intern_pool[i] = new_intern;
	}
	return new_intern->string;
}

const char *intern_string(const char *s) {
	return intern_string_range(s, s+strlen(s));
}

void intern_test(void) {
	char a[] = "hello";
    assert(strcmp(a, intern_string(a)) == 0);
    assert(intern_string(a) == intern_string(a));
    assert(intern_string(intern_string(a)) == intern_string(a));
    char b[] = "hello";
    assert(a != b);
    assert(intern_string(a) == intern_string(b));
    char c[] = "hello!";
    assert(intern_string(a) != intern_string(c));
    char d[] = "hell";
    assert(intern_string(a) != intern_string(d));
}

void common_test(void) {
	array_test();
	hash_test();
	intern_test();
}

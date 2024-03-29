#  A set of commonly used C routines.

## Dynamic arrays

```C
int *a = NULL;
array_push(a, 25);
array_push(a, 8);
for (uint64_t i = 0; i < array_size(a); i++)
	printf("%d\n", a[i]);
array_free(a);
```

## Hash Tables

```C
// You can directly store items that can fit in an uint64_t.
Hash hash;
hash_add(&hash, hash_string("Something"), age);

// Otherwise use an array.
Hash hash;
Item *items;

array_push(items, my_item);
hash_add(&hash, hash_string("Something"), array_size(items)-1);

// Lookup
uint64_t value = hash_lookup(&hash, hash_string("Blah"));
if (value == HASH_UNUSED)
	printf("No value is mapped to the corresponding key.\n");
else
	printf("Value found.\n");
```

## String Interning

```C
const char *a = intern_string("foo");
const char *b = intern_string("foo");

// no need for strcmp()
if (a == b)
	return true;
```

### TODO

* Hash functions
* Explain some caveats about the implementation

## Dependencies

```C
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
```

## Credits

Most of the techniques used here are popularized (possibly invented?) by [Sean Barrett](http://nothings.org/).

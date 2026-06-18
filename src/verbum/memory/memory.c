#include <string.h>
#include <stdlib.h>

#include "memory.h"


void *memory_new(size_t size) {
	return malloc(size);
}

void memory_delete(void *ptr) {
	if(ptr != NULL) {
		free(ptr);
	}
}

void *memory_resize(void *ptr, size_t size) {
	return realloc(ptr, size);
}

void *memory_copy(void *data, size_t size) {
	void *d = memory_new(size);

	return (d == NULL) ? NULL : memcpy(d, data, size);
}

#include "../memory/memory.h"

#include "lexeme.h"


Lexeme lexeme_new(const char *d, size_t l) {
	return (Lexeme) {
		.data = d,
		.length = l,
	};
}

void lexeme_delete(Lexeme l) {
	memory_delete((void *) l.data);
}

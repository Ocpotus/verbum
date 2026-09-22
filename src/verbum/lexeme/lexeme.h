#ifndef SRC_LEXEME_H
#define SRC_LEXEME_H

#include <stdlib.h>


typedef struct Lexeme {
	const char *data;
	size_t length;
} Lexeme;


Lexeme lexeme_new(const char *d, size_t l);
void lexeme_delete(Lexeme l);


#endif

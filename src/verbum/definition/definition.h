#ifndef VERBUM_DEFINITION_H
#define VERBUM_DEFINITION_H

#include <stddef.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../token/token.h"


typedef cvector(Token) Definition;

Definition definition_new();

void definition_delete(Definition d);

void definition_push_back(Definition *d, Token t);

bool definition_equals(Definition a, Definition b);


#endif

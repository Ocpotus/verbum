#ifndef VERBUM_DEFINITION_H
#define VERBUM_DEFINITION_H

#include <stddef.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../symbol/symbol.h"


typedef cvector(Symbol) Definition;

Definition definition_new();

void definition_delete(Definition d);

void definition_push_back(Definition *d, Symbol s);

bool definition_equals(Definition a, Definition b);


#endif

#ifndef OPTIONAL_H
#define OPTIONAL_H


/* This exists only to convey to the programmer that a pointer may or may not point to
 * a value. 
 *
 *
 * For example take the following declarations:
 *
 * char *d1;
 * optional(char) d2;
 *
 * d1 is a normal declaration for a pointer, however it does not convey whether it points
 * to a singular character or multiple characters.
 *
 * d2 however tells the programmer that d2 can point to a singular char or NULL.
 *
 *
 * More examples:
 *
 * typedef char *String;
 *
 * optional(String) optionalString; (Maybe optionalString points to a String or not)
 */
#define optional(type) type *

#define optional_is_valid(optional) (optional != NULL)

#define optional_get(optional) (*optional)


#endif

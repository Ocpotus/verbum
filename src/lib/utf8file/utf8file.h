#ifndef UTF8FILE_H
#define UTF8FILE_H

#include <stdbool.h>
#include <stdlib.h>

#include "../utf8.h/utf8.h"


typedef size_t uoff_t;

/* Opaque declaration */
typedef struct UTF8File UTF8File;

/* Opens a UTF8File from the given path
 *
 * PARAMETERS:
 * 	path: a path to a file
 *
 * RETURNS:
 * 	A pointer to a UTF8File or NULL on failure
 */
UTF8File *utf8file_open(const char *path);

/* Closes a UTF8File
 * 
 * PARAMETERS:
 * 	f: UTF8File to close
 */
void utf8file_close(UTF8File *f);

/* Gets the next UTF8 codepoint in a UTF8File
 *
 * PARAMETERS:
 * 	f: a pointer to a UTF8File
 *
 * RETURNS:
 * 	A UTF8 codepoint
 */
utf8_int32_t utf8file_get_codepoint(UTF8File *f);

/* Ungets a UTF8 codepoint in a UTF8File
 *
 * PARAMETERS:
 * 	f: UTF8File to unget from
 */
utf8_int32_t utf8file_unget_codepoint(UTF8File *f);

/* Copies from the UTF8File
 *
 * PARAMETERS:
 * 	f: UTF8File to copy from
 * 	offset: where in the UTF8File to start copying from
 * 	nbytes: number of bytes to copy
 */
char *utf8file_copy_from(UTF8File *f, uoff_t offset, size_t nbytes);

/* Returns the size of the contents in a UTF8File
 *
 * PARAMETERS:
 * 	f: UTf8File to get the size of
 *
 * RETURNS:
 * 	size of the contents in bytes
 */
size_t utf8file_size(UTF8File *f);

/* Gets the offset in a UTF8File
 *
 * PARAMETERS:
 * 	f: UTF8File to process
 *
 * RETURNS:
 * 	offset in a UTF8File
 */
uoff_t utf8file_tell(UTF8File *f);

/* Checks if a UTF8File's offset is at the end
 *
 * PARAMETERS:
 * 	f: UTF8File to check
 *
 * RETURNS:
 * 	true if the UTF8File's offset is equal to the size.
 * 	false otherwise
 */
bool utf8file_at_end(UTF8File *f);

/* Gets the path of a UTF8File
 *
 * PARAMETERS:
 * 	f: UTF8File to retrieve from
 * 
 * RETURNS:
 * 	the path of the UTF8File
 */
const char *utf8file_path(UTF8File *f);

const char *utf8file_at(UTF8File *f, uoff_t pos);


#endif

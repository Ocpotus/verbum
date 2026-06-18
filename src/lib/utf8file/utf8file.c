#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "../utf8.h/utf8.h"
#include "../try.h/try.h"

#include "utf8file.h"


/* UTF8File struct
 *
 * MEMBERS:
 * 	path: path to the file to process
 * 	contents: actual contents in the file to process
 * 	size: size in bytes of the contents
 * 	offset: the offset of the UTF8File
 * 	codepointsize: the previous codepoint size
 */
typedef struct UTF8File {
	const char *path;
        char *contents;
        size_t size;
        uoff_t offset;
} UTF8File;

/* Extracts the contents of a path
 *
 * PARAMETERS:
 * 	path: the path to extract contents from
 * 	size: a pointer to store the number of bytes of the extracted contents
 *
 * RETURNS:
 * 	A byte array of the contents in a file
 */
static char *file_extract_contents(const char *path, size_t *size);

UTF8File *utf8file_open(const char *path) {
	UTF8File *result = NULL;

	if(path != NULL) {
		size_t size = 0;
		char *contents = try(file_extract_contents(path, &size), contents == NULL, { return NULL; });

		result = try(malloc(1 * sizeof(*result)), result == NULL, { return NULL; });
		result->contents = contents;
		result->size = size;
		result->offset = 0;
		result->path = path;
	}

        return result;
}

void utf8file_close(UTF8File *f) {
	munmap(f->contents, f->size);
	free(f);
}

static char *file_extract_contents(const char *path, size_t *size) {
	char *result = NULL;
        int fd = open(path, O_RDONLY, S_IRUSR | S_IWUSR);
	struct stat s;

        if((fd == -1) || (fstat(fd, &s) == -1)) {
		return NULL;
        } else {
                result = mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
		*size = s.st_size;

                if(result == NULL || close(fd) == -1) {
			return NULL;
                }
        }

	return result;
}

utf8_int32_t utf8file_get_codepoint(UTF8File *f) {
	utf8_int32_t result = '\0';

	if(f != NULL && f->offset != f->size) {
		utf8codepoint(f->contents + f->offset, &result);
		f->offset += utf8codepointsize(result);
	}

	return result;
}

utf8_int32_t utf8file_unget_codepoint(UTF8File *f) {
	utf8_int32_t result = '\0';

	if(f != NULL && f->offset != 0) {
		utf8rcodepoint(f->contents + f->offset, &result);
		f->offset -= utf8codepointsize(result);
	}

	return result;
}

size_t utf8file_size(UTF8File *f) {
	return f->size;
}

uoff_t utf8file_tell(UTF8File *f) {
	return f->offset;
}

bool utf8file_at_end(UTF8File *f) {
	return f->offset == f->size;
}

char *utf8file_copy_from(UTF8File *f, uoff_t offset, size_t nbytes) {
	char *result = malloc((nbytes + 1) * sizeof(*result));

	if(result != NULL) {
		result = memcpy(result, f->contents + offset, nbytes);
		result[nbytes - 1] = '\0';
	}

	return result;
}

const char *utf8file_path(UTF8File *f) {
	return f->path;
}

const char *utf8file_at(UTF8File *f, uoff_t offset) {
	return f->contents + offset;
}

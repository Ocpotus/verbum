#ifndef VERBUM_MEMORY_H
#define VERBUM_MEMORY_H

#include <stdlib.h>


/* Memory allocator
 *
 * PARAMETERS:
 * 	size: nbytes to allocate
 *
 * RETURNS:
 * 	A pointer if successful or NULL on failure
 */
void *memory_new(size_t size);

/* Memory deleter 
 *
 * PARAMETERS:
 *	ptr: pointer to delete
 */
void memory_delete(void *ptr);

/* Memory reallocator
 *
 * PARAMETERS:
 * 	ptr: memory to reallocate
 * 	size: nbytes to reallocate to
 *
 * RETURNS:
 * 	A pointer to the reallocated memory or NULL on failure
 */
void *memory_resize(void *ptr, size_t size);

/* Memory copier
 *
 * PARAMETERS:
 * 	data: memory to copy
 * 	size: nbytes to copy
 *
 * RETURNS:
 * 	A pointer to the allocated memory or NULL on failure
 */
void *memory_copy(void *data, size_t size);


#endif

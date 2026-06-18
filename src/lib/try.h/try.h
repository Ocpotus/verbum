#ifndef TRY_H
#define TRY_H

/* c_try:
 * 	e: the expression to try
 *	c: failure condition
 *	f: what to do in case of failure
 *
 * EXAMPLE:
 * 	char *mem = c_try(malloc(1024 * sizeof(*mem)), mem == NULL, { return NULL; });
 */
#define c_try(e, c, f) \
		e;\
		\
		{\
			if(c) {\
				f\
			}\
		}

#ifndef __cplusplus
#define try c_try
#endif

#endif

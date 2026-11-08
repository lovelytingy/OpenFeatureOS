/*
 * OpenFeatureOS - Freestanding C stdlib.h shim for GCC baremetal
 */

#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
int abs(int j);
long labs(long j);

#endif /* _STDLIB_H_ */

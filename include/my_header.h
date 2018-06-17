#ifndef MY_HEADER_H
#define MY_HEADER_H

#include "sf_free.h"
#include "sf_malloc.h"
#include "sf_realloc.h"
#include "sfmm.h"
#include "debug.h" // ADDED FOR debugging
#include <stdio.h>
#include <stdlib.h>
#include <errno.h> // ADDED FOR sf_errno
#include <string.h>

#define INFO

enum {FALSE, TRUE};
extern void* current_heap_ptr;
extern int sf_sbrk_count;

#endif
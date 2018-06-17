#ifndef SF_REALLOC_H
#define SF_REALLOC_H
#include "my_header.h"
#include "sfmm.h"

sf_free_header* realloc_for_larger_block(sf_free_header *target, size_t request_size);

sf_free_header* realloc_for_smaller_block(sf_free_header *target, size_t requested_size);

sf_header* get_allocated_splited_block_from_free_block(sf_free_header *target, size_t requested_size, int check_realloc);

#endif
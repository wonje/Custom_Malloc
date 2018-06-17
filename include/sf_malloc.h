#ifndef SF_MALLOC_H
#define SF_MALLOC_H
#include "my_header.h"
#include "sfmm.h"

size_t get_payload_size(size_t requested_size);

void init_header_and_footer(sf_header *header, sf_footer *footer, size_t requested_size);

sf_free_header* get_free_header_with_size_if_any(size_t block_size);

sf_free_header* get_free_header_with_addr(sf_header *header);

void insert_sf_free_header_into_free_list(sf_free_header *new_sf_free_header);

sf_free_header* check_immediate_coalescing_for_sf_sbrk(sf_free_header *new_sf_free_header);

free_list* get_seg_free_list_with_size(size_t block_size);

#endif
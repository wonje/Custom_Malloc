#ifndef SF_FREE_H
#define SF_FREE_H
#include "my_header.h"
#include "sfmm.h"

void validate_param_ptr(sf_free_header *target, sf_header *target_header, sf_footer *target_footer);

sf_free_header* immediate_coalescing(sf_free_header *current, sf_free_header *next);

void remove_sf_free_header_and_rearrange(sf_free_header *remove);

#endif
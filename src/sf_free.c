#include "sf_free.h"

void validate_param_ptr(sf_free_header *target, sf_header *target_header, sf_footer *target_footer){

    if(get_heap_start() == NULL){
        debug("%s", "sf_sbrk() did not used!");
        sf_errno = EINVAL;
        abort();
    }else if((void*)target < get_heap_start()){
        debug("%s", "HEADER < HEAP START ADDR");
        sf_errno = EINVAL;
        abort();
    }else if((void*)target_footer > get_heap_end()){
        debug("%s", "FOOTER > HEAP END ADDR");
        sf_errno = EINVAL;
        abort();
    }else if(target_header->allocated != target_footer->allocated){
        debug("%s", "HEADER ALLOC !=  FOOTER ALLOC");
        sf_errno = EINVAL;
        abort();
    }else if(target_header->padded != target_footer->padded){
        debug("%s", "HEADER PADDED != FOOTER PADDED");
        sf_errno = EINVAL;
        abort();
    }else if(target_header->allocated == 0){
        debug("%s", "ALLOC == 0");
        sf_errno = EINVAL;
        abort();
    }else if((target_header->padded == 1 && (target_header->block_size << 4 == target_footer->requested_size + 16))){
        debug("%s", "PADDING == 1, BUT BLOCK SIZE == requested_size + 16");
        sf_errno = EINVAL;
        abort();
    }else if((target_header->padded == 0 && (target_header->block_size << 4 != target_footer->requested_size + 16))){
        debug("%s", "PADDING == 0, BUT BLOCK SIZE != requested_size + 16");
        sf_errno = EINVAL;
        abort();
    }
}

sf_free_header* immediate_coalescing(sf_free_header *current, sf_free_header *next){
    sf_free_header *coalesced_sf_free_header;
    sf_header *coalesced_sf_header;
    sf_footer *coalesced_sf_footer;
    size_t block_size = (current->header.block_size << 4) + (next->header.block_size << 4);

    // DEFINE coalesced_sf_free_header
    coalesced_sf_free_header = current;
    coalesced_sf_header = (sf_header*)current; // SAME
    coalesced_sf_footer = (void*)&(next->header) + (next->header.block_size << 4) - 8; // CHECK IT

    init_header_and_footer(coalesced_sf_header, coalesced_sf_footer, block_size - 16);
    coalesced_sf_footer->requested_size = 0;

    coalesced_sf_free_header->header = *coalesced_sf_header;

    // Re Alignment new coalesced sf_free_header in free_list
    remove_sf_free_header_and_rearrange(current);
    remove_sf_free_header_and_rearrange(next);

    coalesced_sf_free_header->prev = NULL;
    coalesced_sf_free_header->next = NULL;


    return coalesced_sf_free_header;
}

void remove_sf_free_header_and_rearrange(sf_free_header *remove){
    if(remove->prev != NULL){
        // remove IS HEADER OF THE LIST
        if(remove->next != NULL)
            remove->prev->next = remove->next;
        else
            remove->prev->next = NULL;
    }
    if(remove->next != NULL){
        if(remove->prev != NULL)
            remove->next->prev = remove->prev;
        else
            remove->next->prev = NULL;
    }else{
        // NEXT == NULL, PREV == NULL
        free_list* target;
        target = get_seg_free_list_with_size(remove->header.block_size << 4);
        target->head = NULL;
    }
    remove->prev = NULL;
    remove->next = NULL;
}

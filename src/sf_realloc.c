#include "sf_realloc.h"

sf_free_header* realloc_for_larger_block(sf_free_header *old_target, size_t request_size){
    sf_free_header *new_target;
    // STEP 1 : GET NEW FREE BLOCK
    new_target = (sf_free_header*)sf_malloc(request_size);

    if(new_target == NULL)
        return NULL;

    new_target = (void*)new_target - 8;

    // if((new_target - 8) != NULL)
         // new_target = (void*)new_target - 8;
     // else
     //    return NULL;

    // STEP 2 : MEMCPY FROM THE GIVEN BLOCK TO NEW BLOCK
    memcpy((void*)new_target + 8, (void*)old_target + 8, request_size);

    // STEP 3 : FREE THE OLD ALLOCATED BLOCK
    sf_free((void*)old_target + 8);

    return new_target;
}

sf_free_header* realloc_for_smaller_block(sf_free_header *target, size_t requested_size){
    // TARGET IS ALLOCATED BLOCK
    sf_header *target_header = &target->header;
    sf_footer *target_footer = (void*)target_header + (target_header->block_size << 4) - 8;

    // UNALLOCATE THE GIVEN BLOCK TO SPLIT BY CONSIDERING A LARGE SIZE OF FREE BLOCK
    target_header->allocated = 0;
    target_footer->allocated = 0;

    // SPLIT IT AS ALLOCATED BLOCK AND FREE BLOCK
    // IF SPLITED BLOCK IS A SPLINTER, IT DOES NOT PERFORM SPLITING AND UPDATE BLOCK INFOA
    target_header = get_allocated_splited_block_from_free_block(target, requested_size, TRUE);

    // RETURN PAYLOAD ADDRESS OF THE TARGET
    return (void*)target_header;
}

sf_header* get_allocated_splited_block_from_free_block(sf_free_header *target, size_t requested_size, int check_realloc){
    // If target < 64bytes, it is splinter. DO NOT SPLIT
    // Minimum size of one block is 32. So, two blocks should have size at least 64 bytes.
    size_t block_size = get_payload_size(requested_size) + 16;
    sf_header *target_header = (void*)target;
    sf_footer *target_footer = (void*)target_header + (target->header.block_size << 4) - 8;
    if((target_header->block_size << 4) - block_size >= LIST_1_MIN){
        // If size is not division of 16, Add more bytes for payload
        // block_size ==> size of block to be inserted
        target_footer = (void*)target + block_size - 8;

        sf_free_header *splited_free_block = (void*)target_footer + 8;
        sf_header *splited_header = (void*)splited_free_block;
        // sf_footer *splited_footer = (void*)target + (target->header.block_size << 4) - 8;
        sf_footer *splited_footer = (void*)target_header + (target->header.block_size << 4) - 8;

        size_t payload_size_of_splited_block = (target->header.block_size << 4) - block_size - 16;

        init_header_and_footer(splited_header, splited_footer, payload_size_of_splited_block);
        splited_footer->requested_size = 0;
        // CHECK PADDING
        if((splited_header->block_size << 4) - 16 > payload_size_of_splited_block){
            splited_header->padded = 1;
            splited_footer->padded = 1;
        }
        // IF IT CALLED FROM REALLOC, CHECK COALESCING
        if(check_realloc == TRUE && ((void*)splited_footer + 8 < get_heap_end())){
            sf_free_header *current = (void*)splited_header;
            sf_free_header *next = (void*)splited_footer + 8;

            if(next->header.allocated == 0){
                // INIT CURRENT PREV AND NEXT
                current->prev = NULL;
                current->next = NULL;

                // COALESCING
                splited_free_block = immediate_coalescing(current, next);
            }
        }

        // INSERT splited free blocks into free_list
        // sf_blockprint(splited_free_block);
        insert_sf_free_header_into_free_list(splited_free_block);

        // Initial setting of headers and footers
        init_header_and_footer(target_header, target_footer, requested_size);
    }else{
        target_footer->requested_size = requested_size;
    }

    // CHECK PADDING
    if((target_header->block_size << 4) - 16 > requested_size){
        target_header->padded = 1;
        target_footer->padded = 1;
    }

    // USE THE SPLITED FREE BLOCK FOR ALLOCATING MEMORY SPACE
    target_header->allocated = 1;
    target_footer->allocated = 1;
    // sf_blockprint(target_header);

    return target_header;
}
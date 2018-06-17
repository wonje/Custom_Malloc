/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "my_header.h"
/**
 * You should store the heads of your free lists in these variables.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */
free_list seg_free_list[4] = {
    {NULL, LIST_1_MIN, LIST_1_MAX},
    {NULL, LIST_2_MIN, LIST_2_MAX},
    {NULL, LIST_3_MIN, LIST_3_MAX},
    {NULL, LIST_4_MIN, LIST_4_MAX}
};

// ############## SIZE UNIT IS BYTES ################

int sf_errno = 0;
void* current_heap_ptr = 0; // TO DESIGNATE CURRENT HEAP SPACE
int sf_sbrk_count = 0;

void *sf_malloc(size_t size){
    size_t block_size;
    sf_free_header *current_free_header;
    // int sf_sbrk_count = 0;

    // STEP 1 : VALIDATE PARAM SIZE
    if(size == 0 || PAGE_SZ * 4 < size){
        sf_errno = EINVAL;
        return NULL;
    }

    // DEFINE BLOCK SIZE FOR MALLOC
    block_size = get_payload_size(size) + 16;
    debug("sf_malloc() START! BLOCK SIZE FOR MALLOC : %ld", block_size);
    // STEP 2 : IF NO FREE LIST, USE 'sf_sbrk()' TO DEFINE NEW FREE LIST
    while((current_free_header = get_free_header_with_size_if_any(block_size)) == NULL){
        // IF THE BLOCK SIZE IS TOO LARGE
        if(sf_sbrk_count >= 4){
            sf_errno = ENOMEM;
            debug("%s","get_heap_end ALREADY CALLED 4 TIMES!");
            return NULL;
        }

        // CALL sf_sbrk() AND IMCREMENT THE COUNTER
        current_heap_ptr = sf_sbrk();
        sf_sbrk_count++;

        // ASSIGN FREE BLOCK
        sf_header *heap_header;
        sf_footer *heap_footer;

        heap_header = current_heap_ptr;
        heap_footer = get_heap_end() - 8; // followed by 64 bits
        init_header_and_footer(heap_header, heap_footer, get_heap_end() - current_heap_ptr - 16);
        heap_footer->requested_size = 0;
        // Q. If get_heap_end() - current_heap_ptr is not divisible by 16, what am I gonna do?
        // Does it happen??? No!!

        // DEFINE FREE BLOCK TO BE INSERTED
        sf_free_header *new_sf_free_header = (sf_free_header*) heap_header;
        new_sf_free_header->header = *heap_header;
        new_sf_free_header->next = NULL;
        new_sf_free_header->prev = NULL;

        // COALESCING WITH PREVIOUS FREE BLOCK IF EXISTS
        sf_free_header *coalesced_block;
        coalesced_block = check_immediate_coalescing_for_sf_sbrk(new_sf_free_header); // UPDATE current_heap_ptr

        // INSERT FREE BLOCK INTO FREE LIST
        insert_sf_free_header_into_free_list(coalesced_block);
        sf_snapshot();
    }
    // REMOVE free block from free_list
    remove_sf_free_header_and_rearrange(current_free_header);

    // STEP 3 : SPLITS THE CURRENT FREE BLOCK FOR THE requested_size
    sf_header *target = get_allocated_splited_block_from_free_block(current_free_header, size, FALSE);

    // STEP 4 : OUTPUT PRINT
    debug("%s", "sf_malloc() IS DONE...");
    // sf_blockprint(target);
    // sf_snapshot();
	return (void*)target + 8;
}

void *sf_realloc(void *ptr, size_t size) {
    sf_free_header *target;
    sf_header *target_header;
    sf_footer *target_footer;

    debug("%s", "sf_realloc() START!");
    // STEP 1 : VALIDATE PARAMETERS
    if(ptr == NULL){
        debug("%s", "NULL POINTER ABORT!");
        sf_errno = EINVAL;
        abort();
    }else if(get_heap_start() == NULL){
        debug("%s", "sf_sbrk() was not called!");
        sf_errno = EINVAL;
        abort();
    }

    // Init
    target = (void*)ptr - 8;
    target_header = &target->header;
    target_footer = (void*)target_header + (target_header->block_size << 4) - 8;

    validate_param_ptr(target, target_header, target_footer);

    // IF SIZE == 0, FREE THE BLOCK
    if(size == 0){
        sf_free((void*)target + 8);
        return NULL;
    }

    // STEP 2 : COMPARE THE SIZE OF TARGET WITH THE REQUESTED SIZE
    // get_payload_size(size)
    if((target_header->block_size << 4) < get_payload_size(size) + 16){
        // REQUEST SIZE IS LARGER THAN THE GIVEN BLOCK --> ALLOCATE NEW MEMORY USING sf_sbrk()
        target = realloc_for_larger_block(target, size);
    }else if((target_header->block_size << 4) > get_payload_size(size) + 16){
        // REQUESET SIZE IS SMALLER THAN THE GIVEN BLOCK --> SPLIT THE GIVEN BLOCK USING sf_free()
        target = realloc_for_smaller_block(target, size);
    }else{
        // REQUESTE SIZE == THE GIVEN BLOCK PAYLOAD SIZE --> RETURN THE GIVEN BLOCK WITH UPDATE HEADER AND FOOTER
        target_footer->requested_size = size;
        target_header->padded = 0;
        target_footer->padded = 0;
    }

    debug("%s", "sf_realloc() IS DONE...");
    // sf_snapshot();
    // SAME ADDRESS OF THE GIVEN BLOCK PAYLOAD
    debug("TARGET : %p", target);

    if(target == NULL)
        return NULL;

	return (void*)target + 8;
}

void sf_free(void *ptr) {
    sf_free_header *target;
    sf_header *target_header;
    sf_footer *target_footer;
    debug("%s", "sf_free() START!");

    // STEP 1 : FILTER INVALID POINTER
    if(ptr == NULL){
        debug("%s", "NULL POINTER ABORT!");
        sf_errno = EINVAL;
        abort();
    }else if(get_heap_start() == NULL){
        debug("%s", "sf_sbrk() was not called!");
        sf_errno = EINVAL;
        abort();
    }
    // Init
    target = (void*)ptr - 8;
    target_header = &target->header;
    target_footer = (void*)target_header + (target_header->block_size << 4) - 8;
    validate_param_ptr(target, target_header, target_footer);
    // STEP 2 : UNALLOCATE THE GIVEN BLOCK
    target_header->allocated = 0;
    target_footer->allocated = 0;

    // STEP 3 : CHECK NEXT BLOCK IS UNALLOCATED. IF IT DOES, COALESCE IT.
    sf_free_header *next_target;
    sf_free_header *coalesced_target = target;

    if((void*)target + (target->header.block_size << 4) != get_heap_end()){
        next_target = (sf_free_header*)((void*)target + (target->header.block_size << 4));
        if(next_target->header.allocated == 0){
            // Coalescing
            remove_sf_free_header_and_rearrange(next_target);
            coalesced_target = immediate_coalescing(target, next_target);
        }
    }
    // STEP 4 : INSERT INTO FREE_LIST
    insert_sf_free_header_into_free_list(coalesced_target);

    debug("%p sf_free() IS DONE...", target);
    // sf_snapshot();
	return;
}

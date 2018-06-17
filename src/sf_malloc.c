#include "sf_malloc.h"

size_t get_payload_size(size_t requested_size){
    size_t payload_size;
    size_t size_with_header_footer = requested_size + 16;
    if(size_with_header_footer < 32){
        payload_size = 16; // 32 BYTE - HEADER(8) - FOOTER(8) = 16
    }else if(size_with_header_footer % 16){
        payload_size = requested_size + (16 - (size_with_header_footer % 16));
    }else{
        payload_size = requested_size;
    }
    return payload_size;
}

void init_header_and_footer(sf_header *header, sf_footer *footer, size_t requested_size){
    // INIT HEADER
    header->allocated = 0;
    header->padded = 0;
    header->two_zeroes = 0;
    header->block_size = 0;
    header->block_size = (get_payload_size(requested_size) + 16) >> 4;
    header->unused = 0;

    // INIT FOOTER
    footer->allocated = 0;
    footer->padded = 0;
    footer->two_zeroes = 0;
    footer->block_size = 0;
    footer->block_size = (get_payload_size(requested_size) + 16) >> 4;
    footer->requested_size = requested_size;
}

sf_free_header* get_free_header_with_size_if_any(size_t block_size){
    sf_free_header *free_header = NULL;

    // FIND FREE LIST IF ANY
    int i;
    for(i = 0; i < 3; i++){
         if(block_size <= seg_free_list[i].max && seg_free_list[i].head != NULL){
            // CHECK THERE IS SUFFICIENT FREE BLOCK IN THE ARRAY
            sf_free_header *temp = seg_free_list[i].head;
            do{
                if((temp->header.block_size << 4) >= block_size){
                    free_header = seg_free_list[i].head;
                    break;
                }
            }while((temp = temp->next) != NULL);
            // free_header = seg_free_list[i].head;
            // break;
        }
    }

    if(i == 3 && seg_free_list[i].head != NULL){
        free_header = seg_free_list[i].head;
    }


    if(free_header == NULL)
        return NULL;

    // FIND FREE BLOCK HEADER
    do{
        if(free_header->header.block_size << 4 >= block_size)
            return free_header;
    }while((free_header = free_header->next) != NULL);

    return NULL;
}


sf_free_header* check_immediate_coalescing_for_sf_sbrk(sf_free_header *new_sf_free_header){
    sf_footer *previous_free_footer;
    sf_header *previous_free_header;

    previous_free_footer = current_heap_ptr - 8;
    previous_free_header = current_heap_ptr - (previous_free_footer->block_size << 4);

    // CHECK PREVIOUS BLOCK IS ALLOCATED OR FIRST ONE
    if(previous_free_footer->allocated == 1 || current_heap_ptr - PAGE_SZ < get_heap_start())
        return new_sf_free_header;

    // UPDATE CURRENT HEAP PTR
    current_heap_ptr = (void*)previous_free_header;

    // REMOVE THE BLOCK FROM FREE LIST
    sf_free_header *previous_sf_free_header = get_free_header_with_addr(previous_free_header);
    return immediate_coalescing(previous_sf_free_header, new_sf_free_header);
}

sf_free_header* get_free_header_with_addr(sf_header *header){
    int i;
    size_t block_size = header->block_size << 4;
    sf_free_header *free_header = NULL;

    // FIND FREE LIST
    for(i = 0; i < 4; i++){
        if(seg_free_list[i].head == NULL)
            continue;
        if(i == 3){
            free_header = seg_free_list[3].head;
            break;
        }else if(block_size <= seg_free_list[i].max){
            free_header = seg_free_list[i].head;
            break;
        }
    }
    if(free_header == NULL)
        return NULL;

    // FIND FREE BLOCK WITH THE HEADER
    while(header != &(free_header->header)){
        if((free_header = free_header->next) == NULL)
            return NULL;
    }

    return free_header;
}

void insert_sf_free_header_into_free_list(sf_free_header *target){
    size_t block_size;
    free_list* target_list;

    block_size = target->header.block_size << 4;
    target_list = get_seg_free_list_with_size(block_size);

    if(target_list->head == NULL){
        target_list->head = target;
    }else{
        // Insert target ahead of the free_list
        target->next = target_list->head;
        target_list->head->prev = target;
        target_list->head = target;
    }
}

free_list* get_seg_free_list_with_size(size_t block_size){
    int i;
    for(i = 0; i < 3; i++){
        if(block_size <= seg_free_list[i].max)
            break;
    }

    return &seg_free_list[i];
}
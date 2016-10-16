#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sfmm.h"

/**
 *  HERE ARE OUR TEST CASES NOT ALL SHOULD BE GIVEN STUDENTS
 *  REMINDER MAX ALLOCATIONS MAY NOT EXCEED 4 * 4096 or 16384 or 128KB
 */

Test(sf_memsuite, Malloc_an_Integer, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(sizeof(int));
    *x = 4;
    cr_assert(*x == 4, "Failed to properly sf_malloc space for an integer!");
}

Test(sf_memsuite, Free_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(short));
   
    sf_free(pointer);

    pointer -= 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->alloc == 0, "Alloc bit in header is not 0!\n");
    sf_footer *sfFooter = (sf_footer *) (pointer + (sfHeader->block_size << 4) -8);
    cr_assert(sfFooter->alloc == 0, "Alloc bit in the footer is not 0!\n");
}

Test(sf_memsuite, PaddingSize_Check_char, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(char));
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->padding_size == 15, "Header padding size is incorrect for malloc of a single char!\n");
}

Test(sf_memsuite, Check_next_prev_pointers_of_free_block_at_head_of_list, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(4);
    memset(x, 0, 4);
    cr_assert(freelist_head->next == NULL);
    cr_assert(freelist_head->prev == NULL);
}


Test(sf_memsuite, Coalesce_no_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4);
    void *y = sf_malloc(4);
    memset(y, 0xFF, 4);
    sf_free(x);
    cr_assert(freelist_head == x-8);
    sf_free_header *headofx = (sf_free_header*) (x-8);
    sf_footer *footofx = (sf_footer*) (x - 8 + (headofx->header.block_size << 4)) - 8;

    sf_blockprint((sf_free_header*)((void*)x-8));
    // All of the below should be true if there was no coalescing
    cr_assert(headofx->header.alloc == 0);
    cr_assert(headofx->header.block_size << 4 == 32);
    cr_assert(headofx->header.padding_size == 0);

    cr_assert(footofx->alloc == 0);
    cr_assert(footofx->block_size << 4 == 32);
}

/*
//############################################
// STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
// DO NOT DELETE THESE COMMENTS
//############################################
*/

Test(sf_memsuite, Allocating_max_page_size, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4070);
    void *y = sf_malloc(4070);
    sf_varprint(x);
    sf_varprint(y);
    cr_assert(y - x == 4096);
}

Test(sf_memsuite, Coalescing_all_cases, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *a = sf_malloc(10);
    void *b = sf_malloc(10);
    void *c = sf_malloc(10);
    void *d = sf_malloc(10);
    void *e = sf_malloc(10);
    void *f = sf_malloc(10);

    //check if previous block free to coalesce
    sf_free(a);
    sf_free(b);
    
    a -= 8;
    cr_assert(((sf_header*)(a))->block_size << 4 == 64 );

    //check if next block free to coalesce
    sf_free(e);
    sf_free(d);
  
    sf_varprint(d);
    cr_assert(((sf_header*)(d-8))->block_size << 4 == 64);

    //check if both blocks free to coalesce

    sf_free(c);
    sf_free(f);
    //the whole page should have been coalesced as 1 free block
    cr_assert(((sf_header*)freelist_head)->block_size << 4 == 4096);

}

Test(sf_memsuite, Realloc_to_smaller, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *g = sf_malloc(40);
    g = sf_realloc(g,4);
    g -= 8;
    //assert block was realloced correctly
    cr_assert(((sf_header*)(g))->block_size << 4 == 32);
    cr_assert(((sf_header*)(g))->padding_size == 12);

}

Test(sf_memsuite, Realloc_to_larger, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *h = sf_malloc(40);
    h = sf_realloc(h,400);
    h -=8;
    //assert block was realloced correctly
    cr_assert(((sf_header*)(h))->block_size << 4 == 416);
    cr_assert(((sf_header*)(h))->padding_size == 0);

}
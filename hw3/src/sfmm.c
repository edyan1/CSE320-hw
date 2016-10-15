#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "sfmm.h"

/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */

sf_free_header* freelist_head = NULL;
static info status;
static int pagesCalled = 0;

void *sf_malloc(size_t size){

	void* addr;
	if (size == 0) return NULL;
	else if (size > 16368) {
		//if size is greater than the size of 4 pages minus header and footer
		errno = ENOMEM;
		return NULL;
	}
	else {

		if (freelist_head == NULL) { 
			//first call of sf_malloc, set initial info status values
			status.internal = 0;
			status.allocations = 0;
			status.frees = 0;
			status.coalesce = 0;

			//call sf_sbrk
			addr = sf_sbrk(size);
			pagesCalled++;
			status.external = 4096;

			//if space is not evenly divisible by 16 (remainder of 8), then add 8
			if ((int)addr % 16 == 8) addr += 8; 


			//set the first node of freelist_head
			sf_header* newFree = addr;
			newFree->alloc = 0x0;
			newFree->block_size = 4096;
			newFree->unused_bits = 0;
			newFree->padding_size = 0;
			freelist_head = (sf_free_header*)newFree;
			freelist_head->next = NULL;
			freelist_head->prev = NULL;
		}
		
		//not the firt call of sf_malloc
		//find the first free block that fits
		addr = freelist_head;
		freelist_head = freelist_head->next;

		int payload = size;
		int padding = (16 - payload%16);
		int blockSize;
		int sizeNeeded = payload + padding + 16;


		sizeNeeded = 
		blockSize = ((sf_header*)addr)->block_size;


		while (blockSize > sizeNeeded){

			if (freelist_head->next != NULL){ 
				addr = freelist_head->next;
				blockSize = ((sf_header*)addr)->block_size - 8;
			}
			else if (pagesCalled >= 4) {
				errno = ENOMEM;
				return NULL;
			}
			else { //allocate new page on heap
				addr = sf_sbrk(size);
				pagesCalled++;
				status.external = 4096;

				//if space is not evenly divisible by 16 (remainder of 8), then add 8
				if ((int)addr % 16 == 8) addr += 8; 


				//set the first node of freelist_head
				sf_header* newFree = addr;
				newFree->alloc = 0x0;
				newFree->block_size = 4096;
				newFree->unused_bits = 0;
				newFree->padding_size = 0;

				if (freelist_head == NULL){
					freelist_head = (sf_free_header*)newFree;
					freelist_head->next = NULL;
					freelist_head->prev = NULL;
				}
				else if (freelist_head != NULL){
					freelist_head->next = (sf_free_header*)newFree;
					((sf_free_header*)newFree)->prev = freelist_head;
					((sf_free_header*)newFree)->next = NULL;
				}
				break;
			}
		}


		//allocation to a free node:
		blockSize = ((sf_header*)addr)->block_size; //size of free block to be allocated to
		
		//if (payload%16 != 0) payload += (16-payload%16);
		//header
		sf_header* header = (sf_header*)addr;
		//alloc bits
		header->alloc = 0x1;
		//block size, payload + padding + header and footer
		header->block_size = payload + padding + 16;
		//unused bits
		header->unused_bits = 0;
		//padding
		header->padding_size = padding;

		//footer
		sf_footer* foot = (sf_footer*)(addr + 8 + payload + padding);
		foot->alloc = 0x1;
		foot->block_size = payload + padding +16;

		//update info status
		status.allocations += 1;
		status.internal += (8 + 8 + padding); //header footer and padding

		//set the rest of block as free block
		//set free header
		sf_free_header* nextFree = foot + 8;
		(nextFree->header).alloc = 0;
		(nextFree->header).block_size = blockSize - header->block_size;
		(nextFree->header).unused_bits = 0;
		(nextFree->header).padding_size = (nextFree->header).block_size%16;
		//set free block footer
		sf_footer* freeFoot = (nextFree->header).block_size - 8;
		freeFoot->alloc = 0x0;
		freeFoot->block_size = (nextFree->header).block_size;

		nextFree->next = freelist_head->next;
		freelist_head = nextFree;
		//return address of first payload row
		return header+8;
	}
}

void sf_free(void *ptr){

	//set allocated bit to 0
	sf_header* headAddr = ptr - 8;
	headAddr->alloc = 0x0; //set header alloc bit to 0
	sf_footer* footAddr = ptr + (headAddr->padding_size + headAddr->block_size);
	footAddr->alloc = 0x0;	//set footer alloc bit to 0

	//add block back onto free list
	void* temp = freelist_head;
	freelist_head = (void*)headAddr;
	freelist_head->next = temp;

	//update info status
	status.frees += 1;
	status.external += (headAddr->block_size - 16); //freed space equal to block size minus header and footer
	status.internal -= headAddr->padding_size; //remove the padding from internal
	status.internal -= 16; //remove the header and footer size from internal 
	
}

void *sf_realloc(void *ptr, size_t size){
  return NULL;
}

int sf_info(info* meminfo){
	if (meminfo == NULL) return -1;
	else if (status.allocations == 0) return -1;
	else {
		meminfo->internal = status.internal;
		meminfo->external = status.external;
		meminfo->allocations = status.allocations;
		meminfo->frees = status.frees;
		meminfo->coalesce = status.coalesce;
	  return 0;
	}
}

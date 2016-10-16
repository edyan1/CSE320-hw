#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "sfmm.h"

/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */

sf_free_header* freelist_head = NULL;
static info status;
static int pagesCalled = 0;
static void* heapStart;
static void* heapEnd;

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
			status.external = 0;
			status.allocations = 0;
			status.frees = 0;
			status.coalesce = 0;

			//call sf_sbrk
			addr = sf_sbrk(size);
			heapStart = addr;
			heapEnd = addr + 4096;
			pagesCalled++;
			status.external += 4096;

			//if payload address is not evenly divisible by 16 (remainder of 8), then add 8
			if ((unsigned long)(addr+8) % 16 == 8) addr += 8; 


			//set the first node of freelist_head
			sf_header* newFree = addr;
			newFree->alloc = 0x0;
			newFree->block_size = (4096)>>4;
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
		int padding = (16 - payload%16)%16;
		int blockSize;
		int sizeNeeded = payload + padding + 16;

		blockSize = ((sf_header*)addr)->block_size<<4;


		while (blockSize < sizeNeeded){

			
			if (pagesCalled >= 4) {
				errno = ENOMEM;
				return NULL;
			}
			else if (freelist_head != NULL && freelist_head->next != NULL){
					addr = freelist_head->next;
					blockSize = ((sf_header*)addr)->block_size;
				
			}
			else { //allocate new page on heap
				addr = sf_sbrk(size);
				heapEnd += 4096;
				pagesCalled++;
				status.external += 4096;

				//if space is not evenly divisible by 16 (remainder of 8), then add 8
				if ((unsigned long)(addr+8) % 16 == 8) addr += 8; 


				//set the first node of freelist_head
				sf_header* newFree = addr;
				newFree->alloc = 0x0;
				newFree->block_size = (4096)>>4;
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
		blockSize = ((sf_header*)addr)->block_size << 4; //size of free block to be allocated to
		
		//if (payload%16 != 0) payload += (16-payload%16);
		//header
		sf_header* header = (sf_header*)addr;
		//alloc bits
		header->alloc = 0x1;
		//block size, payload + padding + header and footer
		header->block_size = (payload + padding + 16)>>4;
		//unused bits
		header->unused_bits = 0;
		//padding
		header->padding_size = padding;

		//footer
		sf_footer* foot = addr + 8 + payload + padding;
		foot->alloc = 0x1;
		foot->block_size = (payload + padding +16)>>4;

		//update info status
		status.allocations += 1;
		status.internal += (8 + 8 + padding); //header footer and padding


		//set the rest of block as free block
		//set free header
		sf_header* nextFree = (sf_header*)(foot + 2);
		nextFree->alloc = 0;
		nextFree->block_size = (blockSize - ((header->block_size)<<4))>>4;
		nextFree->unused_bits = 0;
		nextFree->padding_size = (nextFree->block_size<<4)%16;

		//set free block footer, unless blocksize is 0
		if (nextFree->block_size != 0) {
		sf_footer* freeFoot = (void*)(foot) + (nextFree->block_size<<4);
		freeFoot->alloc = 0;
		freeFoot->block_size = nextFree->block_size;
		}
		if (nextFree->block_size != 0){
			if (freelist_head != NULL)
				((sf_free_header*)nextFree)->next = freelist_head;

			freelist_head = (sf_free_header*)nextFree;
		}	
		//return address of first payload row
		return header+1;
	}
}

void sf_free(void *ptr){

	if (ptr == NULL) return;
	if (ptr < heapStart) return;
	if (ptr > heapEnd) return;
	//if the previous 8 bytes isn't a header with allocated bit = 0x1, then return
	if (((sf_header*)(ptr-8))->alloc != 0x1) return;
	
	else {
		//set allocated bits to 0
		sf_header* headAddr = ptr - 8;
		headAddr->alloc = 0x0; //set header alloc bit to 0
		headAddr->padding_size = 0x0; //set padding to 0
		sf_footer* footAddr = ptr + (headAddr->block_size<<4) - 16; 
		//address of payload + block size - header and footer sizes
		footAddr->alloc = 0x0;	//set footer alloc bit to 0

		sf_footer* prevFoot;
		sf_header* nextHead;
		int coalesce = 0;

		//coalesce if necessary
		if ((unsigned long)headAddr > (unsigned long)heapStart && (unsigned long)footAddr < (unsigned long)heapEnd) {
			prevFoot = ptr - 16;
			nextHead = ptr + (headAddr->block_size << 4) - 8;
			coalesce = 1;
		}
	
		if (coalesce) {
		//only coalesced if free block is within the heap

			//first, if there is a previous free block coalesce it
			if (prevFoot->alloc == 0x0){
				//newly freed footer stays in same location, prev foot and current header become free space
				footAddr->block_size += prevFoot->block_size;
				sf_header* prevHead = (void*)prevFoot - (prevFoot->block_size<<4) + 8;
				prevHead->block_size = footAddr->block_size;
				headAddr = prevHead;
			}

			//then, if there is a next free block coalesce it
			if (nextHead->alloc == 0x0){
				//header (either coalesced or not) stays same location, footer and next header become free space
				sf_footer* nextFoot = (void*)nextHead + (nextHead->block_size<<4) - 8;
				headAddr->block_size += nextHead->block_size;
				
				nextFoot->block_size = headAddr->block_size;
				footAddr = nextFoot;
			}

			status.coalesce++;
		}

		//update info status
		status.frees += 1;
		status.external += (headAddr->block_size<<4) - 16; //freed space equal to block size minus header and footer
		status.internal -= headAddr->padding_size; //remove the padding from internal
		status.internal -= 16; //remove the header and footer size from internal

		//add block back onto free list as the head (most recently freed)
		void* temp = freelist_head;

		freelist_head = (sf_free_header*)headAddr;
		freelist_head->next = temp;
		//check if free.next was coalesced, if so, remove and set as next head
		if (freelist_head->next != NULL && (unsigned long)freelist_head->next == (unsigned long)nextHead){
			void* temp2 = (freelist_head->next)->next;
			freelist_head->next = temp2;
		}
		(freelist_head->next)->prev = freelist_head;

		freelist_head->prev = NULL;
		//make sure next in freelist head is not itself
		if(freelist_head == freelist_head->next) freelist_head->next = NULL;
	}
	
}

void *sf_realloc(void *ptr, size_t size){
	if (ptr == NULL){
		errno = EINVAL;
  		return NULL;
  	}
  	if (ptr < heapStart){
  		errno = EINVAL;
  		return NULL;
  	}
	if (ptr > heapEnd){
  		errno = EINVAL;
  		return NULL;
  	}  	
  	if (size == 0){
  		errno = EINVAL;
  		return NULL;
  	}
  	if (size > 16368){
  		errno = EINVAL;
  		return NULL;
  	}
  	if (((sf_header*)(ptr-8))->alloc != 0x1) {
  		errno = EINVAL;
  		return NULL;
  	}

  	void* addr = ptr; 

	//get payload (and padding) size which is the block size of the block being realloc'd minus 16
	int payPadSize = (((sf_header*)(ptr-8))->block_size << 4) - 16;


  	//if sf_realloc size is bigger than previous sf_malloc size:
  	if(size > payPadSize){
	  	void* newBlock = sf_malloc(size);
	  	addr = memmove(newBlock, ptr, payPadSize);
	  	sf_free(ptr);
	}

	if(size < payPadSize){
		sf_free(ptr);
		void* newBlock = sf_malloc(size);
		addr = memmove(newBlock, ptr, size);
	}

	
  	return addr;
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

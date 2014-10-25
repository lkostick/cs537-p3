/******************************************************************************
 * FILENAME: mem.c
 * DATE:     20 Nov 2013
 * PROVIDES: Contains a set of library functions for memory allocation
 * MODIFIED BY: Logan Kostick, section 1 boop
 *
 * AUTHOR:   cherin@cs.wisc.edu <Cherin Joseph>
 * *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include "mem.h"
#include "bitmap.h"
#include <pthread.h>
#include "myPthread.h"

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

/* Global variable - This will always point to the first block */
/* ie, the block with the lowest address */
char* list_head = NULL;
//int current = 0;
int* curr = NULL;
int maxSlots = 0;

/* Function used to Initialize the memory allocator */
/* Not intended to be called more than once by a program */
/* Argument - sizeOfRegion: Specifies the size of the chunk which needs to be allocated */
/* Returns 0 on success and -1 on failure */
int Mem_Init(int sizeOfRegion)
{
  int pagesize;
  int padsize;
  int fd;
  int alloc_size;
  int arraySize;
  int i;
  int pad;
  int sub;
  void* space_ptr;
  static int allocated_once = 0;
  
  Pthread_mutex_lock(&m);

  if(0 != allocated_once)
  {
    fprintf(stderr,"Error:mem.c: Mem_Init has allocated space during a previous call\n");
    Pthread_mutex_unlock(&m);
    return -1;
  }
  if(sizeOfRegion <= 0)
  {
    fprintf(stderr,"Error:mem.c: Requested block size is not positive\n");
    Pthread_mutex_unlock(&m);
    return -1;
  }

  /* Get the pagesize */
  pagesize = getpagesize();

  /* Calculate padsize as the padding required to round up sizeOfRegio to a multiple of pagesize */
  padsize = sizeOfRegion % pagesize;
  padsize = (pagesize - padsize) % pagesize;

  alloc_size = sizeOfRegion + padsize;

  /* Using mmap to allocate memory */
  fd = open("/dev/zero", O_RDWR);
  if(-1 == fd)
  {
    fprintf(stderr,"Error:mem.c: Cannot open /dev/zero\n");
    Pthread_mutex_unlock(&m);
    return -1;
  }
  space_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (MAP_FAILED == space_ptr)
  {
    fprintf(stderr,"Error:mem.c: mmap cannot allocate space\n");
    allocated_once = 0;
    Pthread_mutex_unlock(&m);
    return -1;
  }
  
  allocated_once = 1;
  
  /* To begin with, there is only one big, free block */
  list_head = (char*)space_ptr;
  maxSlots = alloc_size / 16 ;
//  fprintf(stdout, "int size = %d\n", (int) sizeof(int));
  fprintf(stdout, "maxSlots = %d\n", maxSlots);
  arraySize = maxSlots / 64; 
  fprintf(stdout, "arraySize = %d\n", arraySize);
  if(arraySize == 0){
     arraySize++;
  }
  curr = (int *) space_ptr;
  for(i = 0; i <= arraySize; i++){
     curr[i] = 0;
  }
  list_head += (arraySize*8+ 8);
  pad = (arraySize + 1) / 2; 
  pad = (arraySize + 1 + pad) % 2;
  sub = (arraySize + 1) + pad;
  maxSlots = maxSlots - (sub/2); 
  fprintf(stdout, "sub = %d, maxSlots = %d\n", sub, maxSlots);
  fprintf(stdout, "curr = %p, head = %p\n", curr, list_head);
  /* Remember that the 'size' stored in block size excludes the space for the header */
  close(fd); // Is this right?
  Pthread_mutex_unlock(&m);
  return 0;
}


/* Function for allocating 'size' bytes. */
/* Returns address of allocated block on success */
/* Returns NULL on failure */
/* Here is what this function should accomplish */
/* - Check for sanity of size - Return NULL when appropriate */
/* - Round up size to a multiple of 4 */
/* - Traverse the list of blocks and allocate the first free block which can accommodate the requested size */
/* -- Also, when allocating a block - split it into two blocks when possible */
/* Tips: Be careful with pointer arithmetic */
void* Mem_Alloc(int size)
{
  Pthread_mutex_lock(&m);
//  int blocksize = 8;
  /* Your code should go in here */
//  int msize; /* Size to a multiple of 8 */
  char* tmp; /* Pointer to traverse the list */
  tmp = list_head; /* Set the tmp to list head */
  int distance = 0;  
  /* If an incorrect argument is passed in return NULL */
  if(size <= 0)
  {
    Pthread_mutex_unlock(&m);
    return NULL;
  }
  if((*curr) < maxSlots){
	distance = (int) (tmp - list_head);
	distance = distance / 16;
	while((testBit((curr+1), distance)) == 1){
		tmp += 16;
		distance = (int) (tmp - list_head);
		distance = distance / 16;
	}
	*(curr) = *(curr) + 1;	
	setBit((curr+1), distance);
	Pthread_mutex_unlock(&m);
	return (void *) tmp;

  }
  Pthread_mutex_unlock(&m);
  return NULL; /* If there isn't enough space, return NULL */
}


/* Function for freeing up a previously allocated block */
/* Argument - ptr: Address of the block to be freed up */
/* Returns 0 on success */
/* Returns -1 on failure */
/* Here is what this function should accomplish */
/* - Return -1 if ptr is NULL */
/* - Return -1 if ptr is not pointing to the first byte of a busy block */
/* - Mark the block as free */
/* - Coalesce if one or both of the immediate neighbours are free */
int Mem_Free(void *ptr)
{
  Pthread_mutex_lock(&m);
 // /* Your code should go in here */
//  int check = 0; /* Variable to see if ptr is from Mem_Alloc call */
  char* tmp;
  char* ptr1;
  int distance = 0;
  int check  = 0;
  int i = 0;
  tmp = list_head; 
  if(ptr == NULL) /* If ptr is NULL return -1 */
  {
    Pthread_mutex_unlock(&m);
    return -1;
  }
  ptr1 = (char *) ptr;
  distance = (int) (ptr1 - list_head);
  distance = distance / 16;

  if((distance < 0) || (distance >= maxSlots)){
	Pthread_mutex_unlock(&m);
  	return -1;
  }
  for(i = 0; i <= distance; i++){
  	if(tmp == ptr1){
		check = 1;
		break;
	}
	tmp += 16;
  }
  if(!check){
	Pthread_mutex_unlock(&m);
	return -1;	
  }
  if((testBit((curr+1), distance)) == 0){
	Pthread_mutex_unlock(&m);
	return -1;
  } 
  
  clearBit((curr+1), distance);
  Pthread_mutex_unlock(&m);
  return 0;
}

/* Not implemented for workload 1 */
void Mem_Dump()
{
   return;
}

/* Not implemented for workload 1 */
int Mem_Available()
{
   return 0;
}

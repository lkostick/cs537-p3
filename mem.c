/******************************************************************************
 * FILENAME: mem.c
 * AUTHOR:   cherin@cs.wisc.edu <Cherin Joseph>
 * DATE:     20 Nov 2013
 * PROVIDES: Contains a set of library functions for memory allocation
 * MODIFIED BY: Logan Kostick, section 1 boop
 * *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "mem.h"

/* this structure serves as the header for each block */
typedef struct block_hd{
  /* The blocks are maintained as a linked list */
  /* The blocks are ordered in the increasing order of addresses */
  struct block_hd* next;

  /* size of the block is always a multiple of 4 */
  /* ie, last two bits are always zero - can be used to store other information*/
  /* LSB = 0 => free block */
  /* LSB = 1 => allocated/busy block */

  /* For free block, block size = size_status */
  /* For an allocated block, block size = size_status - 1 */

  /* The size of the block stored here is not the real size of the block */
  /* the size stored here = (size of block) - (size of header) */
  int size_status;

}block_header;

/* Global variable - This will always point to the first block */
/* ie, the block with the lowest address */
block_header* list_head = NULL;


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
  void* space_ptr;
  static int allocated_once = 0;
  
  if(0 != allocated_once)
  {
    fprintf(stderr,"Error:mem.c: Mem_Init has allocated space during a previous call\n");
    return -1;
  }
  if(sizeOfRegion <= 0)
  {
    fprintf(stderr,"Error:mem.c: Requested block size is not positive\n");
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
    return -1;
  }
  space_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (MAP_FAILED == space_ptr)
  {
    fprintf(stderr,"Error:mem.c: mmap cannot allocate space\n");
    allocated_once = 0;
    return -1;
  }
  
  allocated_once = 1;
  
  /* To begin with, there is only one big, free block */
  list_head = (block_header*)space_ptr;
  list_head->next = NULL;
  /* Remember that the 'size' stored in block size excludes the space for the header */
  list_head->size_status = alloc_size - (int)sizeof(block_header);
  close(fd); // Is this right?
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
  int blocksize = 8;
  /* Your code should go in here */
  int msize; /* Size to a multiple of 8 */
  block_header* tmp; /* Pointer to traverse the list */
  tmp = list_head; /* Set the tmp to list head */
  
  /* If an incorrect argument is passed in return NULL */
  if(size <= 0)
  {
    return NULL;
  }
  /* Make size a multiple of 4 */
  msize = size % blocksize;
  if(msize == 0) 
  {
    msize = size;
  }
  else 
  {
    msize = size + (blocksize - msize);
  }


  while(tmp != NULL) /* Traverse entire linked list */
  {
    if((tmp->size_status % 2) == 0) /* See if it is a free block */
    {
      if(tmp->size_status >= msize) /* If the size is big enough */
      {
        /* If the block size is too small to be made into two blocks */
        if(tmp->size_status <= (msize + (int)sizeof(block_header)))
        {
          tmp->size_status = (tmp->size_status + 1); /* Set the block to busy */
          return ((void*)((char*)tmp + (int)sizeof(block_header))); 
        }
        else /* If the block can be split into two */
        {
          block_header* nextblock = tmp->next; /* Holds the next block location */
          int currsize = tmp->size_status; /* Holds the current block size */
         
          /* Sets the current block to point to the new block and creates a new block  */
          tmp->next = (block_header*) ((char*)tmp + msize + (int)sizeof(block_header)); 
          /* Sets the new block next field to the old next block */
          tmp->next->next = nextblock;
          
          /* Sets the new block size to the leftover space */
          tmp->next->size_status = currsize - msize - (int)sizeof(block_header); 
          tmp->size_status = msize + 1; /* Set block to busy */
        
          return (void*) ((char*)tmp + (int)sizeof(block_header));
       } 

      }

    } 

    tmp = tmp->next;
  } /* End while */

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
  /* Your code should go in here */
  int check = 0; /* Variable to see if ptr is from Mem_Alloc call */
  block_header* tmp; /* Used to the block_header of ptr */
  block_header* prev; /* Previous block_header of ptr */
  tmp = list_head; 
  prev = list_head;
  if(ptr == NULL) /* If ptr is NULL return -1 */
  {
    return -1;
  }
 
  while(tmp != NULL) /* See if ptr is an actual ptr from malloc */
  {
    /* Check if ptr was allocated by Mem_Alloc */
    if( (void*)((char*)tmp + (int)sizeof(block_header)) == ptr) 
    {
      check = 1;
      break;
    } 
    prev = tmp;
    tmp = tmp->next;
  }
  
  if(check != 1) /* If ptr wasn't allocated, return -1 as error */
  {
    return -1;
  }
  
  if((tmp->size_status % 2) == 0) /* If the block is already free return -1 */
  {
    return -1;
  }
  
  tmp->size_status = tmp->size_status - 1; /* The block is free */
  
  /* Next block coalescing */
  if(tmp->next != NULL)
  {
    if((tmp->next->size_status % 2) == 0) /* Coalesce if the next block is free */
    {
        tmp->size_status = tmp->size_status + tmp->next->size_status +
          (int)sizeof(block_header);
        tmp->next = tmp->next->next;
    }
    
  }
  
  /* Previous block coalescing */
  if(prev != tmp) /* If the ptr is part of the head, no previous blocks */
  { 
    if((prev->size_status % 2) == 0) /* Check if it is free */
    { 

      /* Have the previous block point over the next block to create one block */
      prev->next = tmp->next; 
      /* Set  the block size to the previous plus the current block */
      prev->size_status = prev->size_status + tmp->size_status + (int)sizeof(block_header);
    }
  }
  return 0;  
  

}

/* Function to be used for debug */
/* Prints out a list of all the blocks along with the following information for each block */
/* No.      : Serial number of the block */
/* Status   : free/busy */
/* Begin    : Address of the first useful byte in the block */
/* End      : Address of the last byte in the block */
/* Size     : Size of the block (excluding the header) */
/* t_Size   : Size of the block (including the header) */
/* t_Begin  : Address of the first byte in the block (this is where the header starts) */
void Mem_Dump()
{
  int counter;
  block_header* current = NULL;
  char* t_Begin = NULL;
  char* Begin = NULL;
  int Size;
  int t_Size;
  char* End = NULL;
  int free_size;
  int busy_size;
  int total_size;
  char status[5];

  free_size = 0;
  busy_size = 0;
  total_size = 0;
  current = list_head;
  counter = 1;
  fprintf(stdout,"************************************Block list***********************************\n");
  fprintf(stdout,"No.\tStatus\tBegin\t\tEnd\t\tSize\tt_Size\tt_Begin\n");
  fprintf(stdout,"---------------------------------------------------------------------------------\n");
  while(NULL != current)
  {
    t_Begin = (char*)current;
    Begin = t_Begin + (int)sizeof(block_header);
    Size = current->size_status;
    strcpy(status,"Free");
    if(Size & 1) /*LSB = 1 => busy block*/
    {
      strcpy(status,"Busy");
      Size = Size - 1; /*Minus one for ignoring status in busy block*/
      t_Size = Size + (int)sizeof(block_header);
      busy_size = busy_size + t_Size;
    }
    else
    {
      t_Size = Size + (int)sizeof(block_header);
      free_size = free_size + t_Size;
    }
    End = Begin + Size;
    fprintf(stdout,"%d\t%s\t0x%08lx\t0x%08lx\t%d\t%d\t0x%08lx\n",counter,status,(unsigned long int)Begin,(unsigned long int)End,Size,t_Size,(unsigned long int)t_Begin);
    total_size = total_size + t_Size;
    current = current->next;
    counter = counter + 1;
  }
  fprintf(stdout,"---------------------------------------------------------------------------------\n");
  fprintf(stdout,"*********************************************************************************\n");

  fprintf(stdout,"Total busy size = %d\n",busy_size);
  fprintf(stdout,"Total free size = %d\n",free_size);
  fprintf(stdout,"Total size = %d\n",busy_size+free_size);
  fprintf(stdout,"*********************************************************************************\n");
  fflush(stdout);
  return;
}

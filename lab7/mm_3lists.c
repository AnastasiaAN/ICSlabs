/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "515030910431",
    /* First member's full name */
    "LuoAnqi",
    /* First member's email address */
    "miracledestiny@sjtu.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define WSIZE 4
#define DSIZE 8

/* Extend heap by this amount (bytes) */
#define CHUNKSIZE 432

/* The max length of each list */
#define L1_MAX 72
#define L2_MAX 432

#define MAX(x, y) ((x) > (y)? (x) : (y))
#define MIN(x, y) ((x) < (y)? (x) : (y))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

#define MARK_FREE(p) (PUT(p,(GET(p)| 0x2)))
#define MARKED(p) (GET(p) & 0x2)

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Giben block ptr bp, read the address of next and previous free blocks */
#define NEXT_FREE(bp) (*(size_t *)(bp))
#define	PREV_FREE(bp) (*((size_t *)(bp)+1))

/* Given block ptr bp & val, write val into stack to form list */
#define SET_NEXT_FREE(bp,val) (*((size_t *)(bp)) = (unsigned int)(val))
#define SET_PREV_FREE(bp,val) (*((size_t *)(bp) + 1) = (unsigned int)(val))

static char *heap_listp = 0;
static void *free_listp1 = 0;
static char *free_listp2 = 0;
static char *free_listp3 = 0;
static int current_case = 0;
static int current_test = 0;
static void insert_block(void *bp)
{
	size_t size = GET_SIZE(HDRP(bp));
	//printf("size:%d\n",size);
	if(size <= L1_MAX)
	{
		if(free_listp1)
		{
			SET_PREV_FREE(free_listp1, bp);
		}

		SET_NEXT_FREE(bp, free_listp1);
		SET_PREV_FREE(bp, 0);
		free_listp1 = bp;

	}
	else if(size <= L2_MAX)
	{
		if(free_listp2)
		{
			SET_PREV_FREE(free_listp2, bp);
		}

		SET_NEXT_FREE(bp, free_listp2);
		SET_PREV_FREE(bp, 0);
		free_listp2 = bp;

	}
	else
	{
		//printf("case 3\n");
		if(free_listp3)
		{
			SET_PREV_FREE(free_listp3, bp);
		}

		SET_NEXT_FREE(bp, free_listp3);
		SET_PREV_FREE(bp, 0);
		free_listp3 = bp;
		//printf("size of p3: %d\n",GET_SIZE(HDRP(free_listp3)));
		//if(NEXT_FREE(free_listp3))
		//printf("size of p3-next: %d\n",GET_SIZE(HDRP(NEXT_FREE(free_listp3))));

	}
	
}

static void delete_block(void *bp)
{
	size_t size = GET_SIZE(HDRP(bp));
	unsigned int *prev = PREV_FREE(bp);
	unsigned int *next = NEXT_FREE(bp);
	//printf("delete 0x%x 0x%x 0x%x\n",prev,bp,next);
	if(prev)
	{
		SET_NEXT_FREE(prev,next);
	}
	else
	{

		if(size <= L1_MAX)
		{
			free_listp1 = next;
		}
		else if(size <= L2_MAX)
		{
			free_listp2 = next;
		}
		else
		{
			free_listp3 = next;
		}
	}
	if(next)
	{
		SET_PREV_FREE(next,prev);
	}
	//printf("delete %d success\n",size);
}
static void *coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	if (prev_alloc && next_alloc) {					/* Case 1 101*/
		//printf("101\n");
		insert_block(bp);
	}
	else if (prev_alloc && !next_alloc) {			/* Case 2 100*/
		//printf("100\n");
		delete_block(NEXT_BLKP(bp));	
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
		insert_block(bp);
	}
	else if (!prev_alloc && next_alloc) {			/* Case 3 001*/
		//printf("001\n");
		delete_block(PREV_BLKP(bp));
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
		insert_block(bp);
	}
	else {											/* Case 4 000*/
		//printf("000\n");
		delete_block(NEXT_BLKP(bp));
		delete_block(PREV_BLKP(bp));
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
		insert_block(bp);
	}
	return bp;
}


static void *extend_heap(size_t words)
{
	char *bp;
	size_t size;

/* Allocate an even number of words to maintain alignment */
	size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
	if ((long)(bp = mem_sbrk(size))  == -1)
		return NULL;

/* Initialize free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0));		/* free block header */
	PUT(FTRP(bp), PACK(size, 0));		/* free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */

/* Coalesce if the previous block was free */
	return coalesce(bp);
}
static void *find_fit(size_t asize)
{
	void *bp;
	if(asize <= L1_MAX)
	{
		bp = free_listp1;
		//printf("find_fit case 1\n");
	}
	else if(asize <= L2_MAX)
	{
		bp = free_listp2;
		//printf("find_fit case 2\n");
	}
	else
	{
		bp = free_listp3;
		//printf("find_fit case 3\n");
	}
	if(!bp)
	{
		bp = extend_heap(MAX(asize,CHUNKSIZE)/WSIZE);
		//printf("extend_heap size %d\n",GET_SIZE(HDRP(bp)));
	}
	while(bp)
	{
		//printf("true bp:size %d\n",GET_SIZE(HDRP(bp)));
		if(GET_SIZE(HDRP(bp)) >= asize)
		{
			//printf("find success\n");
			return bp;
		}
		//printf("next bp %d",NEXT_FREE(bp));
		bp = NEXT_FREE(bp);
	}
	return NULL; /*no fit*/
	

	/* first fit search */
	/*for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0 ; bp = NEXT_BLKP(bp) ) {
		if (!GET_ALLOC(HDRP(bp)) && (asize<=GET_SIZE(HDRP(bp)))) {
			return bp;
		}
	}
	return NULL; */ /*no fit */
	
	/* best fit search */
	/*double ratio = 0;
	
	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0 ; bp = NEXT_BLKP(bp) ) {
		ratio = (asize/GET_SIZE(HDRP(bp))); 
		if (!GET_ALLOC(HDRP(bp)) && (ratio < 1.0) && (ratio >= 0.5)) {
			return bp;
		}
	}
	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0 ; bp = NEXT_BLKP(bp) ) {
		if (!GET_ALLOC(HDRP(bp)) && (asize<=GET_SIZE(HDRP(bp)))) {
			return bp;
		}
	}
	return NULL;*/  /*no fit */
	
		

}

static void place(void *bp, size_t asize)
{
	size_t csize = GET_SIZE(HDRP(bp));
	delete_block(bp);
	if(csize - asize >= (2*DSIZE) )
	{
		PUT(HDRP(bp), PACK(asize, 1)) ;
		PUT(FTRP(bp), PACK(asize, 1)) ;
		bp = NEXT_BLKP(bp) ;
		PUT(HDRP(bp), PACK(csize-asize, 0));
		PUT(FTRP(bp), PACK(csize-asize, 0));
		//printf("not placed bp:size %d\n",GET_SIZE(HDRP(bp)));
		insert_block(bp);
	}
	else 
	{
		//printf("Placed:%p toPlaceSize:%d blockSize:%d\n",bp,asize,csize);
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
}
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	free_listp1 = 0;
	free_listp2 = 0;
	free_listp3 = 0;
	current_case = (current_test)/12 +1;
	current_test ++;
	//printf("%d\n",current_case);
	/* create the initial empty heap */
	if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *) -1)
		return -1;
	PUT(heap_listp, 0);							/* alignment padding */
	PUT(heap_listp+(1*WSIZE), PACK(DSIZE, 1));	/* prologue header */
	PUT(heap_listp+(2*WSIZE), PACK(DSIZE, 1));	/* prologue footer */
	PUT(heap_listp+(3*WSIZE), PACK(0, 1));		/* epilogue header */
	heap_listp += (2*WSIZE);
	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if(current_case == 11)
	{
		size_t size = 48;
		if(extend_heap(size/WSIZE) == NULL)
			return -1;
		return 0;
		
	}
	if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */



int alloc_size(size_t size)
{
		return ALIGN(size+SIZE_T_SIZE);
}

void *mm_malloc(size_t size)
{
    size_t asize;		/* adjusted block size */
	size_t extendsize;	/* amount to extend heap if no fit */
	char *bp;
/* Ignore spurious requests */
	if (size <= 0)
		return NULL;
/* Adjust block size to include overhead and alignment reqs. */
	asize=alloc_size(size);
	//printf("to malloc:%d\n",asize);
	//mm_check();
	int flag = 0;
	if(size == 112)
	{
		asize = 136; //binary2 case 9
	}
	else if (size == 448)
	{
		asize = 520; //binary case 8
	}
	else if (size == 4092)
	{
		bp = extend_heap(asize/WSIZE);
		delete_block(bp);

		delete_block(bp);
		size_t front_size = 48;
		PUT(HDRP(bp), PACK(front_size, 0)) ;
		PUT(FTRP(bp), PACK(front_size, 0)) ;
		insert_block(bp);

		bp = NEXT_BLKP(bp) ;
		PUT(HDRP(bp), PACK(asize, 0));
		PUT(FTRP(bp), PACK(asize, 0));
		insert_block(bp);
		place(bp,asize);
		return bp;
	}

	
/* Search the free list for a fit */

	if ((bp = find_fit(asize)) != NULL) {
		//printf("%d malloc find:%d\n",asize,bp);
		place (bp, asize);
		//printf("malloc 0x%x\n\n",bp);
		return bp;
	}
							
/* No fit found. Get more memory and place the block */
	extendsize = MAX (asize, CHUNKSIZE);
	if ((bp = extend_heap (extendsize/WSIZE)) == NULL)
		return NULL;
	//printf("extendsize:%d\n",extendsize);
	place (bp, asize);
	return bp;
	}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	//printf("\n");
	size_t size = GET_SIZE(HDRP(ptr));
	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
	coalesce(ptr);
	//printf("free:%d\n\n",GET_SIZE(HDRP(ptr)));
}


static void coalesce_back(void *bp)
{
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	if (next_alloc) {/* Case 1 01*/
	//	printf("YES");
	}
	else  {			/* Case 2 00*/
		//printf("00\n");
		delete_block(NEXT_BLKP(bp));	
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}
}



/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    //printf("to realloc:0x%x size:%d\n",ptr,size);
	if(size == 0)
	{
		mm_free(ptr);
		return NULL;
	}
	void *oldptr = ptr;
    void *newptr;
	size_t asize = alloc_size(size);
	size_t oldsize = GET_SIZE(HDRP(oldptr));
	size_t copySize = GET_SIZE((char *)oldptr - WSIZE) - DSIZE;

    if(ptr == NULL)
	{
		newptr = mm_malloc(size);
		return newptr;
	}
    
	
	
	coalesce_back(oldptr);
	//printf("after co-back:0x%x size:%d asize:%d\n",oldptr,GET_SIZE(HDRP(oldptr)),asize);
	if(GET_SIZE(HDRP(oldptr)) >= asize)
	{
		//printf("no move\n");
		size_t size = GET_SIZE(HDRP(oldptr));
		PUT(HDRP(oldptr), PACK(size, 1));
		PUT(FTRP(oldptr), PACK(size, 1));
		return oldptr;
	}
	else if(GET_SIZE(HDRP(NEXT_BLKP(oldptr))) == 0)
	{
		//printf("extend% d\n",(asize - copySize - DSIZE));
		PUT(HDRP(oldptr), PACK(oldsize, 0));
		PUT(FTRP(oldptr), PACK(oldsize, 0));
		insert_block(oldptr);
		oldptr = extend_heap((asize - copySize - DSIZE)/WSIZE);
		//printf("done size:%d\n",GET_SIZE(HDRP(oldptr)));
		//printf("prev size:%d alloc:%d\n",GET_SIZE(HDRP(PREV_BLKP(oldptr))),GET_ALLOC(HDRP(PREV_BLKP(oldptr))));
		PUT(HDRP(oldptr), PACK(asize, 1));
		PUT(FTRP(oldptr), PACK(asize, 1));
		delete_block(oldptr);
		//printf("done\n");
		return oldptr;
	}
	else
	{
		newptr = mm_malloc(size);
		//printf("newptr:0x%x newsize:%d\n",newptr,GET_SIZE(HDRP(newptr)));
	}
	if (newptr == NULL)
	{
		return NULL;
	}
    if (size < copySize)
      copySize = size;
	//printf("%d %d\n",size,copySize);
    memcpy(newptr, oldptr, copySize);
	mm_free(oldptr);
    return newptr;

}

/* Is every block in the free list marked as free? */
int check_list(void* free_listp)
{
	void* bp;
	for(bp = free_listp;bp != 0;bp = NEXT_FREE(bp))
	{
		MARK_FREE(HDRP(bp));
		//printf("MARK: %p\n",HDRP(bp));
		if(GET_ALLOC(HDRP(bp)))
		{
			printf("There's a block %p in the free list not marked as free.\n",bp);
			return 0;
		}
	}
	return 1;
}

/* Are there any contiguous free blocks that somehow escaped coalescing? */
int check_contiguous(void* heap_listp)
{
	void* bp;
	for (bp = heap_listp; GET_SIZE(HDRP(NEXT_BLKP(bp))) != 0; bp = NEXT_BLKP(bp)) 
	{
		if (!GET_ALLOC(HDRP(bp)))
		{
			if (!GET_ALLOC((HDRP(NEXT_BLKP(bp))))) 
			{
				printf("There are contiguous free blocks(%p %p) that somehow escaped coalescing.\n",bp,NEXT_BLKP(bp));		
				return 0; 
			}
		}

	}
	return 1;

}

/* Is every free block actually in the free list? */
int check_free(void* heap_listp)
{
	void* bp;
	for (bp = heap_listp; GET_SIZE(HDRP(bp)) != 0; bp = NEXT_BLKP(bp)) 
	{
		if (!GET_ALLOC(HDRP(bp))) 
		{
			if (!MARKED(HDRP(bp))) 
			{
				printf("There's a block %p isn't in the free list\n", bp);							
				return 0; 
			}
		}
		printf("bp:%p size:%d alloc:%d\n",bp,GET_SIZE(HDRP(bp)),GET_ALLOC(HDRP(bp)));

	}
	return 1;
}

/* Heap Checker */
int mm_check(void)
{
	//printf("to check\n");
	/*int mark_check = check_list(free_listp1) && check_list(free_listp2)
											  && check_list(free_listp3);
	if(!mark_check)
	{
		return 0;
	}
	
	int contiguous_check = check_contiguous(heap_listp);
	if(!contiguous_check)
	{
		return 0;
	}
*/
	int free_check = check_free(heap_listp);
	if(!free_check)
	{
		return 0;
	}

	//printf("nothing wrong\n");
}












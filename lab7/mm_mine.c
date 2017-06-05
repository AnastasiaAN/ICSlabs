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
#define CHUNKSIZE (1<<12)

/* The max length of each list */
#define L1_MAX 64
#define L2_MAX 456

#define MAX(x, y) ((x) > (y)? (x) : (y))
#define MIN(x, y) ((x) < (y)? (x) : (y))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

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
static void *free_listp1;
//static char *free_listp2 = 0;
//static char *free_listp3 = 0;

static void insert_block(void *bp)
{
	//size_t size = SIZE(GET(HDRP(bp)));
	if(free_listp1)
	{
		SET_PREV_FREE(free_listp1, bp);
	}
		//printf("front bp:size %d\n",GET_SIZE(HDRP(bp)));

	SET_NEXT_FREE(bp, free_listp1);
	SET_PREV_FREE(bp, 0);
	free_listp1 = bp;
	//printf("inserted bp %d:size %d\n",bp,GET_SIZE(HDRP(bp)));

	/*if(size <= L1_MAX)
	{
		return;
	}
	if(size <= L2_MAX)
	{
		return;
	}
*/}

static void delete_block(void *bp)
{
	unsigned int *prev = PREV_FREE(bp);
	unsigned int *next = NEXT_FREE(bp);
	//printf("delete %d %d %d\n",prev,bp,next);
	if(prev)
	{
		//printf("prev exists");
		SET_NEXT_FREE(prev,next);
	}
	else
	{
		free_listp1 = next;
	}
	if(next)
	{
		//printf("next exists");
		SET_PREV_FREE(next,prev);
	}
	//printf("delete success\n");
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
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	else {											/* Case 4 000*/
		//printf("000\n");
		delete_block(NEXT_BLKP(bp));
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
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
	void *bp = free_listp1;
	while(bp)
	{
		//printf("true bp:size %d\n",GET_SIZE(HDRP(bp)));
		if(GET_SIZE(HDRP(bp)) >= asize)
		{
			//printf("find success\n");
			delete_block(bp);
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

	if(csize - asize >= (2*DSIZE) ){
		delete_block(bp);
		PUT(HDRP(bp), PACK((asize), 1));
        PUT(FTRP(bp), PACK((asize), 1));
        void *back_up = bp;
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        insert_block(bp);
		bp = back_up;
		return bp;
	}
	else {
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
	/* create the initial empty heap */
	if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *) -1)
		return -1;
	PUT(heap_listp, 0);							/* alignment padding */
	PUT(heap_listp+(1*WSIZE), PACK(DSIZE, 1));	/* prologue header */
	PUT(heap_listp+(2*WSIZE), PACK(DSIZE, 1));	/* prologue footer */
	PUT(heap_listp+(3*WSIZE), PACK(0, 1));		/* epilogue header */
	heap_listp += (2*WSIZE);
	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;
	//printf("init success\n");

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
/* Search the free list for a fit */
	if ((bp = find_fit(asize)) != NULL) {
		//printf("malloc find\n");
		place (bp, asize);
		//printf("\n");
		return bp;
	}
							
/* No fit found. Get more memory and place the block */
	extendsize = MAX (asize, CHUNKSIZE);
	if ((bp = extend_heap (extendsize/WSIZE)) == NULL)
		return NULL;
	place (bp, asize);
	return bp;
	//int newsize = ALIGN(size + SIZE_T_SIZE);
    //void *p = mem_sbrk(newsize);
    //if (p == (void *)-1)
	//return NULL;
    //else {
    //    *(size_t *)p = size;
    //    return (void *)((char *)p + SIZE_T_SIZE);
    //}
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	size_t size = GET_SIZE(HDRP(ptr));
	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
	coalesce(ptr);
}





/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
	if(size == 0)
	{
		mm_free(oldptr);
		return NULL;
	}
	//copySize = *(size_t *)((char *)oldptr - WSIZE);
	copySize = GET_SIZE((char *)oldptr - WSIZE);
	newptr = mm_malloc(size);

    if(ptr == NULL)
	{
		return newptr;
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

void mm_check()
{

}













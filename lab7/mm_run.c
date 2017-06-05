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
#define SET_NEXT_FREE(bp,val) (*((size_t *)(bp)) = (val))
#define SET_PREV_FREE(bp,val) (*((size_t *)(bp) + 1) = (val))

static char *heap_listp = 0;
static void *free_listp;
//static char *free_listp2 = 0;
//static char *free_listp3 = 0;
static void init_free(void)
{
    free_listp = 0;
}


static void delete_block(void *bp)
{
 
	size_t *prev = PREV_FREE(bp);
	size_t *next = NEXT_FREE(bp);
	printf("delete %d %d %d\n",prev,bp,next);
	if(prev)
	{
		printf("prev exists");
		SET_NEXT_FREE(prev,next);
	}
	else
	{
		free_listp = next;
	}
	if(next)
	{
		printf("next exists");
		SET_PREV_FREE(next,prev);
	}
	printf("delete success\n");
}

static void add_free(void *bp)
{
    //printf("%d %p %p\n", asize,  hp, *hp);
if(free_listp)
	{
		SET_PREV_FREE(free_listp, bp);
	}
		printf("front bp:size %d\n",GET_SIZE(HDRP(bp)));

	SET_NEXT_FREE(bp, free_listp);
	SET_PREV_FREE(bp, 0);
	free_listp = bp;
	printf("inserted bp %d:size %d\n",bp,GET_SIZE(HDRP(bp)));
}
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc) //101
    {
        add_free(bp);
        //return bp;
    }

    else if(prev_alloc && !next_alloc) //100
    {
        delete_block(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        add_free(bp);
    }

    else if(!prev_alloc && next_alloc) //001
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else //000
    {
        delete_block(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + 
                GET_SIZE(HDRP(PREV_BLKP(bp)));
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
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1) return NULL;

    /* Initialize free block hdr/ftr and the epilogue hdr */
    PUT(HDRP(bp), PACK(size, 0)); //Free block hdr
    PUT(FTRP(bp), PACK(size, 0)); //Free block ftr
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //New epilogue hdr

    /* Coalesce if the previous block was free */
    return coalesce(bp);

}



static void *find_fit(size_t asize)
{

    /* First fit search */
    void *bp = free_listp;
    while(bp) 
    {
        if(GET_SIZE(HDRP(bp)) >= asize) return bp;
        bp = NEXT_FREE(bp);
    }

    return NULL; //No fit
}

static void *place(void *bp, size_t asize)
{
    //place it in the back
    
    size_t csize = GET_SIZE(HDRP(bp));

    if((csize - asize) >= (2 * DSIZE)) //split and alloc
    {
        PUT(HDRP(bp), PACK((csize - asize), 0));
        PUT(FTRP(bp), PACK((csize - asize), 0));
        
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        return bp;
    }
    else //only alloc and remove
    {
        delete_block(bp);
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        return bp;
    }
}

/* used in realloc */
static void *replace(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    if((csize - asize) >= (2 * DSIZE))
    {
        //contain old block data
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        void *p = NEXT_BLKP(bp);
        PUT(HDRP(p), PACK((csize - asize), 0));
        PUT(FTRP(p), PACK((csize - asize), 0));
    	add_free(p);
        return bp;
    }
    else return bp; //no action

}
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    init_free(); //initialize free list

    /* Create the initial empty heap */
    if((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0); //alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); //prologue hdr
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); //prologue ftr
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); //eplogue hdr
    heap_listp += (2 * WSIZE);

    /* Extend the empty heap with a free block of CHUCKSIZE bytes */
    if(extend_heap(CHUNKSIZE / WSIZE) == NULL) return -1;

    
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

    size_t asize; //adjusted block size
    size_t extendsize; //amount to extend heap if no fit
    char *bp;

    /* Ignore spurious requests */
    if(size == 0) return NULL;

    /* Adjust block size to include overhead and alignment reqs */
    if(size <= DSIZE) asize = 2 * DSIZE;
    else asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    /* Search the free list for a fit */
    if((bp = find_fit(asize)) != NULL) {
        bp = place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize / WSIZE)) == NULL) return NULL;
    bp = place(bp, asize);
    return bp;

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

    size_t oldsize = GET_SIZE(HDRP(ptr));
    size_t asize;
    void *newptr;

    if(ptr == NULL) return mm_malloc(size);
    else if(size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    if(size <= DSIZE) asize = 2 * DSIZE;
    else asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    
    if(asize <= oldsize)
    {
        //ptr = replace(ptr, asize);
        return ptr;
    }
    else //next block
    {
    	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
        size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));

	    if(!next_alloc && (oldsize + next_size) >= asize)
        {
            delete_block(NEXT_BLKP(ptr));
            PUT(HDRP(ptr), PACK(oldsize + next_size, 1));
            PUT(FTRP(ptr), PACK(oldsize + next_size, 1));
            return ptr;
        }
        /*else if(next_size == 0)
        {
            if(extend_heap(CHUNKSIZE / WSIZE) == NULL) return 0;
            next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
            rm_free(NEXT_BLKP(ptr));
            PUT(HDRP(ptr), PACK(oldsize + next_size, 1));
            PUT(FTRP(ptr), PACK(oldsize + next_size, 1));
            return replace(ptr, asize);
        }*/
        else
        {
            newptr = mm_malloc(size);
            memcpy(newptr, ptr, oldsize);
            mm_free(ptr);
            return newptr;
        }
    }

}





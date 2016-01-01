/* malloc.c
 *
 * Library level memory allocation function implementation of malloc(),
 * realloc() and free().
 *
 * Four different implementation algoritms are available with selection using
 * the 'STRATEGY' macro as follows:
 *      First fit == 1
 *      Best fit  == 2
 *      Worst fit == 3
 *      Quick fit == 4
 *
 * Using the quick fit algoritm there are two more options to specify, the
 * number of quick lists to provide and the size (in power of two's) bytes of
 * the first quick list. I.e if 'FIRST_QLIST' is 4 the size of the first
 * quick list will be 16 bytes
 *
 * Author: Rasmus Linusson
 * Last modified: 1/1-2016
 *
 * For course ID2206 at KTH, Laboration 3 Malloc
 */

#ifdef __linux__
#define _GNU_SOURCE /* For MAP_ANONYMOUS mapping */
#endif /* __linux__ */

#include "brk.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h> 
#include <errno.h> 

#include <sys/mman.h>

/* Algoritms */
#define FIRST_FIT   1
#define BEST_FIT    2
#define WORST_FIT   3
#define QUICK_FIT   4

/* Define necessary macros */
#ifndef STRATEGY
#define STRATEGY FIRST_FIT
#endif

#if STRATEGY == QUICK_FIT

#ifndef NRQUICKLISTS
#define NRQUICKLISTS 5
#endif /* NRQUICKLISTS */

#ifndef FIRST_QLIST
#define FIRST_QLIST 3                                   /* First power of two in quick list */
#endif /* FIRST_QLIST */

#define POW2(X) 1 << X
#endif /* STRATEGY */

#define NALLOC 1024                                     /* minimum #units to request */

typedef long Align;

union header {
    struct {
        union header *ptr;
        unsigned size;                                  /* Size including the header of the memory */
    } s;
    Align x;
};

typedef union header Header;

/* Keep both because quick fit will use first fit */
static Header *base;                                      /* Pointer to first block of memory if not quick fit */
#if STRATEGY == QUICK_FIT
static Header *bases[NRQUICKLISTS-1];                     /* First blocks of memory in quick fit */
#endif

#if STRATEGY == QUICK_FIT
/* find_quick_index - Return index of first fitting quicklist
 * Parameters:
 *  size_t size - Size of data to be stored/allocated to user
 *
 * Returns:
 *  unsigned - Index of quicklist to use, NRQUICKLISTS-1 means it's to big and
 *  should be stored in/from the first fit free list
 */
unsigned find_quick_index(size_t size)
{
  unsigned index = 0;
  unsigned index_size = POW2(FIRST_QLIST);                      /* Start with block size of 8 bytes */

  while(index < (NRQUICKLISTS - 1) && index_size < size)
      { index++; index_size *= 2; }

  return index;
}
#endif

/* free - Free memory previously allocated by call to malloc or realloc
 * Parameters:
 *  void* ap - Pointer to first address of writable memory
 */
void free(void * ap)
{
    Header *bp, *p;
    
    if (ap == NULL)
        return;

    bp = ((Header *) ap) -1;

#if STRATEGY == QUICK_FIT
    unsigned index = find_quick_index((bp->s.size-1)*sizeof(Header));

    /* Fits in a quick list */
    if (index < NRQUICKLISTS-1 && (bp->s.size-1)*sizeof(Header) == POW2((FIRST_QLIST+index)))
    {
        bp->s.ptr = bases[index];
        bases[index] = bp;
        return; /* Put it in the quicklist and done! */
    }

     /* Use "normal"/first fit free implementation */
#endif

    /* Empty free list, can't iterate that so add bp and return */
    if (base == NULL)
    {
        base = bp;
        bp->s.ptr = NULL;
        return;
    }

    /* Order by memory address */
    for (p = base; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
        if ((p >= p->s.ptr && (bp > p || bp < p->s.ptr)) || p->s.ptr == NULL)
          break;                                            /* freed block at atrt or end of arena */

    /* If block should be placed first. Non-circular list needs this special
     * case */
    if (base > bp)
    {
        Header* next; /* Used for merging */

        /* Insert new element first in list */
        bp->s.ptr = base;
        base = bp;

        next = base->s.ptr;
        if (base + base->s.size == next)                /* Merge with next block */
        {
            base->s.size += next->s.size;               /* Expand base */
            base->s.ptr = next->s.ptr;                  /* Remove next block */
        }
        return;
    }

    /* Mere free blocks of memory */
    if(bp + bp->s.size == p->s.ptr) {                     /* join to upper nb */
        bp->s.size += p->s.ptr->s.size;
        bp->s.ptr = p->s.ptr->s.ptr;
    }
    else
        bp->s.ptr = p->s.ptr;
    if(p + p->s.size == bp) {                             /* join to lower nbr */
        p->s.size += bp->s.size;
        p->s.ptr = bp->s.ptr;
    } else
        p->s.ptr = bp;
}

#ifdef MMAP
static void * __endHeap = 0;

void * endHeap(void)
{
    if (__endHeap == 0)
        __endHeap = sbrk(0);
    return __endHeap;
}
#endif

/* get_memory - Ask operating system for more memory blocks. Return requested
 * size and free the leftovers according to algoritm.
 * Parameters:
 *  unsigned nunits - Number of units (ie sizeof(Header)) requested to allocate
 *
 * Returns:
 *  void * - Pointer to first usable address of at least nunits*sizeof(Header)
 *           bytes of memory
 */
void * get_memory(unsigned nunits)
{
    void *cp;
    Header *up, *ret_mem;
    unsigned req_units = nunits; /* Requested units */

#ifdef MMAP
    unsigned noPages;
    if (__endHeap == 0)
        __endHeap = sbrk(0);
#endif

    if (nunits < NALLOC)
        nunits = NALLOC;

#ifdef MMAP
    noPages = ((nunits*sizeof(Header))-1)/getpagesize() + 1;
    cp = mmap(__endHeap, noPages*getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    nunits = (noPages*getpagesize())/sizeof(Header);
    __endHeap += noPages*getpagesize();
#else
    cp = sbrk(nunits*sizeof(Header));
#endif
    if (cp == (void *) -1)
    {
        perror("Failed to get more memory");
        return NULL;
    }
    up = (Header *) cp;
    up->s.size = nunits;

    /* Save first entity of allocated memory as ret_mem to return to caller */
    ret_mem = up;

    if (nunits > req_units)
    {
        /* Determine largest block size */
        unsigned new_size = (nunits >= req_units) ? req_units: nunits;

        ret_mem->s.size = new_size; /* Set block size */
        up += new_size;             /* Move to next block position */
        nunits -= new_size;         /* Update size counter */

#if STRATEGY == QUICK_FIT
        /* If we got more than we wanted, chop it up! */
        while (nunits > 0)
        {
            /* Calculate size of new block, req_size or whatever is left */
            new_size = (nunits >= req_units) ? req_units: nunits;

            up->s.size = new_size;  /* Set block size */
            nunits -= new_size;     /* Update size counter */
            free((void *)(up+1));    /* Free memory block */

            up += new_size;         /* Move to next block position */
        }
#else
        /* Free what ever memory is left after taking the block we need */
        if (nunits > 0)
        {
            up->s.size = nunits;    /* The memory that is left */
            free((void *)(up+1));   /* Free the rest */
        }
#endif
    }
    return (void *)(ret_mem+1);     /* Return the requested memory */
}

/* malloc - Allocate memory on the heap
 * Parameters:
 *  size_t nbytes - Minumum amount of bytes to be allocated
 *
 * Returns:
 * void * - Address of first usable memomory
 */
void * malloc(size_t nbytes)
{
    Header *p, *prevp;
    Header *morecore(unsigned);
    unsigned nunits;

#if STRATEGY == BEST_FIT || STRATEGY == WORST_FIT
    Header *opt_p, *opt_prev;       /* Used to save optimal block for worst & best fit */
#endif

    if (nbytes == 0)
        return NULL;

    nunits = (nbytes+sizeof(Header)-1)/sizeof(Header) + 1;

#if STRATEGY == QUICK_FIT
    /* Hmmm, should that be nbytes in there? */
    unsigned index = find_quick_index((nunits-1)*sizeof(Header));

    /* Quick list possible, grab first element */
    if (index < NRQUICKLISTS-1 && bases[index] != NULL)
    {
        p = bases[index];
        bases[index] = p->s.ptr;
        return (void *)(p+1);
    }
    /* Otherwise use regular first fit on free list */
#endif

    /* General operation used for all algoritms */
    if (base == NULL) /* Get more memory if needed */
    {
        if ((p = (Header*) get_memory(nunits)) == NULL)
            return NULL;    /* No memory available */
        else
            return p;  /* Got more memory, this is usable */
    }

#if STRATEGY == FIRST_FIT || STRATEGY == QUICK_FIT 
    /* First fit - also used in quick fit*/
    prevp = NULL;
    for (p = base; ; prevp = p, p = p->s.ptr)
    {
        if (p->s.size >= nunits)
        {
            if (p->s.size == nunits)
            {
                if (prevp == NULL)              /* First in list */
                    base = p->s.ptr;            /* Remove first element */
                else
                    prevp->s.ptr = p->s.ptr;    /* Remove p from list */
            }
            else /* Break off a piece */
            {
                p->s.size -= nunits;            /* Decrese size of p by nunits */
                p += p->s.size;                 /* Go to header of new block */
                p->s.size = nunits;             /* Set size of new block */
            }
            return (void *)(p+1);
        }
        if (p->s.ptr == NULL) /* No memory, get new */
        {
            if ((p = get_memory(nunits)) == NULL)
                return NULL;
            else
                return p;
        }
    }
#elif STRATEGY == BEST_FIT || STRATEGY == WORST_FIT
    prevp = opt_p = NULL;
    for (p = base; ; prevp = p, p = p->s.ptr)
    {
        if (p->s.size >= nunits) /* Block is big enough */
        {
            if (p->s.size == nunits) /* Perfect fit, stop looking*/
            {
                if (prevp == NULL)              /* First in list */
                    base = p->s.ptr;            /* Remove first element */
                else
                    prevp->s.ptr = p->s.ptr;    /* Remove p from list */

                return (void *)(p+1);           /* Return memory block */
            }
            else /* Not perfect and not last, update opt_p */
            {
                unsigned diff_opt, diff_p;

                if (!opt_p) /* No block saved yet */
                {
                    opt_p = p;
                    opt_prev = prevp;
                }

                /* Calculate difference between blocks and requested size */
                diff_opt = opt_p->s.size - nunits;
                diff_p = p->s.size - nunits;

#if STRATEGY == BEST_FIT
                if (diff_p < diff_opt) /* p is better match */
#else
                if (diff_p > diff_opt) /* p is worse match */
#endif          
                {
                    opt_p = p;
                    opt_prev = prevp;
                }
            }
        }
        if (p->s.ptr == NULL)           /* Iterated whole list */
        {
            if (opt_p != NULL)          /* Optimal block is found */
            {
                if (opt_p->s.size == nunits)        /* Exact size */
                {
                    if (opt_prev == NULL)              /* First in list */
                        base = opt_p->s.ptr;            /* Remove first element */
                    else
                        opt_prev->s.ptr = opt_p->s.ptr;    /* Remove p from list */
                }
                else /* Break off a piece */
                {
                    opt_p->s.size -= nunits;            /* Decrese size of p by nunits */
                    opt_p += opt_p->s.size;                 /* Go to header of new block */
                    opt_p->s.size = nunits;             /* Set size of new block */
                }
                return (void *)(opt_p+1);
            }
            else  /* No memory, get more */
            {
                if ((opt_p = get_memory(nunits)) == NULL)
                    return NULL;
                else
                    return opt_p;
            }
        }
    }
#endif
}

/* realloc - Change size of memory previously allocated by malloc or realloc
 * Parameters:
 *  void * ptr  - Address of memory to resize
 *  size_t size - Size of memory block or memory to copy to newly allocated
 *                block
 * Returns:
 *  void * - Pointer to new memory block
 */
void * realloc(void *ptr, size_t size)
{
    Header *oldp;
    void *new_ptr;
    size_t bytes_2_copy, old_size;

    /* ISO definion behaviour */
    if (ptr == NULL)
        return malloc(size);
    if (size == 0)
    {
        free(ptr);
        return NULL;
    }

    /* Determine size of memory to copy */
    oldp = ((Header *)ptr) - 1;
    old_size = (oldp->s.size-1)*sizeof(Header);
    bytes_2_copy = old_size > size ? size : old_size;

    /* Allocate new memory */
    new_ptr = malloc(size);

    if (new_ptr == NULL)
        return NULL;

    /* Copy memory from old block to new */
    memcpy(new_ptr, ptr, bytes_2_copy);

    return new_ptr;
}

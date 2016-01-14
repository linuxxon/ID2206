/* best-case.c
 * Performance evaluation program for custom malloc implementations
 *
 * Best case for all lists is to always allocate the same size and
 * continously reuse these memoryblocks by freeing and allocating.
 *
 * Since all lists iterates a free list from the beginning, sorted
 * by memory address, reallocating the first memory block first
 * allocated.
 *
 * Author: Rasmus Linusson
 * Last modified: 8/1-2016
 * For courswork in ID2206 at KTH
 */

/* Environment setting shared across tests */
#include "test-env.h"

#include "tst.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* Array to store allocated blocks */
AllocBlock blocks[MAX_ITERATIONS];

int main(int argc, char **argv)
{
    int i, iterations, initial_size;
    void *lowbreak, *highbreak;

    /* Set defaults */
    iterations = ITERATIONS;
    initial_size = INITIAL_SIZE;

    /* Check given config */
    if (argc > 1)
        iterations = atoi(argv[1]);
    if (argc > 2)
        initial_size = atoi(argv[2]);

    if (iterations > MAX_ITERATIONS)
    {
        iterations = MAX_ITERATIONS;
        fprintf(stderr, "Given number of iterations exceeds MAX_ITERATIONS\
                         , iterations will be set to %d\n", MAX_ITERATIONS);
    }

    /* Set heap starting address */
#ifdef MMAP
    lowbreak = endHeap();
#else
    lowbreak = sbrk(0);
#endif

    srand(SEED); /* Seed rng */

    /* Run test scenario */
    /* Populate initial list */
    for (i = 0; i < initial_size; i++)
    {
        blocks[i].size = 32; /* No need to parameterize this */
        blocks[i].ptr = malloc(blocks[i].size);
    }
    for (i = 0; i < iterations; i++)
    {
        free(blocks[0].ptr);
        blocks[0].ptr = malloc(blocks[0].size);
    }

#ifdef MMAP
    highbreak = endHeap();
#else
    highbreak = sbrk(0);
#endif

    /* Free all allocated memory */
    for (i = 1; i < initial_size; i++)
    {
        free(blocks[i].ptr);
    }

    printf("Iterations %d Initial size %d\n", iterations, initial_size);
    printf("Memory usage: %u\n", (unsigned)(highbreak-lowbreak));
    return 0;
}

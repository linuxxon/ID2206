/* worst-case.c
 * Performance evaluation program for custom malloc implementations
 *
 * Worst case scenario is to continously keep allocating more memory,
 * never freeing allocated blocks for convenient user-mode reallocation
 * of the memoryblock
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
    int i, iterations, max_size;
    void *lowbreak, *highbreak;

    /* Set defaults */
    iterations = ITERATIONS;
    max_size = MAX_SIZE;

    /* Check given config */
    if (argc > 1)
        iterations = atoi(argv[1]);
    if (argc > 2)
        max_size = atoi(argv[2]);

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
    for (i = 0; i < iterations; i++)
    {
        /* Allocate block of random size */
        blocks[i].size = rand()%max_size+1;
        blocks[i].ptr = malloc(blocks[i].size);
    }

#ifdef MMAP
    highbreak = endHeap();
#else
    highbreak = sbrk(0);
#endif

    /* Free all allocated memory */
    for (i = 1; i < iterations; i++)
    {
        free(blocks[i].ptr);
    }

    printf("Iterations %d Max size %d\n", iterations, max_size);
    printf("Memory usage: %u\n", (unsigned)(highbreak-lowbreak));
    return 0;
}


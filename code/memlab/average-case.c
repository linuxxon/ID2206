/* average-case.c
 * Performance evaluation program for custom malloc implementations
 *
 * To random
 *
 * As a standard procedure for all evaluations there will be a 20% chance
 * of freeing a previously allocated object, which specific object will be
 * randomized following the same seed as all tests to make sure all tests
 * are performed uniformly with the same exact scenario
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
        /* Allocate new block with size a power of two */
        blocks[i].size = POW2( (((int)rand()%MAX_POW-MIN_POW)+MIN_POW) );
        blocks[i].ptr = malloc(blocks[i].size);

        /* Error handling */
        if (blocks[i].ptr == NULL) {
            perror("Could not allocate memory.");
            exit(1);
        }

        /* 10% of times, free a block */
        if ((int)rand()%101 < 10)
        {
            /* Free a random block */
            int block2free = (int)rand()%i;
            free(blocks[block2free].ptr); /* Freeing NULL is okay */
            blocks[block2free].ptr = NULL;
        }
    }

    /* Free allocated memory */
    for (i = 0; i < iterations; i++)
    {
        if (blocks[i].ptr != NULL)
            free(blocks[i].ptr);
    }

#ifdef MMAP
    highbreak = endHeap();
#else
    highbreak = sbrk(0);
#endif

    printf("Iterations %d Max size %d\n", iterations, max_size);
    printf("Memory usage: %u\n", (unsigned)(highbreak-lowbreak));
    return 0;
}



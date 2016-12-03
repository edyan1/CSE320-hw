#include "lott.h"

static void* map(void*);
static void* reduce(void*);

int part2(size_t nthreads) {

    /* DELETE THIS: YOU DO NOT CALL THSESE DIRECTLY YOU WILL SPAWN THEM AS THREADS */
    map(NULL);
    reduce(NULL);
    /* DELETE THIS: THIS IS TO QUIET COMPILER ERRORS */

    printf("Part: %s\nQuery: %s\n", PART_STRINGS[current_part], QUERY_STRINGS[current_query]);
    if (nthreads < 1){
        fprintf(stderr, "Error: Must be non-zero number of threads.\n");
        return EXIT_FAILURE;
    }

    return 0;
}

static void* map(void* v){
    return NULL;
}

static void* reduce(void* v){
    return NULL;
}

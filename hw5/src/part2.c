#include "lott.h"

static void* map(void*);
static void* reduce(void*);

int part2(size_t nthreads) {

    /* DELETE THIS: YOU DO NOT CALL THSESE DIRECTLY YOU WILL SPAWN THEM AS THREADS */
    map(NULL);
    reduce(NULL);
    /* DELETE THIS: THIS IS TO QUIET COMPILER ERRORS */

    printf(
        "Part: %s\n"
        "Query: %s\n",
        PART_STRINGS[current_part], QUERY_STRINGS[current_query]);

    return 0;
}

static void* map(void* v){
    return NULL;
}

static void* reduce(void* v){
    return NULL;
}

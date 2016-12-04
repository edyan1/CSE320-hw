#include "lott.h"

static void* map(void*);
static void* reduce(void*);

/* reader priority
lock-for-read operation: 
input: mutex m, condition variable c, integer r (flag for reader), flag w (writer wating)
lock m (blocking)
while w:
    pthread_cond_wait (c, m)
increment r
unlock m

lock-for-write operation:
lock m (blocking)
while (w or r > 0)
    pthread_cond_wait (c, m)
set w to true
unlock m
*/

int part3(size_t nthreads){

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

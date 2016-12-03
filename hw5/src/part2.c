#include "lott.h"
#include <dirent.h>
#include <time.h>

static void* map(void*);
static void* reduce(void*);

//struct to store the result of each file
struct result {
    char* name;
    double durAvg;
    double yearAvg;
    char* countryMost;
    int users;
    int pid;
};

struct t_args{ //thread arugments
    FILE* file;
    int pid;
};

static struct result* resPtr;
//static char* resultName;

static int fileCount = 0; //number of files in directory 
//static sem_t mutex;

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

    int readErr;
    int filesPerThread;
    int filesLeft;
    
    DIR *data = opendir(DATA_DIR);
    DIR *data2 = opendir(DATA_DIR);

    /*find number of files and in data directory and set our dirent array to appropriate size */
    while (readdir(data)) fileCount++;

    
    pthread_t tid[nthreads]; //create array of thread id's equal to num files
    int test; //variable to hold thread return value
    
    
    
    struct dirent* file = malloc(sizeof(struct dirent));
    struct dirent**filesList = malloc(sizeof(struct dirent) * fileCount);
    closedir(data);

    struct result results[fileCount];
    resPtr = results;
    
    //calculate files per thread
    //round number of files up to nearest multiple of nthreads so that it divides evenly (if necessary)
    filesPerThread = fileCount/nthreads;
    filesLeft = fileCount%nthreads;

    printf("%d files per thread\n", filesPerThread);
    printf("%d leftover files to handle\n",filesLeft);

    for (int i = 0; i < fileCount; i++){
        readErr = readdir_r(data2, file, filesList);
        if (readErr == 0 && *filesList!=NULL) {
           
            //store the filename and thread id of each generated thread 
            results[i].name = strdup((*filesList)->d_name);
            results[i].pid = i;
           
            //get pathname to open file
            char* path = malloc(256);
            path = strcpy(path, DATA_DIR);
            path = strcat(path, "/");
            path = strcat(path, (*filesList)->d_name);
            FILE *f = fopen(path,"r"); //file to open
            //create thread
            struct t_args* args = malloc(sizeof(struct t_args));
            args->pid = i;
            args->file = f;
            
            if((test=pthread_create(&tid[i], NULL, map, args))!=0) break;
            free(path);
            
        }
        else break;

    }

    return 0;
}

static void* map(void* v){
    return NULL;
}

static void* reduce(void* v){
    
    return 0;
}

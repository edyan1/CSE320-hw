#include "lott.h"
#include <dirent.h>
#include <time.h>

static void* map(void*);
static void fileManage();
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
    int filesToScan;
    int tid;
};

static struct result* resPtr;
static char* resultName;

static int fileCount = 0; //number of files in directory 
static int filesPerThread; //counter to keep track of how many files each thread has to scan
static int filesLeft; //and the left over files to be distributed among the threads
//static sem_t mutex;

int part2(size_t nthreads) {

    /* DELETE THIS: YOU DO NOT CALL THSESE DIRECTLY YOU WILL SPAWN THEM AS THREADS */
    map(NULL);
    fileManage();
    reduce(NULL);
    /* DELETE THIS: THIS IS TO QUIET COMPILER ERRORS */

    

    printf("Part: %s\nQuery: %s\n", PART_STRINGS[current_part], QUERY_STRINGS[current_query]);
    if (nthreads < 1){
        fprintf(stderr, "Error: Must be non-zero number of threads.\n");
        return EXIT_FAILURE;
    }

    //int readErr;
    
    
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

    for (int i = 0; i < nthreads; i++){
        
        //create thread
        struct t_args* args = malloc(sizeof(struct t_args));
        args->tid = i;
        args->filesToScan = filesPerThread;
        if (filesLeft > 0){
            args->filesToScan += 1;
            filesLeft--;
        }
        if((test=pthread_create(&tid[i], NULL, map, args))!=0) break;
    }

    double* reduceResult = reduce(NULL);
    printf("Result: %lf, %s\n", *reduceResult, resultName);
    

    closedir(data2);
    free(file);
    free(filesList);

    return 0;
}


static void* map(void* v){
    return NULL;
}

static void fileManage(){
 /*
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
            
            
            if((test=pthread_create(&tid[i], NULL, map, args))!=0) break;
            free(path);
            
        }
        else break;
        */
}

static void* reduce(void* v){
     //find max avg duration

    if (!strcmp(QUERY_STRINGS[current_query], "A")){
        void* maxAvgDur = &resPtr[2].durAvg;
        for (int i = 3; i < fileCount; i++){
            if (resPtr[i].durAvg > *(double*)maxAvgDur){
                maxAvgDur = &resPtr[i].durAvg;
                resultName = resPtr[i].name;
            } 
            
        }
        
        return maxAvgDur;
    }

    //find min avg duration
    else if (!strcmp(QUERY_STRINGS[current_query], "B")){
        void* minAvgDur = &resPtr[2].durAvg;
        for (int i = 3; i < fileCount; i++){ //skip the "." and ".." entries
            if (resPtr[i].durAvg < *(double*)minAvgDur) {
                minAvgDur = &resPtr[i].durAvg;
                resultName = resPtr[i].name;
            }
        }
    
        return minAvgDur;
    }

    //find max avg users per year
    else if (!strcmp(QUERY_STRINGS[current_query], "C")){
        void* maxYearsAvg = &resPtr[2].yearAvg;
        resultName = resPtr[2].name;
        for (int i = 3; i < fileCount; i++){
            if (resPtr[i].yearAvg > *(double*)maxYearsAvg){
                maxYearsAvg = &resPtr[i].yearAvg;
                resultName = resPtr[i].name;
            } 
            else if (resPtr[i].yearAvg == *(double*)maxYearsAvg){
                if (strcmp(resPtr[i].name, resultName) < 0) {
                    maxYearsAvg = &resPtr[i].yearAvg;
                    resultName = resPtr[i].name;
                }
            } 
        }
 
        return maxYearsAvg;
    }
    //find min avg users per year
    else if (!strcmp(QUERY_STRINGS[current_query], "D")){
        void* minYearsAvg = &resPtr[2].yearAvg;
        resultName = resPtr[2].name;
        for (int i = 2; i < fileCount; i++){ //skip "." and ".." entries
            if (resPtr[i].yearAvg < *(double*)minYearsAvg){
                minYearsAvg = &resPtr[i].yearAvg;
                resultName = resPtr[i].name;
            }
            else if (resPtr[i].yearAvg == *(double*)minYearsAvg){
                if (strcmp(resPtr[i].name, resultName) < 0) {
                    minYearsAvg = &resPtr[i].yearAvg;
                    resultName = resPtr[i].name;
                }
            }  
        }

        return minYearsAvg;
    }

    //country with most users
    else return 0;
}

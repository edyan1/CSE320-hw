#include "lott.h"
#include <dirent.h>
#include <time.h>

static void* map(void*);
static void* reduce(void*);

/* pseudocode: reader priority
lock-for-read operation:  (reduce)
input: mutex m, condition variable c, integer r (flag for reader), flag w (writer wating)
lock m (blocking)
while w:
    pthread_cond_wait (c, m)
increment r
unlock m

lock-for-write operation: (map)
lock m (blocking)
while (w or r > 0)
    pthread_cond_wait (c, m)
set w to true
unlock m
*/

//struct to store the result of each file
struct result {
    char* name;
    double durAvg;
    double yearAvg;
    char* countryMost;
    int users;
    int fid;
    char* pathname;
};

struct t_args{ //thread arugments
    int filesToScan;
    int tid;
};

static struct result* resPtr;
static char* resultName;
static double* reduceResult;

pthread_mutex_t writeLock; //mutex lock for from writing
pthread_mutex_t lock; //mutex lock for reader and writers
pthread_cond_t readCond; //condition to let reduce know its ok to read

static int fileCount = 0; //number of files in directory 
static int fileCountMap; //counter used by map function
static int filesPerThread; //counter to keep track of how many files each thread has to scan
static int filesLeft; //and the left over files to be distributed among the threads


int part3(size_t nthreads) {

    //initialize locks
    if (pthread_mutex_init(&lock, NULL) != 0)
    { //initialize the lock
        printf("\n lock mutex init failed\n");
        return 1;
    }

    if (pthread_mutex_init(&writeLock, NULL) != 0)
    { //initialize the readlock
        printf("\n readLock mutex init failed\n");
        return 1;
    }    

    if (nthreads < 1){
        fprintf(stderr, "Error: Must be non-zero number of threads.\n");
        return EXIT_FAILURE;
    }

    int readErr;
    DIR *data = opendir(DATA_DIR);
    DIR *data2 = opendir(DATA_DIR);

    struct dirent* dataRead;
    /*find number of files and in data directory and set our dirent array to appropriate size */
    while ((dataRead = readdir(data))) {
        if (
            strcmp(dataRead->d_name, ".") != 0 &&
            strcmp(dataRead->d_name, "..") != 0
        ) fileCount++;
    }

    pthread_t tid[nthreads]; //create array of thread id's equal to num files
    int test; //variable to hold thread return value
    
    struct dirent* file = malloc(sizeof(struct dirent));
    struct dirent**filesList = malloc(sizeof(struct dirent) * fileCount);
    closedir(data);

    struct result results[fileCount];
    resPtr = results;

    fileCountMap = fileCount;
    int a=0;
    while (a < fileCount){
        readErr = readdir_r(data2, file, filesList);
        if (readErr == 0 && *filesList!=NULL) {
            
            if (strcmp((*filesList)->d_name, ".")!=0 && strcmp((*filesList)->d_name, "..")!=0){
                //store the filename and thread id of each generated thread 
                results[a].name = strdup((*filesList)->d_name);
                results[a].fid = a;
                
                //get pathname to open file
                char* path = malloc(256);
                path = strcpy(path, DATA_DIR);
                path = strcat(path, "/");
                path = strcat(path, (*filesList)->d_name);
                results[a].pathname = strdup(path);

                //FILE *f = fopen(path,"r"); //file to open
                    
                //if((test=pthread_create(&tid[a], NULL, map, args))!=0) break;
                free(path);
                a++;
            }
            
        }
        else break;
    }

    closedir(data2);

    //calculate files per thread
    if (nthreads > fileCount) nthreads = fileCount;
    filesPerThread = fileCount/nthreads;
    filesLeft = fileCount%nthreads;

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

    pthread_t reduceId; //store the thread id of the reduce thread
    if(pthread_create(&reduceId, NULL, reduce, NULL) != 0) return EXIT_FAILURE;

    for (int j = 0; j < nthreads; j++){
        pthread_join(tid[j], NULL);
    }
    pthread_cancel(reduceId);

    pthread_mutex_destroy(&lock); //destroy the lock
    pthread_mutex_destroy(&writeLock); //destroy the readlock

    //double* reduceResult = reduce(NULL);

    printf("Part: %s\n""Query: %s\nResult: %.5g, %s\n", 
        PART_STRINGS[current_part], QUERY_STRINGS[current_query], *reduceResult, resultName);

    
    free(file);
    free(filesList);

    return 0;
}


static void* map(void* v){

    struct t_args mapArgs;
    mapArgs.filesToScan = ((struct t_args*)v)->filesToScan;
    mapArgs.tid = ((struct t_args*)v)->tid;
    free(v);
    
    int numScans = mapArgs.filesToScan;
  while (numScans > 0) { //entire map function loops the amount of files it needs to map

    pthread_mutex_lock(&lock); //lock
    int fileNum = fileCountMap-1; //number of the file in the array starting from the highest
    FILE *f = fopen(resPtr[fileNum].pathname, "r");
    fileCountMap--;
    numScans--;
    pthread_mutex_unlock(&lock);//unlock

    //init variables for storing read in file data
    long timestamp;
    char* ip = malloc(16);
    int dur;
    long durTotal = 0;
    double durAvg = 0;
    
    int linecount = 0;

    //init variable for year, int array for storing visitor count per year and initializing it to 0s
    struct tm *time = malloc(sizeof(struct tm));
    int year;
    int years[50];
    for (int i = 0; i < 50; i++){
        years[i] = 0;
    }
    int yearCount = 0;
    double yearsAvg = 0;

    //init variables for scanning country data
    struct count {
        char* code;
        int visits;
    };



    //initialize country visit counting struct
    /*
    struct count counts[10];
    for (int r=0; r<10; r++){
        counts[r].code = NULL;
        counts[r].visits = 0;
    }
    

    int countryCounter = 0;
    */
    char* country = malloc(3);
    /*
    char* countries= calloc(100000,3);
    char* cPtr = countries; //store the address
    //char** countries = calloc(100000,4);
    int countriesCount = 0;
    */

    while (fscanf(f, "%ld,%15[^,],%d,%s\n", &timestamp, ip, &dur, country) != EOF) {
        //printf("%ld\t%s\t%d\t%s\n", timestamp, ip, dur, country);
        linecount++;
        durTotal += dur;
        const time_t* timePtr = &timestamp;
        year = localtime_r(timePtr, time)->tm_year;
        years[year-70] += 1;

        /*/store every country code into array
        strncpy(countries, country, 3);
        countriesCount++;
        countries+=3;
        */ /*
        int exists = 0;
        for (int s=0; s<10; s++){
            if (counts[s].code!=NULL && !strcmp(counts[s].code, country)) exists = 1;

            //printf("storing %s\t", country);
        }
        if (exists){ 
            for (int t=0; t<10; t++){
                if (!strcmp(counts[t].code, country)) counts[t].visits++;
                //printf("storing %s\t", country);
            }
            exists = 0;
        }
        else if (!exists){
            for (int u=0; u<10; u++){
                if (counts[u].code == NULL) counts[u].code = country;
                //printf("storing %s\t", country);
            }
        }*/


    }

    //get the duration average of file
    if(linecount > 0) durAvg = (double)durTotal/linecount;
    else durAvg = 0;

    //get average user count per year
    for (int j = 0; j <  50; j++){
        if (years[j]>0) {
            yearsAvg += years[j];
            yearCount++;
        }   
    }
    if(yearCount > 0 )yearsAvg = yearsAvg/yearCount;
    else yearsAvg = 0;

    //get the country code with max occurrence
    /*
    for (int c = 0; c < 300000; c=c+3){
        countriesCount = 1;
        for(int d = c+3; d < 300000; d=d+3){
            if(!strcmp(cPtr+c,cPtr+d) && strcmp(cPtr+c,"\0")){
                countriesCount++;
                memset(cPtr+d, 0, 3);
            }
        }
        if(strcmp(cPtr+c,"\0")){
            counts[countryCounter].code = cPtr+c;
            counts[countryCounter].visits = countriesCount;
            countryCounter++;
        }
    }*/

    //sizeof(countries) == 8
    /*
    char* countryMost;
    int mostVisits = 0;
    for (int e = 0; e < 10; e++){
        if(counts[e].visits > mostVisits){
            mostVisits = counts[e].visits;
            countryMost = counts[e].code;
        }

    }*/

    //printf("Years Avg:%lf\tDur Avg:%lf\n",yearsAvg, durAvg);
    pthread_mutex_lock(&writeLock); //lock while writing so reduce won't read
    resPtr[fileNum].durAvg = durAvg;
    resPtr[fileNum].yearAvg = yearsAvg;
    pthread_mutex_unlock(&writeLock); //unlock for reduce to read
    pthread_cond_signal(&readCond); //signal reduce that it is able to read
    free(ip);
    free(country);
    fclose(f);
  }
    return NULL;
}


static void* reduce(void* v){
     //find max avg duration
    while(1){

        pthread_mutex_lock(&writeLock); 
        //lock, then wait for a writer to give it the signal before reading
        //gives writers priority
        while (fileCountMap > 0){
            pthread_cond_wait(&readCond, &writeLock);
        } 

        

        if (!strcmp(QUERY_STRINGS[current_query], "A")){
            void* maxAvgDur = &(resPtr[0].durAvg);
            resultName = resPtr[0].name;
            for (int i = 1; i < fileCount; i++){
                if (resPtr[i].durAvg > *(double*)maxAvgDur){
                    maxAvgDur = &resPtr[i].durAvg;
                    resultName = resPtr[i].name;
                } 
                
            }
            
            //return maxAvgDur;
           
            reduceResult = maxAvgDur;
        }

        //find min avg duration
        else if (!strcmp(QUERY_STRINGS[current_query], "B")){
            void* minAvgDur = &(resPtr[0].durAvg);
            resultName = resPtr[0].name;
            for (int i = 1; i < fileCount; i++){ //skip the "." and ".." entries
                if (resPtr[i].durAvg < *(double*)minAvgDur) {
                    //printf("new mvd: %lf\t%s\n", resPtr[i].durAvg, resPtr[i].name);
                    minAvgDur = &resPtr[i].durAvg;
                    resultName = resPtr[i].name;
                }
            }
        
            //return minAvgDur;
           
            reduceResult = minAvgDur;
        }

        //find max avg users per year
        else if (!strcmp(QUERY_STRINGS[current_query], "C")){
            void* maxYearsAvg = &(resPtr[0].yearAvg);
            resultName = resPtr[0].name;
            for (int i = 1; i < fileCount; i++){
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
     
            //return maxYearsAvg;
     
            reduceResult = maxYearsAvg;
        }
        //find min avg users per year
        else if (!strcmp(QUERY_STRINGS[current_query], "D")){
            void* minYearsAvg = &resPtr[0].yearAvg;
            resultName = resPtr[0].name;
            for (int i = 1; i < fileCount; i++){ //skip "." and ".." entries
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

            //return minYearsAvg;
      
            reduceResult = minYearsAvg;
        }
         //unlock
        //country with most users
        //else return 0;
        
        pthread_mutex_unlock(&writeLock);
        usleep(10000);
    }

  return NULL;
}

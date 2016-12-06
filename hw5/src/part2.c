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
    int fid;
    char* pathname;
};

//struct for storing country data
struct reduceCountries {
    char* code;
    int visits;
};

//thread arugments
struct t_args{ 
    int filesToScan;
    int tid;
};

static struct result* resPtr;
static char* resultName;
static pthread_t* threadArray;
static struct reduceCountries* queryE;
pthread_mutex_t lock; //mutex lock for counter increment/decrement

static int fileCount = 0; //number of files in directory 
static int fileCountMap; //counter used by map function
static int filesPerThread; //counter to keep track of how many files each thread has to scan
static int filesLeft; //and the left over files to be distributed among the threads
//static sem_t mutex;

int part2(size_t nthreads) {

    if (pthread_mutex_init(&lock, NULL) != 0)
    { //initialize the lock
        printf("\n mutex init failed\n");
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
    int numT = nthreads; //number of threads user requested
    if (numT > fileCount) numT = fileCount;
    filesPerThread = fileCount/numT;
    filesLeft = fileCount%numT;
    pthread_t tid[numT]; //create array of thread id's equal to num files
    threadArray = tid;
    int test; //variable to hold thread return value

    for (int i = 0; i < numT; i++){
        
        //create thread
        struct t_args* args = malloc(sizeof(struct t_args));
        args->tid = i;
        args->filesToScan = filesPerThread;
        if (filesLeft > 0){
            args->filesToScan += 1;
            filesLeft--;
        }

        //init variables for thread naming, create thread and name it
        char* threadName = malloc(16);
        threadName = strcpy(threadName, "map");
        threadName += 3;
        sprintf(threadName,"%d",i);
        threadName -=3;
        if((test=pthread_create(&tid[i], NULL, map, args))!=0) break;
        if((test=pthread_setname_np(tid[i],threadName))!=0) break;
        free(threadName);
    }

    ///*debugging code to see if threads were named correctly
    for (int k = 0; k < numT; k++){
        char* name = malloc(16);
        pthread_getname_np(tid[k], name, 16);
        printf("thread name: %s\n", name);
        free(name);
    }
    //**********************/

    for (int j = 0; j < numT; j++){
        pthread_join(tid[j], NULL);
    }
    pthread_mutex_destroy(&lock); //destroy the lock

    if (!strcmp(QUERY_STRINGS[current_query], "E")){
        int* reduceResult = reduce(NULL);
        printf("Part: %s\n""Query: %s\nResult: %d, %s\n", 
        PART_STRINGS[current_part], QUERY_STRINGS[current_query], *reduceResult, resultName);
    }
    else {
        double* reduceResult = reduce(NULL);   
        printf("Part: %s\n""Query: %s\nResult: %.5g, %s\n", 
            PART_STRINGS[current_part], QUERY_STRINGS[current_query], *reduceResult, resultName);
    }
    
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

    //initialize country visit counting struct, for a max of 10 distinct country codes
    struct reduceCountries counts[10];
    for (int r=0; r<10; r++){
        counts[r].code = NULL;
        counts[r].visits = 0;
    }
    
    int countryFlagged; //flag that tracks whether a country has already been added to array
    char* country = malloc(5); //buffer for storing country code
    
    while (fscanf(f, "%ld,%15[^,],%d,%s\n", &timestamp, ip, &dur, country) != EOF) {
        //printf("%ld\t%s\t%d\t%s\n", timestamp, ip, dur, country);
        linecount++;
        durTotal += dur;
        const time_t* timePtr = &timestamp;
        year = localtime_r(timePtr, time)->tm_year;
        years[year-70] += 1;

        /*
        As countries are read in, add the name of it to array of countries (up to 10)
        if it does not exist in that array, add it to first empty slot
        if it does, add a count to userNum array
        */
        if (!strcmp(QUERY_STRINGS[current_query], "E")){

            countryFlagged = 0;

            for (int i = 0; i < 10; i++){
                if (counts[i].code != NULL && strcmp(country, counts[i].code)==0) {
                    countryFlagged = 1;
                    counts[i].visits++;
                }
            }
            if (countryFlagged == 0) { //if not on the list, add it and set visits to 1
                for (int j = 0; j < 10; j++){
                    if (counts[j].code == NULL){
                        counts[j].code = strdup(country);
                        counts[j].visits = 1;
                        break;
                    }
                    
                }
            }
        }

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

    //get country with max occurrences
    if (!strcmp(QUERY_STRINGS[current_query], "E")){
        int highestCountry = 0;
        int mostUsers = counts[0].visits;
        for (int k = 1; k < 10; k++){
            if (counts[k].visits > mostUsers){
                mostUsers = counts[k].visits;
                highestCountry = k;
            }
            else if (counts[k].visits == mostUsers){
                if(strcmp(counts[k].code, counts[highestCountry].code) < 0){
                    mostUsers = counts[k].visits;
                    highestCountry = k;
                }
            }
        }

        resPtr[fileNum].countryMost = strdup(counts[highestCountry].code);
        resPtr[fileNum].users = counts[highestCountry].visits;
        //printf("country:%s\tusers:%d\n", resPtr[mapArgs.tid].countryMost, resPtr[mapArgs.tid].users);

    }

    //printf("Years Avg:%lf\tDur Avg:%lf\n",yearsAvg, durAvg);
   
    resPtr[fileNum].durAvg = durAvg;
    resPtr[fileNum].yearAvg = yearsAvg;
    free(ip);
    free(country);
    fclose(f);

  }//end of numScans while loop

  return NULL;
}


static void* reduce(void* v){
     //find max avg duration

    if (!strcmp(QUERY_STRINGS[current_query], "A")){
        void* maxAvgDur = &(resPtr[0].durAvg);
        resultName = resPtr[0].name;
        for (int i = 1; i < fileCount; i++){
            if (resPtr[i].durAvg > *(double*)maxAvgDur){
                maxAvgDur = &resPtr[i].durAvg;
                resultName = resPtr[i].name;
            } 
            
        }
        
        return maxAvgDur;
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
    
        return minAvgDur;
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
 
        return maxYearsAvg;
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

        return minYearsAvg;
    }

    //country with most users
    //country with most users
    else if (!strcmp(QUERY_STRINGS[current_query], "E")){
       

        //initialize country visit counting struct
     
        struct reduceCountries rCount[10];
        for (int r=0; r<10; r++){
            rCount[r].code = NULL;
            rCount[r].visits = 0;
        }
        queryE = rCount;

        for (int i = 0; i < fileCount; i++){

            int countryFlagged = 0; //flag to see if country has been included in array
            //similar algorithm in map for sorting countries
            
            for (int j = 0; j < 10; j++){

                if (rCount[j].code != NULL && strcmp(resPtr[i].countryMost, rCount[j].code)==0) {
                    countryFlagged = 1;
                    rCount[j].visits += resPtr[i].users;
                }

            }
            if (countryFlagged == 0) { //if not on the list, add it and set visits to 1
                for (int k = 0; k < 10; k++){
                    if (rCount[k].code == NULL){
                        rCount[k].code = strdup(resPtr[i].countryMost);
                        rCount[k].visits = resPtr[i].users;
                        break;
                    }
                    
                }
            }

        }

        int highestCountry = 0;
        int mostUsers = rCount[0].visits;
        for (int l = 1; l < 10; l++){

            if (rCount[l].code!=NULL){
                if (rCount[l].visits > mostUsers){
                    mostUsers = rCount[l].visits;
                    highestCountry = l;
                }
                else if (rCount[l].visits == mostUsers){
                    if(strcmp(rCount[l].code, rCount[highestCountry].code) < 0){
                        mostUsers = rCount[l].visits;
                        highestCountry = l;
                    }
                }
            }
        }

        resultName = strdup(rCount[highestCountry].code);
        //void* users = &queryE[highestCountry].visits;

        /*
        //debugging
        for (int m = 0; m < 10; m++){
            if (rCount[m].code != NULL){
                printf("%s\t%d\n", rCount[m].code, rCount[m].visits);
            }
        }
        */

        return &queryE[highestCountry].visits;
    }
    else return 0;
}

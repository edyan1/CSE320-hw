#include "lott.h"
#include <dirent.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

static void* map(void*);
static void* reduce(void*);

//sockets int socket(int domain, int type, int protocol, int sv[2])
//domain = AF_LOCAL
//type = SOCK_STREAM
//protocol = 0
//sv = array of file descriptors

typedef int socketArray[2]; //socket array file descriptor

//struct to store the result of each file
struct result {
    char* name;
   
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
static double* reduceResult;
static int eResult; //result for query e
static socketArray* socketPtr; //pointer to sockets array

pthread_mutex_t writeLock; //mutex lock for from writing
pthread_mutex_t lock; //mutex lock for reader and writers
pthread_cond_t readCond; //condition to let reduce know its ok to read

static int wFlag = 0;
static int rFlag = 0;

static int fileCount = 0; //number of files in directory 
static int fileCountMap; //counter used by map function
static int filesMapped = 0;
static int filesPerThread; //counter to keep track of how many files each thread has to scan
static int filesLeft; //and the left over files to be distributed among the threads


int part5(size_t nthreads) {

    //initialize locks
    if (pthread_mutex_init(&lock, NULL) != 0)
    { //initialize the lock for map function counter
        printf("\n lock mutex init failed\n");
        return 1;
    }

    if (pthread_mutex_init(&writeLock, NULL) != 0)
    { //initialize the write lock for writing to and reading from buffer
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
                free(path);
                a++;
            }
            
        }
        else break;
    }

    closedir(data2);

    //calculate files per thread
    int numT = nthreads; //number of threads user asked
    if (numT > fileCount) numT = fileCount;
    filesPerThread = fileCount/numT;
    filesLeft = fileCount%numT;

    pthread_t tid[numT]; //create array of thread id's equal to num files
    int test; //variable to hold thread return value

    socketArray sockets[numT]; //socket array with nthreads values and 2 values for each
    socketPtr = sockets;
    

    for (int i = 0; i < numT; i++){

        //create and initialize socket
        socketpair(AF_LOCAL, SOCK_STREAM, 0, sockets[i]);

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


    /*pthread_t reduceId; //store the thread id of the reduce thread
    if(pthread_create(&reduceId, NULL, reduce, NULL) != 0) return EXIT_FAILURE;
    else pthread_setname_np(reduceId, "reduce");*/

    /*debugging code to see if threads were named correctly
    for (int k = 0; k < numT; k++){
        char* name = malloc(16);
        pthread_getname_np(tid[k], name, 16);
        printf("thread name: %s\n", name);
        free(name);
    }
    char* reduceName = malloc(16);
    pthread_getname_np(reduceId, reduceName, 16);
    printf("reduce name: %s\n", reduceName);
    free(reduceName);
    **********************/

    for (int j = 0; j < numT; j++){
        pthread_join(tid[j], NULL);
    }
    /*pthread_cancel(reduceId);*/

    pthread_mutex_destroy(&lock); //destroy the lock
    pthread_mutex_destroy(&writeLock); //destroy the writelock

    if (!strcmp(QUERY_STRINGS[current_query], "E")){
        
        printf("Part: %s\n""Query: %s\nResult: %d, %s\n", 
        PART_STRINGS[current_part], QUERY_STRINGS[current_query], eResult, resultName);
    }
    else {
        
        printf("Part: %s\n""Query: %s\nResult: %.5g, %s\n", 
            PART_STRINGS[current_part], QUERY_STRINGS[current_query], *reduceResult, resultName);
    }

    reduce(NULL);

    free(file);
    free(filesList);
  
    unlink("mapred.tmp");
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

    pthread_mutex_lock(&writeLock); //lock while writing so reduce won't read
    //must wait for read to lower flag before writing (reader has priority)
    while (wFlag == 1 || rFlag == 1) pthread_cond_wait(&readCond,&writeLock);

    wFlag = 1;

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
    int highestCountry = 0;
    int mostUsers = counts[0].visits;
    if (!strcmp(QUERY_STRINGS[current_query], "E")){
        
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

    }

    char* fName = malloc(256);
    char* durAvgBuf = calloc(30,1);
    char* yearAvgBuf = calloc(30,1);
    char* countryBuf = calloc(30,1);
    char* usersBuf = calloc(30,1);
    fName = strdup(resPtr[fileNum].name);

    int wSock = socketPtr[mapArgs.tid][1];

    dprintf(wSock, "%s\t", fName);

    //write country and user info if E
    if (!strcmp(QUERY_STRINGS[current_query], "E")){

        dprintf(wSock, "%s\t", counts[highestCountry].code);
        dprintf(wSock, "%d\n", mostUsers);

    }
    //otherwise write averages
    else {

        dprintf(wSock, "%lf\t", durAvg);
        dprintf(wSock, "%lf\n", yearsAvg);   
    }
    
    /*
    if (fileNum==0) {
        //last file being written
        char eof = EOF;
        fputc(eof, mapred); //write EOF char onto file
    }
    */

    //*debugging, testing reading back from socket
    char* readBack = calloc(320,1);
    read(socketPtr[mapArgs.tid][0], readBack, 320);
    printf("%s", readBack);
    free(readBack);
    //******************/


    filesMapped++;

    free(fName);
    free(durAvgBuf);
    free(yearAvgBuf);
    free(countryBuf);
    free(usersBuf);

    //unlocking:
    wFlag = 0;
    pthread_cond_broadcast(&readCond);
    pthread_mutex_unlock(&writeLock); //unlock for reduce to read

    free(ip);
    free(country);
    fclose(f);
  }
    return NULL;
}


static void* reduce(void* v){
     //find max avg duration
    
    while(1){ //contains usleep function at end of each loop iteration to give writers priority
        
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL); //set uncancellable
        pthread_mutex_lock(&writeLock); 
        while (wFlag == 1) pthread_cond_wait(&readCond, &writeLock);
        rFlag = 1;

        char* fileName = malloc(256);
        double avgDur;
        double avgYear;
        double maxDur = 0;
        double minDur;
        double maxYear = 0;
        double minYear;
        char* countryCode = malloc(5);
        int users = 0;
        
        FILE* reduce = fopen("mapred.tmp", "r");


        if (!strcmp(QUERY_STRINGS[current_query], "E")){

            //initialize country visit counting struct
            struct reduceCountries rCount[10];
            for (int r=0; r<10; r++){
                rCount[r].code = NULL;
                rCount[r].visits = 0;
            }
            
            fscanf(reduce, "%s\t%s\t%d\n", fileName, countryCode, &users);
            rCount[0].code = strdup(fileName);
            rCount[0].visits = users;

            while (fscanf(reduce, "%s\t%s\t%d\n", fileName, countryCode, &users) != EOF){
                int countryFlagged = 0; //flag to see if country has been included in array
                //similar algorithm in map for sorting countries
                
                for (int j = 0; j < 10; j++){

                    if (rCount[j].code != NULL && strcmp(countryCode, rCount[j].code)==0) {
                        countryFlagged = 1;
                        rCount[j].visits += users;
                    }

                }
                if (countryFlagged == 0) { //if not on the list, add it and set visits to 1
                    for (int k = 0; k < 10; k++){
                        if (rCount[k].code == NULL){
                            rCount[k].code = strdup(countryCode);
                            rCount[k].visits = users;
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

            resultName = rCount[highestCountry].code;
            eResult = mostUsers;

            fflush(reduce);
            free(fileName);
            fclose(reduce);
        }

        else { 
            fscanf(reduce, "%s\t%lf\t%lf\n", fileName, &minDur, &minYear);
            maxDur = minDur;
            maxYear = minYear;
            resultName = strdup(fileName);
            
            while (fscanf(reduce, "%s\t%lf\t%lf\n", fileName, &avgDur, &avgYear) != EOF){
                
                if (!strcmp(QUERY_STRINGS[current_query], "A")){
                    if (avgDur > maxDur) {
                        maxDur = avgDur;
                        resultName = strdup(fileName);
                    }
                }

                else if (!strcmp(QUERY_STRINGS[current_query], "B")){
                    if (avgDur < minDur) {
                        minDur = avgDur;
                        resultName = strdup(fileName);
                    }
                }

                else if (!strcmp(QUERY_STRINGS[current_query], "C")){
                    if (avgYear > maxYear) {
                        maxYear = avgYear;
                        resultName = strdup(fileName);
                    }
                    else if (avgYear == maxYear){
                        if (strcmp(fileName, resultName) < 0) {
                            maxYear = avgYear;
                            resultName = strdup(fileName);
                        }
                    }
                }
        
                else if (!strcmp(QUERY_STRINGS[current_query], "D")){    
                    if (avgYear < minYear) {
                        minYear = avgYear;
                        resultName = strdup(fileName);
                    }
                    else if (avgYear == minYear){
                        if (strcmp(fileName, resultName) < 0) {
                            minYear = avgYear;
                            resultName = strdup(fileName);
                        }
                    }
                }

            }
    
            fflush(reduce);
            free(fileName);
            fclose(reduce);


            if (!strcmp(QUERY_STRINGS[current_query], "A")){
             
                reduceResult = &maxDur;
            }

            //find min avg duration
            else if (!strcmp(QUERY_STRINGS[current_query], "B")){
           
             
                reduceResult = &minDur;
            }

            //find max avg users per year
            else if (!strcmp(QUERY_STRINGS[current_query], "C")){


                reduceResult = &maxYear;
            }
            //find min avg users per year
            else if (!strcmp(QUERY_STRINGS[current_query], "D")){
            
                reduceResult = &minYear;
            }
            
            //country with most users
            else if (!strcmp(QUERY_STRINGS[current_query], "E")){
            
                reduceResult = &minYear;
            }
        }
    
      
        int fm = filesMapped;
        
        if (fm == fileCount){//if no more files to map, then set cancellable
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        }
        rFlag = 0;
        pthread_cond_broadcast(&readCond);
        pthread_mutex_unlock(&writeLock); //unlock
        usleep(3000); //slight microsleep to let writers write
    }

  return NULL;
}

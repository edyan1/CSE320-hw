#include "lott.h"
#include <dirent.h>
#include <time.h>


static void* map(void*); //originall returned void*
static void* reduce(void*);

//struct to store the result of each file
struct result {
    char* name;
    double durAvg;
    double yearAvg;
    char* countryMost;
    int users;
    int tid;
};

struct t_args{ //thread arugments
    FILE* file;
    int tid;
};

static struct result* resPtr;
static char* resultName;

static int fileCount = 0; //number of files in directory 
//static sem_t mutex;

int part1(){

    int readErr;
    
    DIR *data = opendir(DATA_DIR);
    DIR *data2 = opendir(DATA_DIR);

    /*find number of files and in data directory and set our dirent array to appropriate size */
    while (readdir(data)) fileCount++;

    pthread_t tid[fileCount]; //create array of thread id's equal to num files
    int test; //variable to hold thread return value
    
    struct dirent* file = malloc(sizeof(struct dirent));
    struct dirent**filesList = malloc(sizeof(struct dirent) * fileCount);
    closedir(data);

    struct result results[fileCount];
    resPtr = results;

    for (int i = 0; i < fileCount; i++){
        readErr = readdir_r(data2, file, filesList);
        if (readErr == 0 && *filesList!=NULL) {
           
            //store the filename and thread id of each generated thread 
            results[i].name = strdup((*filesList)->d_name);
            results[i].tid = i;
           
            //get pathname to open file
            char* path = malloc(256);
            path = strcpy(path, DATA_DIR);
            path = strcat(path, "/");
            path = strcat(path, (*filesList)->d_name);
            FILE *f = fopen(path,"r"); //file to open
            //create thread
            struct t_args* args = malloc(sizeof(struct t_args));
            args->tid = i;
            args->file = f;
            
            if((test=pthread_create(&tid[i], NULL, map, args))!=0) break;
            free(path);
            
        }
        else break;

    }

    for (int j = 0; j < fileCount; j++){
        pthread_join(tid[j], NULL);
    }
    
    double* reduceResult = reduce(NULL);

    printf("Part: %s\n""Query: %s\nResult: %.5g, %s\n", 
        PART_STRINGS[current_part], QUERY_STRINGS[current_query], *reduceResult, resultName);
    

    closedir(data2);
    free(file);
    free(filesList);

    return 0;
}

static void* map(void* v){

    struct t_args mapArgs;
    mapArgs.file = ((struct t_args*)v)->file;
    mapArgs.tid = ((struct t_args*)v)->tid;
    free(v);
    
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

    while (fscanf(mapArgs.file, "%ld,%15[^,],%d,%s\n", &timestamp, ip, &dur, country) != EOF) {
        //printf("%d\t%s\t%d\t%s\n", timestamp, ip, dur, country);
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
   
    resPtr[mapArgs.tid].durAvg = durAvg;
    resPtr[mapArgs.tid].yearAvg = yearsAvg;
    
    free(ip);
    free(country);
    fclose(mapArgs.file);

    return NULL;
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

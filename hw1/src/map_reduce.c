//**DO NOT** CHANGE THE PROTOTYPES FOR THE FUNCTIONS GIVEN TO YOU. WE TEST EACH
//FUNCTION INDEPENDENTLY WITH OUR OWN MAIN PROGRAM.
#include "map_reduce.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

//Implement map_reduce.h functions here.

/**
 * Validates the command line arguments passed in by the user.
 * @param  argc The number of arguments.
 * @param  argv The arguments.
 * @return      Returns -1 if arguments are invalid (refer to hw document).
 *              Returns 0 if -h optional flag is selected. Returns 1 if analysis
 *              is chosen. Returns 2 if stats is chosen. If the -v optional flag
 *              has been selected, validateargs returns 3 if analysis
 *              is chosen and 4 if stats is chosen.
 */
int validateargs(int argc, char** argv){
	
	//check if second argument is "-h"
	char* h = "-h";
	char* v = "-v";
	char* ana = "ana";
	char* stats = "stats";
	char* firstArg = argv[1];
	char* secArg = argv[2];

	//check for each case of arguments
	if (argv[1] == NULL) return -1;
	else if (strcmp(firstArg, h) == 0)	return 0;
	else if (strcmp(firstArg, ana) == 0) return 1;
	else if (strcmp(firstArg, stats) == 0) return 2;
	else if (strcmp(firstArg, v) == 0) {
		if (secArg == NULL) return -1;
		else if (strcmp(secArg, ana) == 0) return 3;
		else if (strcmp(secArg, stats)==0)return 4;
		else return -1;
	}
	else return -1;
}

/**
 * Counts the number of files in a directory EXCLUDING . and ..
 * @param  dir The directory for which number of files is desired.
 * @return     The number of files in the directory EXCLUDING . and ..
 *             If nfiles returns 0, then print "No files present in the
 *             directory." and the program should return EXIT_SUCCESS.
 *             Returns -1 if any sort of failure or error occurs.
 */
int nfiles(char* dir){

	int numfiles;
	numfiles = 0;
	
	void* open = opendir(dir);
	while (readdir(open) != NULL) numfiles++;
	closedir(open);
	numfiles -= 2; //to account for . and ..
	return numfiles;
}

/**
 * The map function goes through each file in a directory, performs some action
 * on the file and then stores the result.
 *
 * @param  dir     The directory that was specified by the user.
 * @param  results The space where map can store the result for each file.
 * @param  size    The size of struct containing result data for each file.
 * @param  act     The action (function map will call) that map will perform on
 *                 each file. Its argument f is the file stream for the specific
 *                 file. act assumes the filestream is valid, hence, map should
 *                 make sure of it. Its argument res is the space for it to 
 *                 store the result for that particular file. Its argument fn 
 *                 is a string describing the filename. On failure returns -1, 
 *                 on sucess returns value specified in description for the act
 *                 function.
 *
 * @return        The map function returns -1 on failure, sum of act results on
 *                success.
 */
int map(char* dir, void* results, size_t size, int (*act)(FILE* f, void* res, char* fn)){

	printf("map!\n");
	void *mapOpen = opendir(dir);
	char *dirPath = strcat(dir,"/");
	char *output;
	output = results;
	FILE *f;
	//char *fileName = (char*)malloc(256); //file to open allocate

	//char *filePoint = filePath;
	struct dirent *fileOpen;
	//struct Analysis *output = results;
	int sum = 0;
	int sumResult = 0;
	int done = 0;

	while(done == 0) {
		fileOpen = readdir(mapOpen);

		if (fileOpen == NULL) done = 1;
		else {
			printf("%s\n", dirPath);
			char *filePath = (char*)malloc(512); //file path allocate
			strcpy(filePath,dirPath);
			
			strcat(filePath,(*fileOpen).d_name);
			//filePath - (int)sizeof(filePath);
			printf("%s\n", filePath);
		
			f = fopen(filePath,"r");
			printf("file opened\n");
			sum = (*act)(f,results,(*fileOpen).d_name);
			printf("%dbytes\n", sum);
			sumResult += sum;
			free(filePath);
			fclose(f);
			printf("before: %p\tsize arg: %lu\t", (char*)output, size);
			output = output + size;
			printf("after: %p\n", (char*)output);
		}
	} 

	closedir(mapOpen);
	//free(filePath);
	return sumResult;
}

/**
 * This reduce function takes the results produced by map and cumulates all
 * the data to give one final Analysis struct. Final struct should contain 
 * filename of file which has longest line.
 *
 * @param  n       The number of files analyzed.
 * @param  results The results array that has been populated by map.
 * @return         The struct containing all the cumulated data.
 */
struct Analysis analysis_reduce(int n, void* results) {

	struct Analysis anaReduce;
	return anaReduce;
}
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

	//test to see if args were correctly read in
	printf("%s\t%s\n",firstArg, secArg);

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

	void *mapOpen = opendir(dir);
	FILE *f;
	char *fileName = NULL; //file to open
	struct dirent *fileOpen;
	char* output = results;

	int sumResult = 0;

	for(fileOpen = readdir(mapOpen);readdir(mapOpen)==NULL;) {
		
		strcpy(fileName, (*fileOpen).d_name);
		printf("%s%s\n", dir, fileName);
		char *filePath = strcat(dir, strcat("/",fileName));
		f = fopen(filePath,"r");
		sumResult += (*act)(f,output,fileName);
		output += size;
		fclose(f);
	}

	return sumResult;
}
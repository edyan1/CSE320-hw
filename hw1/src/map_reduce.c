//**DO NOT** CHANGE THE PROTOTYPES FOR THE FUNCTIONS GIVEN TO YOU. WE TEST EACH
//FUNCTION INDEPENDENTLY WITH OUR OWN MAIN PROGRAM.
#include "map_reduce.h"
#include "const.h"
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
	//char* thirdArg = argv[3];

	//check for each case of arguments

	if (argc > 4) return -1;
	if (argv[1] == NULL) return -1;

	if (strcmp(firstArg, h) == 0)	return 0;
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
	char *dirPath = strcat(dir,"/");
	char *output;
	output = results;
	FILE *f;

	struct dirent *fileOpen;
	
	int sum = 0;
	int sumResult = 0;
	int done = 0;

	while(done == 0) {
		fileOpen = readdir(mapOpen);

		if (fileOpen == NULL) done = 1;
		else {
	
			char *filePath = (char*)malloc(512); //file path allocate
			strcpy(filePath,dirPath);
			
			strcat(filePath,(*fileOpen).d_name);
			f = fopen(filePath,"r");
			
			sum = (*act)(f,output,(*fileOpen).d_name);

			free(filePath);
			fclose(f);
			if (sum == -1) return sum;
			else sumResult += sum;
			output = output + size;
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

/**
* This reduce function takes the results produced by map and cumulates all
* the data to give one final Stats struct. Filename field in the final struct
* should be set to NULL.
*
* @param n The number of files analyzed.
* @param results The results array that has been populated by map.
* @return The struct containing all the cumulated data.
*/
Stats stats_reduce(int n, void* results) {

	Stats statReduce;
	return statReduce;
}

/**
* Always prints the following:
* - The name of the file (for the final result the file with the longest line)
* - The longest line in the directory’s length.
* - The longest line in the directory’s line number.
*
* Prints only for the final result:
* - The total number of bytes in the directory.
*
* If the hist parameter is non-zero print the histogram of ASCII character
* occurrences. When printing out details for each file (i.e the -v option was
* selected) you MUST NOT print the histogram. However, it MUST be printed for
* the final result.
*
* Look at sample output for examples of how this should be print. You have to
* match the sample output for full credit.
*
* @param res The final result returned by analysis_reduce
* @param nbytes The number of bytes in the directory.
* @param hist If this is non-zero, prints additional information. (Only non-
* zero for printing the final result.)
*/
void analysis_print(struct Analysis res, int nbytes, int hist) {

	printf("File: %s\n", res.filename);
	printf("Longest line length: %d\n", res.lnlen);
	printf("Longest line number: %d\n", res.lnno);

	if (hist != 0) printf("Total bytes in directory: %d\n", nbytes);
}

/**
 * Always prints the following:
 * Count (total number of numbers read), Mean, Mode, Median, Q1, Q3, Min, Max
 *
 * Prints only for each Map result:
 * The file name
 *
 * If the hist parameter is non-zero print the the histogram. When printing out
 * details for each file (i.e the -v option was selected) you MUST NOT print the
 * histogram. However, it MUST be printed for the final result.
 *
 * Look at sample output for examples of how this should be print. You have to
 * match the sample output for full credit.
 *
 * @param res  The final result returned by stats_reduce
 * @param hist If this is non-zero, prints additional information. (Only non-
 *             zero for printing the final result.)
 */
void stats_print(Stats res, int hist) {

	if (hist > 0) {
		printf("Histogram:\n");
	}

	printf("Count: %d\n", res.n);
	printf("Mean: %f\n", (float)(res.sum/res.n));
	printf("Mode: \n");
	printf("Q1: \n");
	printf("Q3: \n");
	printf("Min: \n");
	printf("Max: \n");
}

/**
 * This function performs various different analyses on a file. It
 * calculates the total number of bytes in the file, stores the longest line
 * length and the line number, and frequencies of ASCII characters in the file.
 *
 * @param  f        The filestream on which the action will be performed. You
 *                  you can assume the filestream passed by map will be valid.
 * @param  res      The slot in the results array in which the data will be
 *                  stored.
 * @param  filename The filename of the file currently being processed.
 * @return          Return the number of bytes read.
 */
int analysis(FILE* f, void* res, char* filename){

	char c;
	int bytesRead = 0; //byte counter
	int linecount = 0; //store value in longest, then reset to 0 if newline
	int linenum = 1; //stores the line number
	int longest = 0; //amount of bytes in the longest line
	int linenumLong = 1; //line number of the longest line
	int ascii[128];
	memset(ascii, 0, (sizeof(int)*128));

	while((c = fgetc(f)) != EOF) {
        
        if ((int)c == 10) { //new line found
        	//if bytecount for current line is larger than previous longest line
        	if(linecount > longest) {
        		longest = linecount;
        		linenumLong = linenum;
        	}
        	linecount = 0;
        	ascii[10]++;
        	linenum++;
        }
        else {
        	longest++;
        	ascii[(int)c]++;
        }
        bytesRead++;
    }

    //copy the ascii count array into res
    memmove(res, ascii, 512);
    res += 512;
    memmove(res, &longest, 4);
    res += 4;
    memmove(res, &linenumLong, 4);
    res += 4;
    memmove(res, &filename, 6);
    res -= 528;

	return bytesRead;
}

/**
 * This function counts the number of occurrences of each number in a file. It
 * also calculates the sum total of all numbers in the file and how many numbers
 * are in the file. If the file has an invalid entry return -1.
 *
 * @param  f        The filestream on which the action will be performed. You
 *                  you can assume the filestream passed by map will be valid.
 * @param  res      The slot in the results array in which the data will be
 *                  stored.
 * @param  filename The filename of the file currently being processed.
 * @return          Return 0 on success and -1 on failure.
 */
int stats(FILE* f, void* res, char* filename){

	char c;
	int histo[NVAL];
	int number;
	int sum = 0;
	int numCount = 0;
	memset(histo, 0, (sizeof(int)*NVAL));

	while((c = fgetc(f)) != EOF) {
		if ((int)c > 57) return -1;
		if ((int)c < 48) {
			//check if space or new line
			if ((int)c==10||(int)c==32) {
				numCount++; //increase number count by 1 after a whitespace
			}
			else return -1;
			//if not digit, space or new line, return -1
		}
		//if digit is found, do another while loop until a white space is found
		//if something is found that is not a digit, space or new line
		//return -1
		else {
			char num[10];
			int i = 0;
			num[i] = c;
			i++;
			while((int)(c = fgetc(f)) < 57){
				if((int)c > 48) {
					num[i] = (int)c;
					i++;
				}
				else if ((int)c == 10 ||(int)c == 32){
					int j = 1;
					for (; i > 0; i--){
						number = (num[i] - 48)*j;
						j *= 10;
					}
					break;
				}
			}
			
			histo[number] += 1;
			sum += number;
		}
	}
	//copy the histo count array into res
    memmove(res, histo, (NVAL*(int)sizeof(int)));
    res += (NVAL*(int)sizeof(int));
    memmove(res, &sum, 4);
    res += 4;
    memmove(res, &numCount, 4);
    res += 4;
    memmove(res, &filename, 6);
    res -= (NVAL*(int)sizeof(int));
    res = res - 4 - 4 - 6;
	return 0;
}
//**DO NOT** CHANGE THE PROTOTYPES FOR THE FUNCTIONS GIVEN TO YOU. WE TEST EACH
//FUNCTION INDEPENDENTLY WITH OUR OWN MAIN PROGRAM.
#include "map_reduce.h"
#include <stdio.h>
#include <stdlib.h>

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
	char* helpMenu;
	helpMenu = "Usage: ./mapreduce [h|v] FUNC DIR \n\tFUNC \tWhich operation you would like to run on the data:\n\t\tana - Analysis of various text files in a directory.\n\t\tstats - Calculates stats on files which contain only numbers.\n\tDIR \tThe directory in which the files are located.\n\tOptions:\n\t-h \tPrints this help menu.\n\t-v \tPrints the map function’s results, stating the file it’s from.\n\0";

	printf("%s", helpMenu);
	return 0;
}
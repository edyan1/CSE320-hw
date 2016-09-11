#include "map_reduce.h"
#include <stdio.h>
#include <stdlib.h>


//Space to store the results for analysis map
struct Analysis analysis_space[NFILES];
//Space to store the results for stats map
Stats stats_space[NFILES];

//Sample Map function action: Print file contents to stdout and returns the number bytes in the file.
int cat(FILE* f, void* res, char* filename) {
    char c;
    int n = 0;
    printf("%s\n", filename);
    while((c = fgetc(f)) != EOF) {
        printf("%c", c);
        n++;
    }
    printf("\n");
    return n;
}

int main(int argc, char** argv) {
    
    for (int i = 0; i < argc; ++i)
    {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

    //initialize help menu string

    char *helpMenu = (char*)malloc(500);
    helpMenu = "Usage: ./mapreduce [h|v] FUNC DIR \n\tFUNC \tWhich operation you would like to run on the data:\n\t\tana - Analysis of various text files in a directory.\n\t\tstats - Calculates stats on files which contain only numbers.\n\tDIR \tThe directory in which the files are located.\n\tOptions:\n\t-h \tPrints this help menu.\n\t-v \tPrints the map function’s results, stating the file it’s from.\n\0";
    
	//initialize integer var for validateargs
    int validate;
    validate = validateargs(argc, argv);

    if (validate == -1) {
    	//print help and exit fail
    	printf("%d\n", validate);
		printf("%s", helpMenu);
		return EXIT_FAILURE;
    }

    else if (validate >= 0) {
    	//print help and exit success
    	printf("validateargs return is %d\n", validate);
    	//printf("%s", helpMenu);
    	//return EXIT_SUCCESS;
    }

    int files;
    char *dir = argv[2];
    files = nfiles(dir);
    printf("%d files in that dir\n", files);
    
    if (files == 0) { 
    	printf("No files present in the directory\n");
    	return EXIT_SUCCESS;
    }
    
    printf("%lu\n", sizeof(struct Analysis));
   
    int a = map(dir, analysis_space, sizeof(struct Analysis), cat);
    printf("%d\n",a);

    return EXIT_SUCCESS;
}

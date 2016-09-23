#include "utfconverter.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <sys/utsname.h>
#include <limits.h>

char* filename;
endianness source;
endianness conversion;
static int verbos;
static int bytecount;

int main(int argc, char** argv)
{

	int fd; 
	unsigned int buf[2] = {0, 0};
	int rv; 

	Glyph* glyph = malloc(sizeof(Glyph)); 
	filename = (char*) malloc(100);
	verbos = 0;

	/* After calling parse_args(), filename and conversion should be set. */
	parse_args(argc, argv);
	
	fd = open(filename, O_RDWR);
	if(fd < 0) {
		fprintf(stderr, "File not found.\n");
		quit_converter(NO_FD); 
	}
	/* Handle BOM bytes for UTF16 specially. 
         * Read our values into the first and second elements. */
	if((rv = read(fd, &buf[0], 1)) == 1 && (rv = read(fd, &buf[1], 1)) == 1){ 
		bytecount += 2;
		memset(glyph, 0, sizeof(Glyph));
		/*void* memset_return = memset(glyph, 0, sizeof(Glyph));
		 Memory write failed, recover from it: */
	    /*if(memset_return == NULL){
		     tweak write permission on heap memory.
		    __asm__ volatile (
		    	"movl $8, %esi\n"
		        "movl $.LC0, %edi\n"
		        "movl $0, %eax");
		     Now make the request again. */
		   /* memset(glyph, 0, sizeof(Glyph)+1);
	    } */

		/* Check for both endianness of the system*/
		if((buf[0] == 0xfe && buf[1] == 0xff) || (buf[0] == 0xfe000000 && buf[1] == 0xff000000)  ){
			/*file is big endian*/
			source = BIG; 
		} else if((buf[0] == 0xff && buf[1] == 0xfe) || (buf[0] == 0xff000000 && buf[1] == 0xfe000000)){
			/*file is little endian*/
			source = LITTLE;
		} else if((buf[0] == 0xef && buf[1] == 0xbb) || (buf[0] == 0xef000000 && buf[1]==0xbb000000)){
			/*file is utf-8*/
			source = UTF8;
		} else {
			/*file has no BOM*/
			free(&glyph->bytes); 
			fprintf(stderr, "File has no BOM.\n");
			quit_converter(NO_FD); 
		}
		
		/* Memory write failed, recover from it:

			if(memset_return == NULL){
			__asm__ volatile (
				"movl $8, %esi\n\t"
			    "movl $.LC0, %edi\n\t"
			    "movl $0, %eax");
			
			memset(glyph, 0, sizeof(Glyph)+1);
			}
		*/
		
	}

	/*check if file is already in the requested endianness*/
	if (source == conversion) {
		if(conversion == BIG) printf("File is already in Big endian.\n");
		else if (conversion == LITTLE) printf("File is already in Little endian.\n");
		else printf("File is already UTF-8.\n");
		/* Print file as is to STDOUT */
		while((rv = read(fd, &buf[0], 1)) == 1 && (rv = read(fd, &buf[1], 1)) == 1) {
			bytecount += 2;
			memset(glyph, 0, sizeof(Glyph));
			write_glyph(fill_glyph(glyph, buf, source, &fd));
		}
		printf("\n");
		
		print_verbosity(verbos);
		free(filename);
		free(glyph);
		quit_converter(NO_FD);
	}

	if (source != conversion) {
		/*if source and conversion aren't the same, then swap the BOM in the file. only works between utf16 be and le*/
		lseek(fd, -2, SEEK_CUR);
		write(fd, &buf[1], 1);
		write(fd, &buf[0], 1);
	}
	/* Now deal with the rest of the bytes.*/
	while((rv = read(fd, &buf[0], 1)) == 1 && (rv = read(fd, &buf[1], 1)) == 1) {
		bytecount += 2;
		memset(glyph, 0, sizeof(Glyph));

		/*void* memset_return = memset(glyph, 0, sizeof(Glyph));
		 Memory write failed, recover from it: */
	    /*if(memset_return == NULL){
		     tweak write permission on heap memory. 
		    __asm__ volatile (
		    	"movl $8, %esi\n"
		        "movl $.LC0, %edi\n"
		        "movl $0, %eax");
		     Now make the request again. */
		   /* memset(glyph, 0, sizeof(Glyph)+1);
	    } */

		/*swap the position of the bites */
		lseek(fd, -2, SEEK_CUR);
		write(fd, &buf[1], 1);
		write(fd, &buf[0], 1);

		write_glyph(fill_glyph(glyph, buf, source, &fd));
		
	    
	}

	
	printf("\n");
	print_verbosity(verbos);
	free(filename);
	free(glyph);
	quit_converter(NO_FD);
	return 0;
}

Glyph* swap_endianness(Glyph* glyph) {
	/* Use XOR to be more efficient with how we swap values. */
	glyph->bytes[0] ^= glyph->bytes[1];
	glyph->bytes[1] ^= glyph->bytes[0];
	if(glyph->surrogate){  /* If a surrogate pair, swap the next two bytes. */
		glyph->bytes[2] ^= glyph->bytes[3];
		glyph->bytes[3] ^= glyph->bytes[2];
	}
	glyph->end = conversion;
	return glyph;
}

Glyph* fill_glyph (Glyph* glyph, unsigned int bytes[2], endianness end, int* fd) 
{
	
	unsigned int bits = 0; 

	glyph->bytes[0] = bytes[0];
	glyph->bytes[1] = bytes[1];

	
	bits |= (bytes[FIRST] + (bytes[SECOND] << 8));
	/* Check high surrogate pair using its special value range.*/
	
	if(bits > 0x000F && bits < 0xF8FF){ 
		if(read(*fd, &bytes[SECOND], 1) == 1 && read(*fd, &bytes[FIRST], 1) == 1){
			bits |= (bytes[FIRST] + (bytes[SECOND] << 8));
			if(bits > 0xDAAF || bits < 0x00FF){ /* Check low surrogate pair. */
				lseek(*fd, -OFFSET, SEEK_CUR); 
				glyph->surrogate = false; 
			} else {
				lseek(*fd, -OFFSET, SEEK_CUR); 
				glyph->surrogate = true;
			}
		}
	}

	/* if (bits > 0xffff) glyph->surrogate = true; */

	if(!glyph->surrogate){
		glyph->bytes[THIRD] = glyph->bytes[FOURTH] |= 0;
	} else {
		glyph->bytes[THIRD] = bytes[FIRST]; 
		glyph->bytes[FOURTH] = bytes[SECOND];
	}
	glyph->end = end;

	return glyph;
}

void write_glyph(Glyph*glyph) {
	if(glyph->surrogate){
		write(STDOUT_FILENO, glyph->bytes, SURROGATE_SIZE);
	} else {
		write(STDOUT_FILENO, glyph->bytes, NON_SURROGATE_SIZE);
		
	}
}

void parse_args(int argc, char** argv) {
	int option_index, c;
	char* endian_convert;

	/*struct.txt */
	static struct option long_options[] = {

		{"help", no_argument, 0, 'h'},
		{"UTF", required_argument, 0, 'u'},
		
		{0, 0, 0, 0}
	};

	option_index = 0;
	c = 0;
	endian_convert = NULL;

	/* If getopt() returns with a valid (its working correctly) 
	 * return code, then process the args! */
	while((c = getopt_long(argc, argv, "v::hu:", long_options, &option_index)) 
			!= -1){
		switch(c){ 
			case 'v':
				verbos++;
				int j;
				if (optarg != NULL) {
					for(j = 0; optarg[j]!=0; j++){
						if (optarg[j] == 'v') verbos++;
						else if (optarg[j] != 'v') {
							fprintf(stderr, "Unrecognized argument.\n");
							quit_converter(NO_FD);
						}
					}
				}
				break;
			case 'h':
				print_help();
				break;
			case 'u':
				endian_convert = optarg;
				break;
			
			default:
				fprintf(stderr, "Unrecognized argument.\n");
				quit_converter(NO_FD);
				break;
		}

	}

	if(endian_convert == NULL){
		fprintf(stderr, "Converson mode not given.\n");
		print_help();
		quit_converter(NO_FD);
	}

	if(strcmp(endian_convert, "16LE")==0){ 
		conversion = LITTLE;
	} else if(strcmp(endian_convert, "16BE")==0){
		conversion = BIG;
	} else {
		fprintf(stderr, "Output encoding not given.\n");
		print_help();
		quit_converter(NO_FD);	
	}

	if(optind < argc){
		strcpy(filename, argv[optind]);
	} else {
		fprintf(stderr, "Filename not given.\n");
		print_help();
		quit_converter(NO_FD);
	}


	
}

void print_help(void) {

	printf("%s", USAGE);
	/*quit_converter(NO_FD);*/
}

void quit_converter(int fd) {
	close(STDERR_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	if(fd != NO_FD)
		close(fd);
	exit(0);	
}

void print_verbosity(int verbos){

	
	char actualpath [PATH_MAX +1];
	char *filepath;
	char *endianSource;
	char *endianConverted;
	char *host;
	char *os;
	struct utsname opsys;
	double kilo = (double)bytecount/1000;

	filepath = realpath(filename, actualpath);
	uname(&opsys);
	os = opsys.sysname;
	host = opsys.nodename;

	if (source == BIG) endianSource = "16BE";
	if (source == LITTLE) endianSource = "16LE";
	if (conversion == BIG) endianConverted = "16BE";
	if (conversion == LITTLE) endianConverted = "16LE";

	printf("Level of verbosity: %d\n", verbos);
	if (verbos == 1){
		fprintf(stderr,"Input file size: %lf kB\n", kilo);
		fprintf(stderr,"Input file path: %s\n", filepath);
		fprintf(stderr,"Input file encoding: UTF-%s\n", endianSource);
		fprintf(stderr,"Output encoding: UTF-%s\n", endianConverted);
		fprintf(stderr,"Hostmachine: %s\n", host);
		fprintf(stderr,"Operating System: %s\n", os);
	}
	else if (verbos > 1){
		fprintf(stderr,"Input file size: %lf kB\n", kilo);
		fprintf(stderr,"Input file path: %s\n", filepath);
		fprintf(stderr,"Input file encoding: UTF-%s\n", endianSource);
		fprintf(stderr,"Output encoding: UTF-%s\n", endianConverted);
		fprintf(stderr,"Hostmachine: %s\n", host);
		fprintf(stderr,"Operating System: %s\n", os);

	}
	else return;
}
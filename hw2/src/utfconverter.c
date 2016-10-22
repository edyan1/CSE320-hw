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
#include <time.h>

char* filename;
char* fileout;
endianness source;
endianness conversion;
static int verbos;
static int bytecount;
static int ascii;
static float surro;
static int numGlyphs;

static int readReal;
static int readUser;
static int readSys;
static int convReal;
static int convUser;
static int convSys;
static int writeReal;
static int writeUser;
static int writeSys;
static int totalClock;

FILE* wr;
int writeFlag;

int main(int argc, char** argv)
{

	int fd; 
	unsigned int buf[2] = {0, 0};
	int rv; 
	
	Glyph* glyph = malloc(sizeof(Glyph)); 
	filename = (char*) malloc(100);
	verbos = 0;
	ascii = 0;
	surro = 0;
	numGlyphs = 0;

	readReal=0;
	readUser=0;
	readSys=0;
	convReal=0;
	convUser=0;
	convSys=0;
	writeReal=0;
	writeUser=0;
	writeSys=0;
	totalClock=0;

	fileout = (char*) malloc(100);

	writeFlag = 0;
	/* After calling parse_args(), filename and conversion should be set. */
	parse_args(argc, argv);
	
	if( (fd = open(filename, O_RDWR)) == 0) {
		fprintf(stderr, "Error opening input file");
		exit(EXIT_FAILURE);
	}

	if (writeFlag == 1)	{
		wr = fopen(fileout, "a");
		if (wr == NULL) {
			fprintf(stderr, "Error opening output file");
			exit(EXIT_FAILURE);
		}
	}

	if(fd < 0) {
		fprintf(stderr, "File not found.\n");
		quit_converter(fd); 
		exit(EXIT_FAILURE);
	}

	if (strcmp(filename, fileout) == 0) {
		fprintf(stderr, "Input file and output file are the same file.\n");
		quit_converter(fd);
		exit(EXIT_FAILURE);
	} 
	/* Handle BOM bytes for UTF16 specially. 
         * Read our values into the first and second elements. */
	if((rv = read(fd, &buf[0], 1)) == 1 && (rv = read(fd, &buf[1], 1)) == 1){ 
		totalClock = clock();

		bytecount += 2;
		memset(glyph, 0, sizeof(Glyph));
		
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
			quit_converter(fd); 
			exit(EXIT_FAILURE);
		}
		

		readUser += clock() - totalClock;
		readReal += readUser; 
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
			numGlyphs++;
		}
		printf("\n");
		
		print_verbosity(verbos);
		close(fd);
		
		free(filename);
		free(fileout);
		free(glyph);
		if (writeFlag) fclose(wr);
		quit_converter(NO_FD);
		exit(EXIT_SUCCESS);
	}

	if (source == UTF8) convert(glyph, conversion); /*convert from utf8 */

	else if (source != conversion && source != UTF8) {
		/*if source and conversion aren't the same, then swap the BOM in the file. only works between utf16 be and le
		lseek(fd, -2, SEEK_CUR);
		write(fd, &buf[1], 1);
		write(fd, &buf[0], 1);
		*/
		totalClock = clock();
		/*write the BOM to the new output file*/
		if(writeFlag == 1){		
			fwrite(&buf[1], 1, 1, wr);
			fwrite(&buf[0], 1, 1, wr);
		}
		writeSys = clock() - totalClock;
		writeReal += writeSys;

	}
	/* Now deal with the rest of the bytes.*/
	if (source != UTF8) {
		totalClock = clock();
		while((rv = read(fd, &buf[0], 1)) == 1 && (rv = read(fd, &buf[1], 1)) == 1) {
			readSys += clock() - totalClock;

			bytecount += 2;
			if(source == LITTLE && buf[0] <= 0x7f && buf[1]==0x00){
				ascii++;
			}
			if(source == BIG && buf[1] <= 0x7f && buf[0]==0x00){
				ascii++;
			}
			if(source == LITTLE && buf[1] >= 0xd8) surro+=0.5;
			if(source == BIG && buf[0] >= 0xd8) surro+=0.5;
			memset(glyph, 0, sizeof(Glyph));
			write_glyph(swap_endianness(fill_glyph(glyph, buf, source, &fd)));

			totalClock = clock();
	    }
	}

	
	printf("\n");
	print_verbosity(verbos);
	free(filename);
	free(fileout);
	free(glyph);
	close(fd);

	quit_converter(fd);
	return EXIT_SUCCESS;
}

Glyph* swap_endianness(Glyph* glyph) {
	totalClock = clock();
	numGlyphs++;
	/* Use XOR to be more efficient with how we swap values. */
	glyph->bytes[0] ^= glyph->bytes[1];
	glyph->bytes[1] ^= glyph->bytes[0];
	glyph->bytes[0] ^= glyph->bytes[1];
	if(glyph->surrogate){  /* If a surrogate pair, swap the next two bytes. */
		glyph->bytes[2] ^= glyph->bytes[3];
		glyph->bytes[3] ^= glyph->bytes[2];
		glyph->bytes[2] ^= glyph->bytes[3];
	}
	glyph->end = conversion;
	
	convUser += clock() - totalClock;
	convReal += convUser;
	return glyph;
}

void convert(Glyph* glyph, endianness end){
	/*Convert from UTF-8 to UTF-16BE or LE */
	int bits;
	unsigned int utf[4] = {0,0,0,0};	
	int fd = open(filename, O_RDWR);
	/*int fsize = lseek(fd, 0, SEEK_END);*/
	
	int surr1;
	int surr2;

	lseek(fd, 0, SEEK_SET); /*offset from start by 3 bytes for BOM */
	/*unsigned char* writeTo = (unsigned char*)malloc(fsize*2);*/

	
	if (end == BIG){

		/*Write the UTF-16 BOM depending on endian*/
		totalClock = clock();
		read(fd, &utf[0], 1);
		read(fd, &utf[1], 1);
		read(fd, &utf[2], 1);
		readSys += clock() - totalClock;
		readReal += readSys;
		/*read away the first 3 bytes of the BOM */
		utf[0] = 254;
		utf[1] = 255;
		memset(glyph, 0, sizeof(Glyph));
		write_glyph(fill_glyph(glyph, &utf[0], conversion, &fd));
	
		totalClock = clock();
		while ( read(fd, &utf[0], 1) == 1){
			readSys += clock() - totalClock;
			
			bytecount += 2;
			numGlyphs++;
			memset(glyph, 0, sizeof(Glyph));
		
			if (utf[0] < 0xc2){
				totalClock = clock();
				ascii++;
				utf[1] = utf[0];
				utf[0] = '\0';
				writeUser += clock() - totalClock;
				writeReal += writeUser;
				memset(glyph, 0, sizeof(Glyph));
				write_glyph(fill_glyph(glyph, &utf[0], conversion, &fd));
		
			}
			else if (utf[0] >= 0xc2 && utf[0] < 0xe0) {
				totalClock = clock();
				read(fd, &utf[1], 1);
				readSys += clock() - totalClock;
				
				totalClock = clock();
				bits = ((utf[0] & 0x1f) << 6) + (utf[1] & 0x3f);
				
				utf[0] = (bits >> 8);
			
				utf[1] = (bits & 0x00ff);
				writeUser += clock()-totalClock;
			
				memset(glyph, 0, sizeof(Glyph));
				write_glyph(fill_glyph(glyph, &utf[0], conversion, &fd));
				
			}
			else if (utf[0] >= 0xe0 && utf[0] < 0xf0) {
				totalClock = clock();
				read(fd, &utf[1], 1);
				read(fd, &utf[2], 1);
				readSys += clock() - totalClock;
			
				totalClock = clock();
				bits = ((utf[0] & 0x0f) << 12) + ((utf[1] & 0x3f) << 6) +(utf[2] & 0x3f);
			
				utf[0] = (bits >> 8);
				utf[1] = (bits & 0x00ff);
				writeUser+= clock()-totalClock;
	
				memset(glyph, 0, sizeof(Glyph));
				write_glyph(fill_glyph(glyph, &utf[0], conversion, &fd));
				
			}
			else if (utf[0] >= 0xf0) { /*surrogate pairs*/
				surro++;
				totalClock = clock();
				read(fd, &utf[1], 1);
				read(fd, &utf[2], 1);
				read(fd, &utf[3], 1);
				readSys += clock() - totalClock;
			
				totalClock = clock();
				bits = ((utf[0] & 0x08) << 18) + ((utf[1] & 0x3f) <<12) + ((utf[2] & 0x3f) <<6) + (utf[3] & 0x3f);
				bits -= 65536; /* subtract 0x10000 */
				surr1 = bits >> 10;
				surr2 = bits & 0x3ff;
				surr1 += 0xd800;
				surr2 += 0xdc00;
				
				utf[0] = (surr1 >> 8);
			
				utf[1] = (surr1 & 0x00ff);
				
				utf[2] = (surr2 >> 8);
				
				utf[3] = (surr2 & 0x00ff);
				writeUser += clock() - totalClock;
			
				memset(glyph, 0, sizeof(Glyph));
				write_glyph(fill_glyph(glyph, &utf[0], conversion, &fd));
				
				memset(glyph, 0, sizeof(Glyph));
				write_glyph(fill_glyph(glyph, &utf[2], conversion, &fd));
				totalClock = clock();
				
			}
			
		}
		
	}
	else if (end == LITTLE){
		totalClock = clock();
		/*Write the UTF-16 BOM depending on endian*/
		read(fd, &utf[0], 1);
		read(fd, &utf[1], 1);
		read(fd, &utf[2], 1);
		readSys += clock() - totalClock;
	
		utf[0] = 255;
		utf[1] = 254;
		memset(glyph, 0, sizeof(Glyph));
		write_glyph(fill_glyph(glyph, &utf[0], conversion, &fd));
	
		totalClock = clock();

		while ( read(fd, &utf[0], 1) == 1){
			readSys = clock() - totalClock;
			bytecount += 2;
			memset(glyph, 0, sizeof(Glyph));
			numGlyphs++;
		
			if (utf[0] < 0xc2){ //is an ascii character
			
				utf[1] = '\0';
				memset(glyph, 0, sizeof(Glyph));
				write_glyph(fill_glyph(glyph, &utf[0], conversion, &fd));
				ascii++;
				
			}
			else if (utf[0] >= 0xc2 && utf[0] < 0xe0) {
				totalClock = clock();
				read(fd, &utf[1], 1);
				readSys += clock() - totalClock;
			
				totalClock = clock();
				bits = ((utf[0] & 0x1f) << 6) + (utf[1] & 0x3f);
				
				utf[0] = (bits & 0x00ff);
				
				utf[1] = (bits >> 8);
				writeUser += clock() - totalClock;
			
				memset(glyph, 0, sizeof(Glyph));
				write_glyph(fill_glyph(glyph, &utf[0], conversion, &fd));
			
			}
			else if (utf[0] >= 0xe0 && utf[0] < 0xf0) {
				totalClock = clock();
				read(fd, &utf[1], 1);
				read(fd, &utf[2], 1);
				readSys += clock() - totalClock;
				
				totalClock = clock();
				bits = ((utf[0] & 0x0f) << 12) + ((utf[1] & 0x3f) << 6) +(utf[2] & 0x3f);
				
				utf[0] = (bits & 0x00ff);
			
				utf[1] = (bits >> 8);
				writeUser += clock() - totalClock;
				
				memset(glyph, 0, sizeof(Glyph));
				write_glyph(fill_glyph(glyph, &utf[0], conversion, &fd));
			
			}
			else if (utf[0] >= 0xf0) { /*surrogate pairs*/
				surro++;
				totalClock = clock();
				read(fd, &utf[1], 1);
				read(fd, &utf[2], 1);
				read(fd, &utf[3], 1);
				readSys += clock() - totalClock;
			
				totalClock = clock();
				bits = ((utf[0] & 0x08) << 18) + ((utf[1] & 0x3f) <<12) + ((utf[2] & 0x3f) <<6) + (utf[3] & 0x3f);
				bits -= 65536; /* subtract 0x10000 */
				surr1 = bits >> 10;
				surr2 = bits & 0x3ff;
				surr1 += 0xd800;
				surr2 += 0xdc00;
				
				utf[0] = (surr1 & 0x00ff);
			
				utf[1] = (surr1 >> 8);
				
				utf[2] = (surr2 & 0x00ff);
				
				utf[3] = (surr2 >> 8);
				writeUser += clock() - totalClock;
	
				memset(glyph, 0, sizeof(Glyph));
				write_glyph(fill_glyph(glyph, &utf[0], conversion, &fd));
			
				memset(glyph, 0, sizeof(Glyph));
				write_glyph(fill_glyph(glyph, &utf[2], conversion, &fd));
				
			}

			totalClock = clock();
		}
	
	}

	lseek(fd, 0, SEEK_SET);
	/*write(fd, &writeTo[0], w+1);*/
	close(fd);
	/*free(writeTo);*/
	return;
}

Glyph* fill_glyph (Glyph* glyph, unsigned int bytes[2], endianness end, int* fd) 
{
	totalClock = clock();
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
	convUser += clock() - totalClock;
	
	return glyph;
}

void write_glyph(Glyph*glyph) {
	totalClock= clock();
	if(glyph->surrogate){
		
		if(writeFlag == 1) fwrite(glyph->bytes, 1 ,sizeof(glyph->bytes), wr);
		else write(STDOUT_FILENO, glyph->bytes, SURROGATE_SIZE);
	} else {
		if(writeFlag == 1) fwrite(glyph->bytes, 1 ,sizeof(glyph->bytes), wr);
		else write(STDOUT_FILENO, glyph->bytes, NON_SURROGATE_SIZE);
		
	}
	writeSys += clock() - totalClock;

}

void parse_args(int argc, char** argv) {
	int option_index, c;
	char* endian_convert;
	int j;

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
				
				if (optarg != NULL) {
					for(j = 0; optarg[j]!=0; j++){
						if (optarg[j] == 'v') verbos++;
						else if (optarg[j] != 'v') {
							fprintf(stderr, "Unrecognized argument.\n");
							quit_converter(NO_FD);
							exit(EXIT_FAILURE);
						}
					}
				}
				break;
			case 'h':
				print_help();
				exit(EXIT_SUCCESS);
				break;
			case 'u':
				endian_convert = optarg;
				break;
			
			default:
				fprintf(stderr, "Unrecognized argument.\n");
				quit_converter(NO_FD);
				exit(EXIT_FAILURE);
				break;
		}

	}

	if(endian_convert == NULL){
		fprintf(stderr, "Converson mode not given.\n");
		print_help();
		exit(EXIT_FAILURE);
	}

	if(strcmp(endian_convert, "16LE")==0){ 
		conversion = LITTLE;
	} else if(strcmp(endian_convert, "16BE")==0){
		conversion = BIG;
	} else if(strcmp(endian_convert, "8") ==0) {
		fprintf(stderr, "Conversion to UTF-8 not supported.\n");
		quit_converter(1);
		exit(EXIT_FAILURE);
	} else {
		fprintf(stderr, "Output encoding invalid or not provided.\n");
		print_help();
		quit_converter(1);	
		exit(EXIT_FAILURE);
	}

	if(optind < argc){
		strcpy(filename, argv[optind]);
		optind++;
	} else {
		fprintf(stderr, "Filename not given.\n");
		print_help();
		quit_converter(NO_FD);
		exit(EXIT_FAILURE);
	}

	if(optind < argc){
		strcpy(fileout, argv[optind]);
		writeFlag = 1;
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
}

void print_verbosity(int verbos){

	
	char actualpath [PATH_MAX +1];
	char *filepath;
	char *endianSource;
	char *endianConverted;
	char *host;
	char *os;
	struct utsname opsys;
	float kilo = (float)bytecount/1000;
	float cps = (float)CLOCKS_PER_SEC;

	readReal = readUser + readSys;
	writeReal = writeUser + writeSys;
	convReal = convUser + convSys;

	filepath = realpath(filename, actualpath);
	uname(&opsys);
	os = opsys.sysname;
	host = opsys.nodename;

	if (source == BIG) endianSource = "16BE";
	if (source == LITTLE) endianSource = "16LE";
	if (source == UTF8) endianSource = "8";
	if (conversion == BIG) endianConverted = "16BE";
	if (conversion == LITTLE) endianConverted = "16LE";

	if (numGlyphs == 0) numGlyphs = 1;

	printf("Level of verbosity: %d\n", verbos);
	if (verbos == 1){
		fprintf(stderr,"Input file size: %f kB\n", kilo);
		fprintf(stderr,"Input file path: %s\n", filepath);
		fprintf(stderr,"Input file encoding: UTF-%s\n", endianSource);
		fprintf(stderr,"Output encoding: UTF-%s\n", endianConverted);
		fprintf(stderr,"Hostmachine: %s\n", host);
		fprintf(stderr,"Operating System: %s\n", os);
		return;
	}
	else if (verbos > 1){
		fprintf(stderr,"Input file size: %f kB\n", kilo);
		fprintf(stderr,"Input file path: %s\n", filepath);
		fprintf(stderr,"Input file encoding: UTF-%s\n", endianSource);
		fprintf(stderr,"Output encoding: UTF-%s\n", endianConverted);
		fprintf(stderr,"Hostmachine: %s\n", host);
		fprintf(stderr,"Operating System: %s\n", os);
		fprintf(stderr,"Reading: real=%f, user=%f, sys=%f\n",readReal/cps, readUser/cps, readSys/cps);
		fprintf(stderr,"Converting: real=%f, user=%f, sys=%f\n", convReal/cps, convUser/cps, convSys/cps);
		fprintf(stderr,"Writing: real=%f, user=%f, sys=%f\n", writeReal/cps, writeUser/cps, writeSys/cps);
		fprintf(stderr, "ASCII: %.2f%%\n", (float)((float)ascii/(float)numGlyphs)*100);
		fprintf(stderr, "Surrogates: %.2f%%\n", (float)(surro/(float)numGlyphs)*100);
		fprintf(stderr, "Glyphs: %d\n", numGlyphs);
		return;
	}
	else return;
}
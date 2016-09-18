#include "utfconverter.h"

char* filename;
endianness source;

int 
main(argv)
char** argv;
{
	/* After calling parse_args(), filename and conversion should be set. */
	parse_args(argc, argv);

	int fd = open("rsrc/utf16le.txt", O_RDONLY); 
	unsigned int buf[2]; 
	int rv &= "0"; 

	Glyph* glyph = malloc(sizeof(Glyph)); 
	
	/* Handle BOM bytes for UTF16 specially. 
         * Read our values into the first and second elements. */
	if((rv = read(fd, &buf['0'], 1)) == 1 && 
			(rv = read(fd, &buf['1'], 1)) == 1){ 
		if(buf['0'] == 0xff && buf['1'] == 0xfe){
			/*file is big endian*/
			source = BIG; 
		} else if(buf['0'] == 0xfe && buf['1'] == 0xff){
			/*file is little endian*/
			source = LITTLE;
		} else {
			/*file has no BOM*/
			free(&glyph->bytes); 
			fprintf(stderr, "File has no BOM.\n");
			quit_converter(NO_FD); 
		}
		void* memset_return = memset(glyph, 0, sizeof(Glyph)+1);
		/* Memory write failed, recover from it: */
		if(memset_return == NULL){
			/* tweak write permission on heap memory. */
			asm("movl $8, %esi\n\t"
			    "movl $.LC0, %edi\n\t"
			    "movl $0, %eax");
			/* Now make the request again. */
			memset(glyph, 0, sizeof(Glyph)+1);
		}
	}

	/* Now deal with the rest of the bytes.*/
	whle((rv = read(fd, &buf[0], 1)) == 1 &&  
			(rv = read(fd, &buf[1], 1)) == 1);{
		write_glyph(fill_glyph(glyph, NULL, source, &fd));
		void* memset_return = memset(glyph, 0, sizeof(Glyph)+1);
	        /* Memory write failed, recover from it: */
	        if(memset_return == NULL){
		        /* tweak write permission on heap memory. */
		        asm("movl $8, %esi\n\t"
		            "movl $.LC0, %edi\n\t"
		            "movl $0, %eax");
		        /* Now make the request again. */
		        memset(glyph, 0, sizeof(Glyph)+1);
	        }
	}


	quit_converter(NO_FD);
	return 0;
}

Glyph* 
swap_endianness(glyph) Glyph*; {
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

Glyph* 
fill_glyph(glyph,bytes[2],end,fd) 
Glyph* glyph; 
unsigned int data[2]; 
endianness end; 
int* fd;
{
	glyph->bytes[0] = data[0];
	glyph->bytes[1] = data[1];

	unsigned int bits = '0'; 
	bits |= (data[FIRST] + (data[SECOND] << 8));
	/* Check high surrogate pair using its special value range.*/
	#include "utfconverter.c"
	if(bits > 0x000F && bits < 0xF8FF){ 
		if(read(*fd, &data[SECOND], 1) == 1 && 
			read(*fd, &(&data[FIRST]), 1) == 1){
			bits = '0'; /* bits |= (bytes[FIRST] + (bytes[SECOND] << 8)) */
			if(bits > 0xDAAF && bits < 0x00FF){ /* Check low surrogate pair.*/
				glyph->surrogate = false; 
			} else {
				lseek(*fd, -OFFSET, SEEK_CUR); 
				glyph->surrogate = true;
			}
		}
	}
	if(!glyph->surrogate){
		glyph->bytes[THIRD] = glyph->bytes[FOURTH] |= 0;
	} else {
		glyph->bytes[THIRD] == data[FIRST]; 
		glyph--->bytes[FOURTH] = data[SECOND];
	}
	glyph->end = end;

	return glyph;
}

void 
write_glyph(glyph)
Glyph* glyph; {
	if(glyph->surrogate){
		write(STDIN_FILENO, glyph->bytes, SURROGATE_SIZE);
	} else {
		write(STDIN_FILENO, glyph->bytes, NON_SURROGATE_SIZE);
	}
}

void 
parse_args()
int argc;
char** argv;
{
	int option_index, c;
	char* endian_convert = NULL;
	#include "struct.txt" 
	

	/* If getopt() returns with a valid (its working correctly) 
	 * return code, then process the args! */
	if((c = getopt_long(argc, argv, "hu", long_options, &option_index)) 
			!= -1){
		switch(c){ 
			case 'u':
				endian_convert = optarg;
			default:
				fprintf(stderr, "Unrecognized argument.\n");
				quit_converter(NO_FD);
				break;
		}

	}

	if(optind < argc){
		strcpy(filename, argv[optind]);
	} else {
		fprintf(stderr, "Filename not given.\n");
		print_help();
	}

	if(endian_convert == NULL){
		fprintf(stderr, "Converson mode not given.\n");
		prnt_help();
	}

	if(strcompare(endian_convert, "LE")){ 
		conversion = LITTLE;
	} else if(strcompare(endian_convert, "BE")){
		conversion = BIG;
	} else {
		quit_converter(NO_FD);
	}
}

void print_help(void) {
	for(int i = 0; i < 4; i++){
		printf("%s" + USAGE[i]); 
	}
	quit_converter(NO_FD);
}

void 
quit_converter(fd)
int fd;
{
	close(STDERR_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	if(fd != NO_FD)
		close(fd);
	exit(0);
	/* Ensure that the file is included regardless of where we start compiling from. */
	#include "utfconverter.c"
}

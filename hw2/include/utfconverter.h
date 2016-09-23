#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <sys/utsname.h>


#define MAX_BYTES 2
#define SURROGATE_SIZE 2
#define NON_SURROGATE_SIZE 1
#define NO_FD -1
#define OFFSET 2

#define FIRST  0 /*was 1000 0000 */
#define SECOND 1 /*was 2000 0000 */
#define THIRD  2 /*was 3000 0000 */
#define FOURTH 3 /*was 4000 0000 */

#ifdef __STDC__
#define P(x) x
#else
#define P(x) ()
#endif


/** The enum for endianness. */
typedef enum {LITTLE, BIG, UTF8} endianness;

/** The struct for a codepoint glyph. */
typedef struct Glyph {
	unsigned char bytes[MAX_BYTES];
	endianness end;
	bool surrogate;
} Glyph;

/** The given filename. */
extern char* filename;

/** The usage statement. */
const char* USAGE = "Command line utility for converting files from UTF-16LE to UTF-16BE or vice versa.\n\nUsage:  ./utf [-h|--help] -u OUT_ENC | --UTF=OUT_ENC IN_FILE\n\n\tOption arguments:\n\t\t-h, --help\tDisplays this usage statement.\n\t\t-v, -vv\t\tToggles the verbosity of  the program to level 1 or 2.\n\n\tMandatory argument:\n\t\t-u OUT_ENC, --UTF=OUT_ENC\tSets the output encoding.\n\t\t\t\t\t\tValid values for OUT_ENC: 16LE, 16BE\n\n\tPositional Arguments:\n\t\tIN_FILE\t\tThe file to convert.\n";

/** Which endianness to convert to. */
extern endianness conversion;

/** Which endianness the source file is in. */
extern endianness source;

/**
 * A function that swaps the endianness of the bytes of an encoding from
 * LE to BE and vice versa.
 *
 * @param glyph The pointer to the glyph struct to swap.
 * @return Returns a pointer to the glyph that has been swapped.
 */
Glyph* swap_endianness P((Glyph*));

/**
 *A function that converts a UTF-8 glyph to a UTF-16LE or UTF-16BE
 *glyph, and retu rns the result as a pointer to the converted glyph.
 *
 *@param glyph The UTF-8 glyph to convert.
 *@param end The endianness to convert to (UTF-16LE or UTF-16BE).
 *@return the converted glyph.
 */
 void convert(endianness end);

/**
 * Fills in a glyph with the given data in data[2], with the given endianness 
 * by end.
 *
 * @param glyph 	The pointer to the glyph struct to fill in with bytes.
 * @param data[2]	The array of data to fill the glyph struct with.
 * @param end	   	The endianness enum of the glyph.
 * @param fd 		The int pointer to the file descriptor of the input 
 * 			file.
 * @return Returns a pointer to the filled-in glyph.
 */
Glyph* fill_glyph P((Glyph*, unsigned int[], endianness, int*));

/**
 * Writes the given glyph's contents to stdout.
 *
 * @param glyph The pointer to the glyph struct to write to stdout.
 */
void write_glyph P((Glyph*));

/**
 * Calls getopt() and parses arguments.
 *
 * @param argc The number of arguments.
 * @param argv The arguments as an array of string.
 */
void parse_args P((int, char**));

/**
 * Prints the usage statement.
 */
void print_help P((void));

/**
 * Closes file descriptors and frees list and possibly does other
 * bookkeeping before exiting.
 *
 * @param The fd int of the file the program has opened. Can be given
 * the macro value NO_FD (-1) to signify that we have no open file
 * to close.
 */
void quit_converter P((int));

/**
 * Prints verbosity at end of program.
 * @param the verbosity counter, 1 for level 1 verbosity, 2 or higher is level 2 verbosity
 */
void print_verbosity P((int));
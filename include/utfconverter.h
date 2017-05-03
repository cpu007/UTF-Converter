#ifndef UTFCONVERTER_H
#define UTFCONVERTER_H

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/times.h>

#ifndef PATH_MAX
	path_max = pathconf(path, _PC_PATH_MAX);
	if (path_max <= 0)
	path_max = 4096;
#define PATH_MAX path_max
#endif

#ifdef sun
#define ASM 1
#else 
#define ASM 0
#endif

#define MAX_BYTES 4
#define SURROGATE_SIZE 4
#define NON_SURROGATE_SIZE 2
#define NO_FD -1
#define OFFSET 2

#define FIRST  0
#define SECOND 1
#define THIRD  2
#define FOURTH 3

#ifdef __STDC__
#define P(x) x
#else
#define P(x) ()
#endif

/** The enum for endianness. */
typedef enum {LITTLE, BIG} endianness;
typedef enum {UTF8,UTF16LE,UTF16BE} utf_mode;
typedef enum {UTF8_BOM=0xEFBBBF,UTF16LE_BOM=0XFFFE,UTF16BE_BOM=0XFEFF} BOM;

/** The struct for a codepoint glyph. */
typedef struct Glyph {
	unsigned char bytes[MAX_BYTES];
	endianness end;
	bool surrogate;
} Glyph;

/** The given filename. */
extern char* filename, *output_file;
extern int output_fd, verbosity;


/** The usage statement. */
#define USAGE_LENGTH 11
const char *USAGE[] = { 
	"Command line utility for converting files from UTF-16LE, UTF-16BE, UTF-8 to UTF-16LE or UTF-16BE.\n\n",
	"Usage: ./utf [-h|--help] [-v|-vv] -u OUT_ENC | --UTF=OUT_ENC IN_FILE [OUT_FILE]\n\n\t",
	"Option arguments:\n\t\t",
	"-h, --help\tDisplays this usage.\n\t\t",
	"-v, -vv\t\tToggles the verbosity of the program to level 1 or 2.\n\n\t",
	"Mandatory argument:\n\t\t",
	"-u OUT_ENC, --UTF=OUT_ENC\tSets the output encoding.\n\t\t\t\t\t\t",
	"Valid values for OUT_ENC: 16LE, 16BE\n\n\t",
	"Positional Arguments:\n\t\t",
	"IN_FILE\t\tThe file to convert.\n\t\t",
	"[OUT_FILE]\tOutput file name. If not present, defaults to stdout.\n"
};

/** Which endianness to convert to. */
extern endianness conversion;

/** Which endianness the source file is in. */
extern endianness source;

extern clock_t real_start, real_end,user_start, user_end, sys_start, sys_end;

/**
 * A function that swaps the endianness of the bytes of an encoding from
 * LE to BE and vice versa.
 *
 * @param glyph The pointer to the glyph struct to swap.
 * @return Returns a pointer to the glyph that has been swapped.
 */
Glyph* swap_endianness P((Glyph*));

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
Glyph* my_fill_glyph P((Glyph*, unsigned int[], endianness, int*));
Glyph* utf8_my_fill_glyph P((Glyph*, unsigned int, endianness, int*));

Glyph* convert P((Glyph*, endianness));

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
void print_help P((int,int));

/**
 * Closes file descriptors and frees list and possibly does other
 * bookkeeping before exiting.
 *
 * @param The fd int of the file the program has opened. Can be given
 * the macro value NO_FD (-1) to signify that we have no open file
 * to close.
 */
void quit_converter P((int, int));

#endif /*UTFCONVERTER_H*/
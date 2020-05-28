#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _POSIX_C_SOURCE
#include <unistd.h>
#elif defined(_WIN32)
#include <io.h>
#else
int isatty(int);
int fileno(FILE*);
#endif

#include "getline.h"
#include "getopt.h"
#include "diagnostic.h"
#include "output.h"
#include "formatter.h"
#include "label.h"
#include "debugger.h"
#include "interpreter.h"

#ifndef HEXPROC_VERSION
#define HEXPROC_VERSION "-"
#endif

#ifndef HEXPROC_DATE
#define HEXPROC_DATE "?"
#endif

#ifndef HEXPROC_COMPILER
#define HEXPROC_COMPILER "?"
#endif

const size_t io_buffer_size = 8 * 1024;
const size_t default_linebuffer_size = 256;

static void print_usage(void) {
	fprintf(stderr,
"Usage: hexproc [OPTION...] [FILE]\n"
"  -v          Print program version and exit\n"
"  -h          Print this help message and exit\n"
"  -b          Output binary data\n"
"  -B          Force output binary data (even when output is a TTY)\n"
"  -c          Output colored text\n"
"  -C          Force output colored text (even when output is not a TTY)\n"
"  -d          Enable debugger\n"
"See the manual page hexproc(1) for more information\n"
	);
}

static void print_version(void) {
	printf("Hexproc %s [%s, %s]\n",
		HEXPROC_VERSION, HEXPROC_DATE, HEXPROC_COMPILER);
}

int main(int argc, char **argv) {
	bool force_binary = false;
	bool force_color = false;

	opterr = 0; // disable 'getopt' error message
	int opt;
	while((opt = getopt(argc, argv, "vhbBdcC")) != -1) {
		switch (opt) {
			case 'h':
				print_usage();
				return 0;
			case 'v':
				print_version();
				return 0;
			case 'B':
				force_binary = true;
				output_mode = OUTPUT_BINARY;
				break;
			case 'b':
				output_mode = OUTPUT_BINARY;
				break;
			case 'd':
				debug_mode = true;
				break;
			case 'C':
				force_color = true;
				output_mode = OUTPUT_HEX_COLOR;
				break;
			case 'c':
				output_mode = OUTPUT_HEX_COLOR;
				break;
			default:
				fprintf(stderr, "Unknown option: %s\n", argv[optind]);
				print_usage();
				return EINVAL; // invalid argument
		}
	}

	if(isatty(fileno(stdout)) && (output_mode == OUTPUT_BINARY) && !force_binary) {
		fprintf(stderr, "Refusing to write binary data to console, use '-B' to override\n");
		return 1;
	}
	if(!isatty(fileno(stdout)) && (output_mode == OUTPUT_HEX_COLOR) && !force_color) {
		fprintf(stderr, "Refusing to write colored output to a non-tty, use '-C' to override\n");
		output_mode = OUTPUT_HEX;
		// not a fatal error, no need to exit
	}

	FILE *input;

	if(optind >= argc) {
		// no file argument given
		input = stdin;
		current_file_name = "<stdin>";
	} else {
		input = fopen(argv[optind], "r");
		current_file_name = argv[optind];
		if(!input) {
			fprintf(stderr, "Couldn't open file \"%s\" (error %d)\n", argv[optind], (int) errno);
			return errno;
		}
	}

	char *line = malloc(default_linebuffer_size);
	size_t len = default_linebuffer_size;
	ssize_t nread;
	/* buffers for input/output streams, allocated with a single malloc call */
	char *shared_buffer = malloc(io_buffer_size * 2);


	if(!debug_mode)
		setvbuf(input, shared_buffer, _IOFBF, io_buffer_size);

	FILE *output = stdout;
	if(!debug_mode)
		setvbuf(output, shared_buffer + io_buffer_size, _IOFBF, io_buffer_size);

	add_constant_label(strdup("LE"), 0);
	add_constant_label(strdup("BE"), 1);
	add_constant_label(strdup("__endian__"), 1);

	if(debug_mode)
		enter_debugger();

	struct bytequeue buffer = make_bytequeue();

	while((nread = getline(&line, &len, input)) != -1) {
		line_number++;
		if(debug_mode && (exists_breakpoint(line_number) || break_on_next)) {
			break_on_next = false;
			enter_debugger();
		}
		process_line(line, &buffer);
	}

	bytequeue_rewind(&buffer);

	line_number = 1;
	offset = 0;

	for(int byte; (byte = bytequeue_get(&buffer)) != EOF;)
		output_byte(byte, output);

	finalize_output(output);

	fflush(output);
	if(output != stdout)
		fclose(output);
	if(input != stdin)
		fclose(input);

	cleanup_formatters();
	cleanup_labels();
	cleanup_breakpoints();
	cleanup_sourcemap();

	free_bytequeue(buffer);

	OPTIONAL_FREE(line);
	OPTIONAL_FREE(shared_buffer);
	return 0;
}

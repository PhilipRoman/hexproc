#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _POSIX_C_SOURCE
#include <unistd.h>
#elif defined(_WIN32)
#include <io.h>
#endif

#include "getline.h"
#include "getopt.h"
#include "diagnostic.h"
#include "output.h"
#include "formatter.h"
#include "label.h"
#include "debugger.h"
#include "interpreter.h"

const size_t io_buffer_size = 32 * 1024;
const size_t default_linebuffer_size = 256;

static void print_usage(void) {
	fprintf(stderr,
"Usage: hexproc [OPTION...] [FILE]\n"
"    -v          Print program version and exit\n"
"    -h          Print this help message and exit\n"
"    -b          Output binary data\n"
"    -B          Force output binary data (even when output is a TTY)\n"
"    -d          Enable debugger\n"
	);
}

static void print_version(void) {
	printf("Hexproc %s [%s]\n", HEXPROC_VERSION, HEXPROC_DATE);
}

int main(int argc, char **argv) {
	opterr = 0; // disable 'getopt' error message
	int opt;
	while((opt = getopt(argc, argv, "vhbBd")) != -1) {
		switch (opt) {
			case 'h':
				print_usage();
				return 0;
			case 'v':
				print_version();
				return 0;
			case 'B':
				output_mode = OUTPUT_BINARY;
				break;
			case 'b':
				if(isatty(fileno(stdin)))
					fprintf(stderr, "Refusing to write binary data to console, use '-B' to override\n");
				else
					output_mode = OUTPUT_BINARY;
				break;
			case 'd':
				debug_mode = true;
				break;
			default:
				fprintf(stderr, "Unknown option: %s\n", argv[optind]);
				print_usage();
				return EINVAL; // invalid argument
		}
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
	/* buffers for streams, allocated with a single malloc call */
	char *shared_buffer = malloc(io_buffer_size * 3);

	if(!debug_mode)
		setvbuf(input, shared_buffer, _IOFBF, io_buffer_size);

	FILE *tmp = tmpfile();
	setvbuf(tmp, shared_buffer + io_buffer_size, _IOFBF, io_buffer_size);

	if(debug_mode)
		enter_debugger();

	while((nread = getline(&line, &len, input)) != -1) {
		line_number++;
		if(debug_mode && (exists_breakpoint(line_number) || break_on_next)) {
			break_on_next = false;
			enter_debugger();
		}
		process_line(line, tmp);
	}

	rewind(tmp);
	line_number = 1;

	FILE *output = stdout;
	if(!debug_mode)
		setvbuf(output, shared_buffer + 2 * io_buffer_size, _IOFBF, io_buffer_size);

	while((nread = getline(&line, &len, tmp)) != -1) {
		output_line(line, output);
		line_number++;
	}

	fclose(tmp);

	fflush(output);
	if(output != stdout)
		fclose(output);

	cleanup_formatters();
	cleanup_labels();

	OPTIONAL_FREE(line);
	OPTIONAL_FREE(shared_buffer);
	return 0;
}

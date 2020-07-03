#include <stdlib.h>

#ifdef _POSIX_C_SOURCE
#include <unistd.h>
#elif defined(_WIN32)
#include <io.h>
// Since Windows is a special snowflake,
// here is a quick fix to get strtold to recognize hex numbers.
// Keep in mind that <stdlib.h> has to be included before this line
#define strtold(...) strtod(__VA_ARGS__)
#define __USE_MINGW_ANSI_STDIO 1
#include <stdio.h>
#else
#include <stdio.h>
int isatty(int);
int fileno(FILE*);
#endif

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

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
"  -V          Print program version and configuration and exit\n"
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
	printf("Hexproc %s\n", HEXPROC_VERSION);
}

static void print_configuration(void) {
#	ifdef HAVE_HP_FLOAT80
	#define HAVE_HP_FLOAT80_YESNO "yes"
#else
	#define HAVE_HP_FLOAT80_YESNO "no"
#endif
#	ifdef HAVE_HP_FLOAT128
	#define HAVE_HP_FLOAT128_YESNO "yes"
#else
	#define HAVE_HP_FLOAT128_YESNO "no"
#endif
#	ifdef HAVE_HP_INT128
	#define HAVE_HP_INT128_YESNO "yes"
#else
	#define HAVE_HP_INT128_YESNO "no"
#endif
	printf(
	"Version:         " HEXPROC_VERSION "\n"
	"Build timestamp: " HEXPROC_DATE "\n"
	"Compiler:        " HEXPROC_COMPILER "\n"
	"Have float80?  " HAVE_HP_FLOAT80_YESNO "\n"
	"Have float128? " HAVE_HP_FLOAT128_YESNO "\n"
	"Have int128?   " HAVE_HP_INT128_YESNO "\n"
	"Float expression type: " CALC_FLOAT_TYPENAME "\n"
	"Int expression type:   " CALC_INT_TYPENAME "\n"
	"Max expression call stack depth: %d\n"
	"Max number of expression tokens: %d\n"
	"=== Internal structure information ===\n"
	"Sizeof struct formatter: %d\n"
	"Sizeof struct label: %d\n"
	"Sizeof struct sourcemap: %d\n",
	(int)NAME_STACK_SIZE,
	(int)YARD_QUEUE_SIZE,
	(int)sizeof(struct formatter),
	(int)sizeof(struct label),
	(int)sizeof(struct sourcemap_entry)
	);
#undef HAVE_HP_FLOAT80_YESNO
#undef HAVE_HP_FLOAT128_YESNO
#undef HAVE_HP_INT128_YESNO
}

// special variables
static void add_builtin_variables(void) {
	set_constant_label(strdup("LE"), 0);
	set_constant_label(strdup("BE"), 1);
	set_constant_label(strdup("hexproc.endian"), 1);
	const char *version = HEXPROC_VERSION;
	long major = 0, minor = 0, patch = 0;
	char *end;
	version++; // skip the 'v' prefix
	major = strtol(version, &end, 0);
	if((version = end)[0] == '.')
		minor = strtol(++version, &end, 0);
	if((version = end)[0] == '.')
		patch = strtol(++version, &end, 0);

	set_constant_label(strdup("hexproc.major"), major);
	set_constant_label(strdup("hexproc.minor"), minor);
	set_constant_label(strdup("hexproc.patch"), patch);
}

void reset_terminal(void) {
	if(isatty(fileno(stdout)))
		fprintf(stdout, "\033[0m");
	if(isatty(fileno(stderr)))
		fprintf(stderr, "\033[0m");
}

int main(int argc, char **argv) {
	bool force_binary = false;
	bool force_color = false;

	opterr = 0; // disable 'getopt' error message
	int opt;
	while((opt = getopt(argc, argv, "vVhbBdcC")) != -1) {
		switch (opt) {
			case 'h':
				print_usage();
				return 0;
			case 'v':
				print_version();
				return 0;
			case 'V':
				print_configuration();
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

	if(debug_mode)
		atexit(reset_terminal);

	if(optind >= argc) {
		// no file argument given
		current_input = stdin;
		current_file_name = "<stdin>";
	} else {
		current_input = fopen(argv[optind], "r");
		current_file_name = argv[optind];
		if(!current_input) {
			fprintf(stderr, "Couldn't open file \"%s\" (error %d)\n", argv[optind], (int) errno);
			return errno;
		}
	}

	char *line = malloc(default_linebuffer_size);
	size_t len = default_linebuffer_size;
	ssize_t nread;
	/* buffers for input/output streams, allocated with a single malloc call */
	char *shared_buffer = malloc(io_buffer_size * 2);


	if(!debug_mode && !isatty(fileno(current_input)))
		setvbuf(current_input, shared_buffer, _IOFBF, io_buffer_size);

	FILE *output = stdout;
	if(!debug_mode && !isatty(fileno(output)))
		setvbuf(output, shared_buffer + io_buffer_size, _IOFBF, io_buffer_size);

	add_builtin_variables();

	if(debug_mode && isatty(fileno(stdin)))
		if(signal(SIGINT, enter_debugger_async) == SIG_ERR)
			fprintf(stderr, "Internal error: could not setup signal handler for SIGINT, errno = %d\n", errno);

	if(debug_mode && isatty(fileno(stdin)))
		enter_debugger();

	struct bytequeue buffer = make_bytequeue();

	while((nread = getline(&line, &len, current_input)) != -1 || !feof(current_input)) {
		clearerr(current_input);
		line_number++;
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
	if(current_input != stdin)
		fclose(current_input);

	cleanup_formatters();
	cleanup_labels();
	cleanup_breakpoints();
	cleanup_sourcemap();

	free_bytequeue(buffer);

	OPTIONAL_FREE(line);
	OPTIONAL_FREE(shared_buffer);
	return 0;
}

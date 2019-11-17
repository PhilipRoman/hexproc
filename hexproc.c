#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _POSIX_C_SOURCE
#include <unistd.h>
#elif defined(_WIN32)
#include <io.h>
#endif

#include "getline.h"
#include "getopt.h"
#include "error.h"
#include "label.h"
#include "formatter.h"
#include "text.h"
#include "calc.h"
#include "output.h"
#include "debugger.h"

uint64_t offset = 0;

const char * const HEX_DIGITS = "0123456789abcdef";

static inline void buffer_append_octet(FILE *buffer, char high, char low) {
	char temp[3];
	temp[0] = high;
	temp[1] = low;
	temp[2] = ' ';
	fwrite(temp, sizeof(temp), 1, buffer);
}

static inline void buffer_append_newline(FILE *buffer) {
	char newline  = '\n';
	fwrite(&newline, 1, 1, buffer);
}

void process_line(char *line, FILE *buffer) {
	start:
	line += scan_whitespace(line);
	while(line[0]) {
		switch(line[0]) {
			case '/': {
				line++;
				if(line[0] == '/')
					goto end_loop;
				break;
			}
			case '#': {
				uint64_t linenum;
				const char *filename;
				if(scan_line_marker(line, &linenum, &filename)) {
					// free(current_file_name);
					current_file_name = filename;
					line_number = linenum - 1;
				}
				goto end_loop;
			}
			case ';': {
				line++;
				goto start;
			}
			case '[': {
				struct formatter formatter;
				line += scan_formatter(line, &formatter);
				offset += formatter.nbytes;
				add_formatter(formatter);
				buffer_append_octet(buffer, '?', HEX_DIGITS[formatter.nbytes]);
				break;
			}
			case '"': {
				// text doesn't include quotes
				const char *literal = line + 1;
				// size includes quotes
				size_t literal_size = scan_quoted_string(line);
				size_t nbytes = literal_size - 2; // don't count the quotes
				offset += nbytes;
				line += literal_size;
				for(size_t i = 0; i < nbytes; i++)
					buffer_append_octet(buffer,
						HEX_DIGITS[(literal[i] >> 4) & 0xf],
						HEX_DIGITS[literal[i] & 0xf]
					);
				break;
			}
			default: {
				// first, try matching an assignment
				const char *key, *value;
				enum assign_mode mode;
				size_t assignment_size = try_scan_assign(line, &key, &value, &mode);
				if(assignment_size) {
					switch(mode) {
						case ASSIGN_LABEL:
							add_constant_label(key, offset);
							break;
						case ASSIGN_LAZY:
							add_expr_label(key, value);
							break;
						case ASSIGN_IMMEDIATE:
							add_constant_label(key, calc(value));
							break;
					}
					line += assignment_size;
					break;
				}
				// then, fall back to matching hex values
				char octet[] = {'\0', '\0', '\0'};
				line += scan_octet(line, octet);
				fprintf(buffer, "%s ", octet);
				offset++;
				break;
			}
		}
		line += scan_whitespace(line);
	}
	end_loop:
	buffer_append_newline(buffer);
}

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

	setvbuf(input, shared_buffer, _IOFBF, io_buffer_size);

	FILE *tmp = tmpfile();
	setvbuf(tmp, shared_buffer + io_buffer_size, _IOFBF, io_buffer_size);

	while((nread = getline(&line, &len, input)) != -1) {
		process_line(line, tmp);
		line_number++;
	}

	if(input != stdin)
		fclose(input);

	rewind(tmp);
	line_number = 1;

	FILE *output = stdout;
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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "posix.h"
#include "error.h"
#include "label.h"
#include "formatter.h"
#include "text.h"
#include "calc.h"

unsigned offset = 0;

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
	const char *label_key;
	const char *label_value;
	size_t label_length;
	if(scan_label_line(line, &label_key, &label_value, &label_length)) {
		if(label_value)
			add_expr_label(label_key, label_value);
		else
			add_constant_label(label_key, offset);
		line += label_length;
		goto start;
	}
	while(line[0]) {
		switch(line[0]) {
			case '/': {
				line++;
				if(line[0] == '/')
					goto end_loop;
				break;
			}
			case '#': {
				goto end_loop;
			}
			case ';': {
				line++;
				goto start;
			}
			case '[': {
				struct hp_formatter formatter;
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

void output_line(const char *line, FILE *output) {
	line += scan_whitespace(line);
	int first = 1;
	while(line[0]) {
		if(line[0] == '?') {
			line += 2;
			struct hp_formatter formatter = take_next_formatter();
			double result = calc(formatter.expr);
			uint8_t buf[8];
			size_t nbytes;
			format_value(result, formatter, buf, &nbytes);
			for(size_t i = 0; i < nbytes; i++) {
				if(first)
					first = 0;
				else
					fputc(' ', output);
				fprintf(output, "%02x", (unsigned int) buf[i]);
			}
		} else {
			char octet[3];
			line += scan_octet(line, octet);
			octet[2] = '\0';
			if(!first)
				fputc(' ', output);
			fputs(octet, output);
		}
		first = 0;
		line += scan_whitespace(line);
	}
	fprintf(output, "\n");
}

const size_t io_buffer_size = 32 * 1024;
const size_t default_linebuffer_size = 256;

void print_usage(void) {
			fprintf(stderr,
"Usage: hexproc [OPTION|FILE]\n"
"    -v, --version             Print program version and exit\n"
"    -h, --help                Print this help message and exit\n"
			);
}

int main(int argc, char **argv) {
	if(argc >= 3) {
		fprintf(stderr, "Too many arguments\n");
		print_usage();
		return 0;
	}
	if(argc == 2) {
		if(!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")) {
			printf("Hexproc %s [%s]\n", HEXPROC_VERSION, HEXPROC_DATE);
			return 0;
		}
		if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			print_usage();
			return 0;
		}
	}

	FILE *input;

	if(argc == 1) {
		input = stdin;
	} else {
		input = fopen(argv[1], "r");
		if(!input) {
			fprintf(stderr, "Couldn't open file \"%s\" (error %d)\n", argv[1], (int) errno);
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

	rewind(tmp);
	line_number = 1;

	FILE *output = stdout;
	setvbuf(output, shared_buffer + 2 * io_buffer_size, _IOFBF, io_buffer_size);

	while((nread = getline(&line, &len, tmp)) != -1) {
		output_line(line, output);
		line_number++;
	}

	fflush(output);

	cleanup_formatters();
	cleanup_labels();

	OPTIONAL_FREE(line);
	OPTIONAL_FREE(shared_buffer);
	return 0;
}

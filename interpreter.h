#include <stdio.h>
#include <stdint.h>

#include "diagnostic.h"
#include "label.h"
#include "formatter.h"
#include "text.h"
#include "calc.h"

/* The current byte offset */
uint64_t offset = 0;
const char * const HEX_DIGITS = "0123456789abcdef";

static inline void buffer_append_octet(FILE *buffer, char high, char low) {
	char temp[3];
	temp[0] = high;
	temp[1] = low;
	temp[2] = ' ';
	fwrite(temp, sizeof(temp), 1, buffer);
}

/* Runs first-pass processing on the given line and
	writes intermediate results to the buffer file. */
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
	fputc('\n', buffer);
}

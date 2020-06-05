#pragma once

#include <stdio.h>
#include <stdint.h>

#include "diagnostic.h"
#include "label.h"
#include "formatter.h"
#include "text.h"
#include "calc.h"
#include "sourcemap.h"
#include "bytequeue.h"

struct bytequeue buffer;

/* The current byte offset */
uint64_t offset = 0;

/* Runs first-pass processing on the given line and
	writes intermediate results to the buffer file. */
void process_line(const char *line, struct bytequeue *buffer) {
	start:
	line += scan_whitespace(line);
	while(line[0]) {
		textfail = false;
		switch(line[0]) {
			case '/': {
				if((++line)[0] == '/')
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
				if(textfail)
					goto end_loop;
				add_formatter(formatter);
				add_sourcemap_entry(offset, SOURCE_FORMATTER);
				offset += formatter.nbytes;
				add_sourcemap_entry(offset, SOURCE_END);
				break;
			}
			case '"': {
				// text doesn't include quotes
				const char *literal = line + 1;
				// size includes quotes
				size_t literal_size = scan_quoted_string(line);
				if(textfail)
					goto end_loop;
				size_t nbytes = literal_size - 2; // don't count the quotes
				add_sourcemap_entry(offset, SOURCE_STRING);
				offset += nbytes;
				add_sourcemap_entry(offset, SOURCE_END);
				line += literal_size;
				for(size_t i = 0; i < nbytes; i++)
					bytequeue_put(buffer, literal[i]);
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
				int byte;
				line += scan_octet(line, &byte);
				if(textfail)
					break;
				bytequeue_put(buffer, byte);
				offset++;
				break;
			}
		}
		line += scan_whitespace(line);
	}
	end_loop:
	textfail = false;
	add_sourcemap_entry(offset, SOURCE_NEWLINE);
}

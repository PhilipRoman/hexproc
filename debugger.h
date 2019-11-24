#pragma once

#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>

#include "label.h"
/* True if debugger has been enabled */
bool debug_mode = false;
/* True if the debugger should be entered on next line */
bool break_on_next = false;

// in practice there are only a few breakpoints, so linear searching is okay
/* Array of breakpoints */
uint64_t *breaklist = NULL;
unsigned breaklist_len = 0;
unsigned breaklist_cap = 0;

bool exists_breakpoint(uint64_t b) {
	for(int i = 0; i < breaklist_len; i++)
		if(breaklist[i] == b)
			return true;
	return false;
}

void cleanup_breakpoints(void) {
	free(breaklist);
}

static void add_breakpoint(uint64_t linenum) {
	if(exists_breakpoint(linenum))
		return;

	if(breaklist_len >= breaklist_cap) {
		breaklist_cap = (breaklist_cap == 0) ? 16 : breaklist_cap * 3;
		breaklist = realloc(breaklist, breaklist_cap * sizeof(breaklist[0]));
	}
	breaklist[breaklist_len++] = linenum;
}

bool handle_debug_command(void) {
	// always scanf 1 less character than buffer size
	char command[32];
	scanf("%31s", command);
	if(!strcmp("break", command) || !strcmp("brk", command)) {
		uint64_t parameter;
		scanf("%"SCNu64, &parameter);
		add_breakpoint(parameter);
	} else if(!strcmp("resume", command) || !strcmp("run", command)) {
		return false;
	} else if(!strcmp("step", command) || !strcmp("s", command)) {
		break_on_next = true;
		return false;
	} else if(!strcmp("vars", command)) {
		for(unsigned i = 0; i < labellist_len; i++) {
			struct label label = labellist[i];
			if(label.expr)
				printf("\t%s -> \"%s\"\n", label.name, label.expr);
			else
				printf("\t%s -> \"%d\"\n", label.name, label.constant);
		}
	} else if(!strcmp("help", command)) {
		printf("\tbreak NUMBER - set a breakpoint on a line\n");
		printf("\trun - resume execution\n");
	} else {
		printf("Unknown command: \"%s\"\n", command);
	}
	return true;
}

void enter_debugger(void) {
	printf("Entered debug mode before line %"PRIu64"\n", line_number);
	do {
		printf("debug[%s:%"PRIu64"]> ", current_file_name, line_number);
	} while(handle_debug_command());
}

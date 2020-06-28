#pragma once

#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>

#include "label.h"
#include "diagnostic.h"
/* True if debugger has been enabled */
bool debug_mode = false;
/* True if the debugger should be entered on next line */
bool break_on_next = false;

// in practice there are only a few breakpoints, so linear searching is okay
/* Array of breakpoints */
uint64_t *breaklist = NULL;
unsigned breaklist_len = 0;
unsigned breaklist_cap = 0;

// returns false if the debugger should exit here
typedef bool debugger_function(void);

// some commands implemented at the end, for readability
bool debugger_add_break(void);
bool debugger_resume(void) { return false; }
bool debugger_step(void) { break_on_next = true; return false; }
bool debugger_vars(void);
bool debugger_help(void);

const struct debugger_command {
	char name[32];
	debugger_function *func;
} debugger_commands[] = {
	{"break",  &debugger_add_break},
	{"brk",    &debugger_add_break},
	{"resume", &debugger_resume},
	{"run",    &debugger_resume},
	{"step",   &debugger_step},
	{"s",      &debugger_step},
	{"v",      &debugger_vars},
	{"vars",   &debugger_vars},
	{"l",      &debugger_vars},
	{"list",   &debugger_vars},
	{"h",      &debugger_help},
	{"help",   &debugger_help},
	{"?",      &debugger_help},
	{{0}, NULL}
};

/* returns false if the debugger should exit here */
bool run_debug_command(void) {
	/* always scanf 1 less character than buffer size */
	char command[32];
	if(fscanf(stdin, "%31s", command) == EOF)
		return false;
	for(unsigned i = 0; debugger_commands[i].func != NULL; i++)
		if(!strcmp(command, debugger_commands[i].name))
			return debugger_commands[i].func();
	fprintf(stderr, "  Unknown command: \"%s\"\n", command);
	return true;
}

void enter_debugger() {
	fprintf(stderr, "  Entered debug mode before line %"PRIu64"\n", line_number);
	do {
		fprintf(stderr, "DEBUGGER %s:%"PRIu64"> ", current_file_name, line_number);
	} while(run_debug_command());
}

// implementations of debugger commands
bool debugger_vars(void) {
	fprintf(stderr, "  List of variables:\n");
	for(unsigned i = 0; i < 64; i++) {
		struct label *label = &labelmap[i];
		while(label && label->name) {
			if(label->expr)
				fprintf(stderr, "    %2u: %16s -> \"%s\"\n",
					i, label->name, label->expr);
			else
				fprintf(stderr, "    %2u: %16s -> %u\n",
					i, label->name, label->constant);
			label = label->next;
		}
	}
	return true;
}

bool exists_breakpoint(uint64_t linenum) {
	for(unsigned i = 0; i < breaklist_len; i++)
		if(breaklist[i] == linenum)
			return true;
	return false;
}

bool debugger_add_break(void) {
	uint64_t linenum;
	if(fscanf(stdin, "%"SCNu64, &linenum) == EOF)
		return true;
	if(exists_breakpoint(linenum))
		return true;
	if(breaklist_len >= breaklist_cap) {
		breaklist_cap = (breaklist_cap == 0) ? 16 : breaklist_cap * 3;
		breaklist = realloc(breaklist, breaklist_cap*sizeof(breaklist[0]));
	}
	breaklist[breaklist_len++] = linenum;
	fprintf(stderr, "Added breakpoint before line %"PRIu64"\n", linenum);
	return true;
}

void cleanup_breakpoints(void) {
	free(breaklist);
}

bool debugger_help(void) {
	fprintf(stderr,
	"  Available commands:\n"
	"    break NUMBER - set a breakpoint before given line number\n"
	"    resume - resume execution\n"
	"    vars - list current variables\n"
	"    help - show debugger usage help\n"
	"  See the manual page hexproc(1) for more info\n"
	);
	return true;
}

#pragma once

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include "label.h"
#include "calc.h"
#include "diagnostic.h"
/* True if debugger has been enabled */
bool debug_mode = false;
/* True if the debugger should be entered on next line */
volatile bool break_on_next = false;
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
bool debugger_eval(void);

const struct {
	char name[32];
	debugger_function *func;
} debugger_commands[] = {
	{"break",  &debugger_add_break},
	{"b",    &debugger_add_break},
	{"resume", &debugger_resume},
	{"r",    &debugger_resume},
	{"step",   &debugger_step},
	{"s",      &debugger_step},
	{"vars",   &debugger_vars},
	{"v",      &debugger_vars},
	{"list",   &debugger_vars},
	{"l",      &debugger_vars},
	{"help",   &debugger_help},
	{"h",      &debugger_help},
	{"?",      &debugger_help},
	{"eval",   &debugger_eval},
	{"e",      &debugger_eval},
	{"=",      &debugger_eval},
	{{0}, NULL}
};

/* returns false if the debugger should exit here */
bool run_debug_command(void) {
	/* always scanf 1 less character than buffer size */
	char command[32];
	switch(fscanf(stdin, "%31s", command)) {
		case EOF:
			return false;
		case 0:
			return true;
	}
	for(unsigned i = 0; debugger_commands[i].func != NULL; i++)
		if(!strcmp(command, debugger_commands[i].name))
			return debugger_commands[i].func();
	fprintf(stderr, "  Unknown command: \"%s\"\n", command);
	return true;
}

/* signature of this method should be compatible with 'signal' api */
void enter_debugger_async(int unused) {
	break_on_next = true;
}
void enter_debugger() {
	if(signal(SIGINT, SIG_DFL) == SIG_ERR)
		fprintf(stderr, "Internal error: could not setup signal handler for SIGINT, errno = %d\n", errno);

	do {
		fprintf(stderr, "\033[1m" "debug %s:%"PRIu64"> " "\033[0m", current_file_name, line_number);
	} while(run_debug_command());

	if(signal(SIGINT, enter_debugger_async) == SIG_ERR)
		fprintf(stderr, "Internal error: could not setup signal handler for SIGINT, errno = %d\n", errno);
	if(isatty(fileno(stderr)))
		fprintf(stderr, "\033[0m"); // reset terminal color
}

// implementations of debugger commands
bool debugger_vars(void) {
	fprintf(stderr, "  List of variables:\n");
	for(unsigned i = 0; i < 64; i++) {
		struct label *label = &labelmap[i];
		while(label && label->name) {
			if(label->expr)
				fprintf(stderr, "\t%2u: %16s = \"%s\"\n",
					i, label->name, label->expr);
			else
				fprintf(stderr, "\t%2u: %16s = %u\n",
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
	"    b, break NUMBER - set a breakpoint before given line number\n"
	"    r, resume - resume execution\n"
	"    v, vars - list current variables\n"
	"    l, list - list current variables\n"
	"    h, help - show debugger usage help\n"
	"    e, eval - evaluate an expression\n"
	"  See the manual page hexproc(1) for more info\n"
	);
	return true;
}

bool debugger_eval(void) {
	char expr[64];
	/* always scanf 1 less character than buffer size */
	fscanf(stdin, "%63[^\n]", expr);
	calc_float_t result = calc(expr);
	if((long long)result == result)
		fprintf(stderr, "= %lld\n", (long long)result);
	else
		fprintf(stderr, "= %Lf\n", (long double)result);
	return true;
}

#pragma once

#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "diagnostic.h"
#include "text.h"
#include "label.h"

#define YARD_STACK_SIZE 64
#define YARD_QUEUE_SIZE 64
#define OPERAND_STACK_SIZE 64
#define NAME_STACK_SIZE 64

/* Either an operator or a numeric value */
struct yard_value {
	union {
		long double num;
		char op;
	} content;
	char isnum;
};

struct yard {
	unsigned slen; // stack length
	unsigned qlen; // queue length
	char stack[YARD_STACK_SIZE];
	struct yard_value queue[YARD_QUEUE_SIZE];
};

struct operand_stack {
	int len;
	long double stack[OPERAND_STACK_SIZE];
};

long double calc(const char *expr);

// IMPLEMENTATION STARTS HERE

/* The precedence of given operator */
int prec(char op) {
	switch(op) {
		case '&':
		case '|':
		case '~': return 50;
		case '+':
		case '-': return 100;
		case '*':
		case '/':
		case '%': return 200;
		case '^': return 300;
		default: {
			report_error("Bad operator: (char %d)", (int) op);
			return 0;
		}
	}
}

/* Returns true if the operator is left-associative */
bool leftassoc(char op) {
	switch(op) {
		case '^': return 0;
		default: return 1;
	}
}

/* Attempts to convert the long double to an integer,
	handling the case when it has no such representation */
long to_integer(long double d) {
	if(!isfinite(d)) {
		report_error("%Lf cannot be converted to an integer", d);
		return 0;
	}
	return (long)d;
}

/* evaluates the result of two arguments applied to
	binary operator */
long double op_eval(char op, long double a, long double b) {
	switch(op) {
		case '+': return a + b;
		case '-': return a - b;
		case '*': return a * b;
		case '/': return a / b;
		case '%': return fmodl(a, b);
		case '^': return powl(a, b);
		case '&': return to_integer(a) & to_integer(b);
		case '|': return to_integer(a) | to_integer(b);
		case '~': return to_integer(a) ^ to_integer(b);
		default: {
			report_error("Bad operator (char %d)", (int) op);
			return NAN;
		};
	}
}

/* appends the value to end of queue */
void yard_put(struct yard *yard, struct yard_value v) {
	if(yard->qlen >= YARD_QUEUE_SIZE) {
		report_error("Shunting yard queue overflow");
		return;
	}
	yard->queue[yard->qlen++] = v;
}

/* puts the value on top of stack */
void yard_push(struct yard *yard, char op) {
	if(yard->slen >= YARD_STACK_SIZE) {
		report_error("Shunting yard stack overflow");
		return;
	}
	yard->stack[yard->slen++] = op;
}

/* returns and removes the operator at the top of stack */
char yard_pop(struct yard *yard) {
	if(!yard->slen) {
		report_error("Shunting yard stack underflow");
		return '?';
	}
	return yard->stack[--yard->slen];
}

/* returns the operator at the top of stack */
char yard_peek(struct yard *yard) {
	if(!yard->slen) {
		report_error("Shunting yard stack underflow");
		return '?';
	}
	return yard->stack[yard->slen-1];
}

void yard_add_num(struct yard *yard, long double x) {
	struct yard_value value = {
		.isnum = 1,
		.content = {.num = x},
	};
	yard_put(yard, value);
}

void yard_add_op(struct yard *yard, char op) {
	while(
		/* while stack has elements */
			yard->slen
		&& (
			/* and precedence of top element is greater */
			prec(yard_peek(yard)) > prec(op)
			||
			/* ...or equal, but top is left-associative*/
			(prec(yard_peek(yard)) == prec(op) && leftassoc(yard_peek(yard)))
		)
	) {
		struct yard_value value = {
			.isnum = 0,
			.content = {.op = yard_pop(yard)},
		};
		yard_put(yard, value);
	}
	yard_push(yard, op);
}

void operand_push(struct operand_stack *stack, long double x) {
	if(stack->len >= OPERAND_STACK_SIZE) {
		report_error("Operand stack overflow");
		return;
	}
	stack->stack[stack->len++] = x;
}

long double operand_pop(struct operand_stack *stack) {
	if(!stack->len) {
		report_error("Operand stack underflow");
		return NAN;
	}
	return stack->stack[--stack->len];
}

long double yard_compute(struct yard *yard) {
	/* pop remaining operators into output queue */
	while(yard->slen) {
		struct yard_value value = {
			.isnum = 0,
			.content = {.op = yard_pop(yard)},
		};
		yard_put(yard, value);
	}
	struct operand_stack stack = {.len = 0};
	for(unsigned i = 0; i < yard->qlen; i++) {
		struct yard_value v = yard->queue[i];
		if(v.isnum) {
			operand_push(&stack, v.content.num);
		} else {
			long double b = operand_pop(&stack);
			long double a = operand_pop(&stack);
			long double result = op_eval(v.content.op, a, b);
			operand_push(&stack, result);
		}
	}
	return operand_pop(&stack);
}

const char *namestack[NAME_STACK_SIZE];
int namestack_len = 0;

void namestack_push(const char *name) {
	if(namestack_len >= NAME_STACK_SIZE) {
		report_error("Name stack overflow");
		return;
	}
	namestack[namestack_len++] = name;
}

void namestack_pop(void) {
	if(!namestack_len) {
		report_error("Name stack underflow");
		return;
	}
	namestack_len--;
}

int namestack_contains(const char *name) {
	for(unsigned i = 0; i < namestack_len; i++)
		if(!strcmp(name, namestack[i]))
			return 1;
	return 0;
}

long double calc(const char *expr) {
	struct yard yard = {.slen = 0, .qlen = 0};
	expr += scan_whitespace(expr);
	while(expr[0]) {
		if(isdigit(expr[0])) {
			const char *numstr;
			expr += scan_name(expr, &numstr);
			long double num = strtold(numstr, NULL);
			free((char*) numstr);
			yard_add_num(&yard, num);
		} else if(expr[0] == '.' || expr[0] == '_' || isalpha(expr[0])) {
			const char *name;
			expr += scan_name(expr, &name);
			struct label *label = lookup_label(name);
			long double result;
			if(!label) {
				report_error("Unknown identifier: \"%s\"", name);
				result = NAN;
			} else if(label->expr) {
				if(namestack_contains(name)) {
					report_error("Recursive label: \"%s\"", name);
					free((char*) name);
					return NAN;
				}
				namestack_push(name);
				result = calc(label->expr);
				namestack_pop();
			} else {
				result = label->constant;
			}
			free((char*) name);
			yard_add_num(&yard, result);
		} else {
			char op = expr[0];
			expr++;
			yard_add_op(&yard, op);
		}
		expr += scan_whitespace(expr);
	}
	return yard_compute(&yard);
}

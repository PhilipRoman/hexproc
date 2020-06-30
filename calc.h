#pragma once

#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "diagnostic.h"
#include "label.h"
#include "largenum.h"

typedef hp_maxfloat_t calc_float_t;
typedef hp_maxint_t calc_int_t;
calc_float_t calc(const char *expr);

#include "text.h"

bool mathfail = false;

#define YARD_STACK_SIZE 64
#define YARD_QUEUE_SIZE 64
#define OPERAND_STACK_SIZE 64
#define NAME_STACK_SIZE 64

/* Either an operator or a numeric value */
struct yard_value {
	union {
		calc_float_t num;
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
	unsigned len;
	calc_float_t stack[OPERAND_STACK_SIZE];
};

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
		case '(':
		case ')': return 1000;
		default: {
			// don't bother reporting errors, other functions will do it
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

/* Attempts to convert the float to an integer,
	handling the case when it has no such representation */
calc_int_t to_integer(calc_float_t d) {
	if(!isfinite(d)) {
		report_error("%Lf cannot be converted to an integer", (long double)d);
		return 0;
	}
	return (calc_int_t)d;
}

/* evaluates the result of two arguments applied to
	binary operator */
calc_float_t op_eval(char op, calc_float_t a, calc_float_t b) {
	switch(op) {
		case '+': return a + b;
		case '-': return a - b;
		case '*': return a * b;
		case '/': return a / b;
		case '%': return fmodl((long double)a, (long double)b);
		case '^': return powl((long double)a, (long double)b);
		case '&': return to_integer(a) & to_integer(b);
		case '|': return to_integer(a) | to_integer(b);
		case '~': return to_integer(a) ^ to_integer(b);
		default: {
			mathfail = true;
			report_error("Bad operator ((char)%d = '%c')",
				(int)op, (char)op);
			return NAN;
		};
	}
}

/* appends the value to end of queue */
void yard_put(struct yard *yard, struct yard_value v) {
	if(yard->qlen >= YARD_QUEUE_SIZE) {
		mathfail = true;
		report_error("Shunting yard queue overflow");
		return;
	}
	yard->queue[yard->qlen++] = v;
}

/* puts the value on top of stack */
void yard_push(struct yard *yard, char op) {
	if(yard->slen >= YARD_STACK_SIZE) {
		mathfail = true;
		report_error("Shunting yard stack overflow");
		return;
	}
	yard->stack[yard->slen++] = op;
}

/* returns and removes the operator at the top of stack */
char yard_pop(struct yard *yard) {
	if(!yard->slen) {
		mathfail = true;
		report_error("Shunting yard stack underflow");
		return '?';
	}
	return yard->stack[--yard->slen];
}

/* returns the operator at the top of stack */
char yard_peek(struct yard *yard) {
	if(!yard->slen) {
		return '?';
	}
	return yard->stack[yard->slen-1];
}

void yard_add_num(struct yard *yard, calc_float_t x) {
	struct yard_value value = {
		.isnum = 1,
		.content = {.num = x},
	};
	yard_put(yard, value);
}
/* https://en.wikipedia.org/wiki/Shunting-yard_algorithm */

void yard_add_op(struct yard *yard, char op) {

#define STACK_NOT_EMPTY (yard->slen)

#define TOP_HAS_GREATER_PRECEDENCE (prec(yard_peek(yard))>prec(op))

#define EQUAL_BUT_TOP_IS_LEFT_ASSOC \
	(prec(yard_peek(yard))==prec(op) && leftassoc(yard_peek(yard)))

#define TOP_IS_NOT_LEFT_PAREN (yard_peek(yard) != '(')

	while(
		STACK_NOT_EMPTY
		&& (TOP_HAS_GREATER_PRECEDENCE || EQUAL_BUT_TOP_IS_LEFT_ASSOC)
		&& TOP_IS_NOT_LEFT_PAREN
	) {
		// pop operators from the operator stack onto the output queue
		struct yard_value value = {
			.isnum = 0,
			.content = {.op = yard_pop(yard)},
		};
		yard_put(yard, value);
		if(mathfail)
			return;
	}
	yard_push(yard, op);

#undef STACK_NOT_EMPTY
#undef TOP_HAS_GREATER_PRECEDENCE
#undef EQUAL_BUT_TOP_IS_LEFT_ASSOC
#undef TOP_IS_NOT_LEFT_PAREN
}

void operand_push(struct operand_stack *stack, calc_float_t x) {
	if(stack->len >= OPERAND_STACK_SIZE) {
		mathfail = true;
		report_error("Operand stack overflow");
		return;
	}
	stack->stack[stack->len++] = x;
}

calc_float_t operand_pop(struct operand_stack *stack) {
	if(!stack->len) {
		report_error("Operand stack underflow");
		return NAN;
	}
	return stack->stack[--stack->len];
}

calc_float_t yard_compute(struct yard *yard) {
	/* pop remaining operators into output queue */
	while(yard->slen) {
		struct yard_value value = {
			.isnum = 0,
			.content = {.op = yard_pop(yard)},
		};
		yard_put(yard, value);
		if(mathfail)
			return NAN;
	}
	struct operand_stack stack = {0};
	for(unsigned i = 0; i < yard->qlen; i++) {
		struct yard_value v = yard->queue[i];
		if(v.isnum) {
			operand_push(&stack, v.content.num);
		} else {
			calc_float_t b = operand_pop(&stack);
			calc_float_t a = operand_pop(&stack);
			calc_float_t result = op_eval(v.content.op, a, b);
			operand_push(&stack, result);
		}
		if(mathfail)
			return NAN;
	}
	return operand_pop(&stack);
}

const char *namestack[NAME_STACK_SIZE];
unsigned namestack_len = 0;

void namestack_push(const char *name) {
	mathfail = true;
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

calc_float_t calc(const char *expr) {
	mathfail = false;
	struct yard yard = {0};
	expr += scan_whitespace(expr);
	// while there are tokens to be read
	while(expr[0]) {
		// if the token is a number
		if(isdigit(expr[0])) {
			char *numend;
			calc_float_t num = strtold(expr, &numend);
			expr = numend;
			// push it to the output queue
			yard_add_num(&yard, num);
		} else if(expr[0] == '.' || expr[0] == '_' || isalpha(expr[0])) {
			// if the token is a variable, calculate recursively
			const char *name;
			expr += scan_name(expr, &name);
			struct label label;
			calc_float_t result;
			if(!lookup_label(name, &label)) {
				mathfail = true;
				report_error("Unknown identifier: \"%s\"", name);
				result = NAN;
			} else if(label.expr) {
				if(namestack_contains(name)) {
					mathfail = true;
					report_error("Recursive label: \"%s\"", name);
					free((char*) name);
					return NAN;
				}
				namestack_push(name);
				result = calc(label.expr);
				namestack_pop();
			} else {
				result = label.constant;
			}
			free((char*) name);
			// push it to the output queue
			yard_add_num(&yard, result);
		} else if(expr[0] == ')') {
			// while top operator is not a left parenthesis
			while(yard_peek(&yard) != '(') {
				// ...pop operator from stack to output queue
				struct yard_value value = {
					.isnum = 0,
					.content = {.op = yard_pop(&yard)},
				};
				yard_put(&yard, value);
				if(mathfail)
					return NAN;
			}
			// if top operand is a left parenthesis
			if(yard_peek(&yard) == '(') {
				yard_pop(&yard); // discard it
			}
			expr++;
			// end of right parenthesis handling
		} else if(expr[0] == '(') {
			// if the token is a left parenthesis,
			//push it onto the operator stack.
			yard_add_op(&yard, '(');
			expr++;
		} else {
			// if the token is an operator,
			// push it onto the operator stack.
			char op = expr[0];
			expr++;
			yard_add_op(&yard, op);
		}
		expr += scan_whitespace(expr);
		if(mathfail)
			return NAN;
	}
	return yard_compute(&yard);
}

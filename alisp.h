#ifndef ALISP_H
#define ALISP_H

#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define the different types of lisp values
typedef enum
{
	ALISP_NULL,
	ALISP_NUMBER,
	ALISP_SYMBOL,
	ALISP_STRING,
	ALISP_PRIMITIVE,
	ALISP_LAMBDA,
	ALISP_PAIR
} alisp_type_t;

// Forward declaration
typedef struct alisp_value alisp_value_t;

// Function pointer for primitive functions
typedef alisp_value_t *(*alisp_primitive_func_t)(alisp_value_t *args);

// Lambda function structure
typedef struct
{
	alisp_value_t *parameters; // List of parameter symbols
	alisp_value_t *body;	   // Expression to evaluate
	alisp_value_t *env;		// Environment where lambda was defined
} alisp_lambda_t;

// Main value structure
typedef struct alisp_value
{
	alisp_type_t type;
	union {
		double number;					// For ALISP_NUMBER
		char *symbol;					 // For ALISP_SYMBOL
		char *string;					 // For ALISP_STRING
		alisp_primitive_func_t primitive; // For ALISP_PRIMITIVE
		alisp_lambda_t lambda;			// For ALISP_LAMBDA
		struct
		{ // For ALISP_PAIR
			alisp_value_t *car;
			alisp_value_t *cdr;
		} pair;
	} value;
} alisp_value_t;

// Environment structure (list of bindings)
typedef alisp_value_t *alisp_env_t;

// Error handling
#define ALISP_ERROR(msg) alisp_make_error(msg)

// Macro to iterate over a list of alisp values
#define alisp_for_each(var, link, args)																				\
	for (link = args; (link) && (link)->type == ALISP_PAIR; (link) = (link)->value.pair.cdr)						   \
		if ((var = (link)->value.pair.car), 1)

// Core functions
alisp_value_t *alisp_make_null();
alisp_value_t *alisp_make_number(double num);
alisp_value_t *alisp_make_symbol(const char *sym);
alisp_value_t *alisp_make_string(const char *str);
alisp_value_t *alisp_make_primitive(alisp_primitive_func_t func);
alisp_value_t *alisp_make_lambda(alisp_value_t *params, alisp_value_t *body, alisp_env_t env);
alisp_value_t *alisp_make_pair(alisp_value_t *car, alisp_value_t *cdr);
alisp_value_t *alisp_make_error(const char *msg);

// Utility functions
int alisp_eq(alisp_value_t *a, alisp_value_t *b);
int alisp_equal(alisp_value_t *a, alisp_value_t *b);

// Environment functions
alisp_env_t alisp_make_env(alisp_env_t outer);
alisp_value_t *alisp_env_get(alisp_env_t env, alisp_value_t *key);
void alisp_env_set(alisp_env_t env, alisp_value_t *key, alisp_value_t *val);

// Parsing functions
alisp_value_t *alisp_read(const char *input);
alisp_value_t *alisp_parse(const char *input, int *pos);

// Error handling
extern jmp_buf error_jmp;
extern char error_msg[256];

// Evaluation functions
alisp_value_t *alisp_eval(alisp_value_t *expr, alisp_env_t env);

// Printing functions
void alisp_print(alisp_value_t *val, char *buffer, size_t size);
void alisp_println(alisp_value_t *val);

// Memory management
void alisp_gc(alisp_value_t *val);
void alisp_free(alisp_value_t *val);

// Default environment
alisp_env_t alisp_make_default_env();

// Destroy environment
void alisp_destroy(alisp_env_t env);

// Error checking
int alisp_is_error(alisp_value_t *val);

#endif // ALISP_H

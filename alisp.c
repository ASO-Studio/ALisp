#include "alisp.h"
#include "alisp_utils.h"
#include <setjmp.h>
#include <stdarg.h>

// Global error handling
jmp_buf error_jmp;
char error_msg[256];

// Make a null value
alisp_value_t *alisp_make_null()
{
	alisp_value_t *val = malloc(sizeof(alisp_value_t));
	if (!val)
		return NULL;
	val->type = ALISP_NULL;
	return val;
}

// Make a number value
alisp_value_t *alisp_make_number(double num)
{
	alisp_value_t *val = malloc(sizeof(alisp_value_t));
	if (!val)
		return NULL;
	val->type = ALISP_NUMBER;
	val->value.number = num;
	return val;
}

// Make a symbol value
alisp_value_t *alisp_make_symbol(const char *sym)
{
	alisp_value_t *val = malloc(sizeof(alisp_value_t));
	if (!val)
		return NULL;
	val->type = ALISP_SYMBOL;
	val->value.symbol = malloc(strlen(sym) + 1);
	if (!val->value.symbol)
	{
		free(val);
		return NULL;
	}
	strcpy(val->value.symbol, sym);
	return val;
}

// Make a string value
alisp_value_t *alisp_make_string(const char *str)
{
	alisp_value_t *val = malloc(sizeof(alisp_value_t));
	if (!val)
		return NULL;
	val->type = ALISP_STRING;
	val->value.string = malloc(strlen(str) + 1);
	if (!val->value.string)
	{
		free(val);
		return NULL;
	}
	strcpy(val->value.string, str);
	return val;
}

// Make a primitive function value
alisp_value_t *alisp_make_primitive(alisp_primitive_func_t func)
{
	alisp_value_t *val = malloc(sizeof(alisp_value_t));
	if (!val)
		return NULL;
	val->type = ALISP_PRIMITIVE;
	val->value.primitive = func;
	return val;
}

// Make a lambda value
alisp_value_t *alisp_make_lambda(alisp_value_t *params, alisp_value_t *body, alisp_env_t env)
{
	alisp_value_t *val = malloc(sizeof(alisp_value_t));
	if (!val)
		return NULL;
	val->type = ALISP_LAMBDA;
	val->value.lambda.parameters = params;
	val->value.lambda.body = body;
	val->value.lambda.env = env; // Copy environment
	return val;
}

// Make a pair (cons cell)
alisp_value_t *alisp_make_pair(alisp_value_t *car, alisp_value_t *cdr)
{
	alisp_value_t *val = malloc(sizeof(alisp_value_t));
	if (!val)
		return NULL;
	val->type = ALISP_PAIR;
	val->value.pair.car = car;
	val->value.pair.cdr = cdr;
	return val;
}

// Make an error value (represented as a string)
alisp_value_t *alisp_make_error(const char *msg)
{
	if (msg)
	{
		strncpy(error_msg, msg, sizeof(error_msg) - 1);
		error_msg[sizeof(error_msg) - 1] = '\0';
	}
	longjmp(error_jmp, 1); // Use longjmp for error handling
	return NULL;
}

// Check if two values are the same object (pointer equality)
int alisp_eq(alisp_value_t *a, alisp_value_t *b)
{
	if (a->type != b->type)
		return 0;

	switch (a->type)
	{
	case ALISP_NULL:
		return 1;
	case ALISP_NUMBER:
		return a->value.number == b->value.number;
	case ALISP_SYMBOL:
		return strcmp(a->value.symbol, b->value.symbol) == 0;
	case ALISP_STRING:
		return strcmp(a->value.string, b->value.string) == 0;
	case ALISP_PRIMITIVE:
		return a->value.primitive == b->value.primitive;
	case ALISP_LAMBDA:
		return a->value.lambda.body == b->value.lambda.body &&
			   alisp_equal(a->value.lambda.parameters, b->value.lambda.parameters);
	case ALISP_PAIR:
		return a->value.pair.car == b->value.pair.car && a->value.pair.cdr == b->value.pair.cdr;
	}
	return 0;
}

// Check if two values are structurally equal
int alisp_equal(alisp_value_t *a, alisp_value_t *b)
{
	if (a->type != b->type)
		return 0;

	switch (a->type)
	{
	case ALISP_NULL:
		return 1;
	case ALISP_NUMBER:
		return a->value.number == b->value.number;
	case ALISP_SYMBOL:
		return strcmp(a->value.symbol, b->value.symbol) == 0;
	case ALISP_STRING:
		return strcmp(a->value.string, b->value.string) == 0;
	case ALISP_PAIR:
		return alisp_equal(a->value.pair.car, b->value.pair.car) && alisp_equal(a->value.pair.cdr, b->value.pair.cdr);
	case ALISP_LAMBDA:
		return a->value.lambda.body == b->value.lambda.body &&
			   alisp_equal(a->value.lambda.parameters, b->value.lambda.parameters);
	default:
		return alisp_eq(a, b);
	}
	return 0;
}

// Create a new environment
alisp_env_t alisp_make_env(alisp_env_t outer)
{
	alisp_value_t *env = alisp_make_pair(alisp_make_null(), outer ? outer : alisp_make_null());
	if (!env)
		return NULL;
	return env;
}

// Get a value from environment
alisp_value_t *alisp_env_get(alisp_env_t env, alisp_value_t *key)
{
	if (!env || env->type != ALISP_PAIR)
		return NULL;

	alisp_value_t *bindings = env->value.pair.car;
	while (bindings && bindings->type == ALISP_PAIR)
	{
		alisp_value_t *binding = bindings->value.pair.car;
		if (binding->type == ALISP_PAIR && alisp_equal(binding->value.pair.car, key))
		{
			return binding->value.pair.cdr;
		}
		bindings = bindings->value.pair.cdr;
	}

	// If not found in current env, check outer env
	alisp_value_t *outer = env->value.pair.cdr;
	if (outer && outer->type == ALISP_PAIR)
	{
		return alisp_env_get(outer, key);
	}

	return NULL;
}

// Deep copy function to copy an alisp value and all its sub-values
alisp_value_t *alisp_copy(alisp_value_t *val)
{
	if (!val)
		return NULL;

	alisp_value_t *copy = malloc(sizeof(alisp_value_t));
	if (!copy)
		return NULL;

	copy->type = val->type;

	switch (val->type)
	{
	case ALISP_NULL:
		// Nothing to copy
		break;
	case ALISP_NUMBER:
		copy->value.number = val->value.number;
		break;
	case ALISP_SYMBOL:
		copy->value.symbol = malloc(strlen(val->value.symbol) + 1);
		if (!copy->value.symbol)
		{
			free(copy);
			return NULL;
		}
		strcpy(copy->value.symbol, val->value.symbol);
		break;
	case ALISP_STRING:
		copy->value.string = malloc(strlen(val->value.string) + 1);
		if (!copy->value.string)
		{
			free(copy);
			return NULL;
		}
		strcpy(copy->value.string, val->value.string);
		break;
	case ALISP_PRIMITIVE:
		copy->value.primitive = val->value.primitive;
		break;
	case ALISP_LAMBDA:
		copy->value.lambda.parameters = alisp_copy(val->value.lambda.parameters);
		copy->value.lambda.body = alisp_copy(val->value.lambda.body);
		copy->value.lambda.env = val->value.lambda.env; // Environment is referenced, not copied
		break;
	case ALISP_PAIR:
		copy->value.pair.car = alisp_copy(val->value.pair.car);
		copy->value.pair.cdr = alisp_copy(val->value.pair.cdr);
		break;
	}

	return copy;
}

// Set a value in environment - deep copy both key and value to prevent memory issues
void alisp_env_set(alisp_env_t env, alisp_value_t *key, alisp_value_t *val)
{
	if (!env || env->type != ALISP_PAIR)
		return;

	alisp_value_t *key_copy = alisp_copy(key);
	alisp_value_t *val_copy = alisp_copy(val);

	alisp_value_t *binding = alisp_make_pair(key_copy, val_copy);
	alisp_value_t *new_bindings = alisp_make_pair(binding, env->value.pair.car);
	env->value.pair.car = new_bindings;
}

// Helper to skip whitespace
static void skip_whitespace(const char *input, int *pos)
{
	while (input[*pos] && isspace(input[*pos]))
	{
		(*pos)++;
	}
}

// Parse an atom (number, symbol, or string)
static alisp_value_t *parse_atom(const char *input, int *pos)
{
	skip_whitespace(input, pos);

	if (input[*pos] == '\0')
		return alisp_make_null();

	// Handle string literals
	if (input[*pos] == '"')
	{
		(*pos)++; // Skip opening quote
		int start = *pos;
		while (input[*pos] && input[*pos] != '"')
		{
			if (input[*pos] == '\\')
				(*pos)++; // Skip escaped chars
			(*pos)++;
		}
		if (input[*pos] != '"')
			return ALISP_ERROR("Unterminated string");

		char *str = malloc(*pos - start + 1);
		if (!str)
			return alisp_make_error("Memory allocation failed");

		strncpy(str, input + start, *pos - start);
		str[*pos - start] = '\0';
		(*pos)++; // Skip closing quote
		return alisp_make_string(str);
	}

	// Handle numbers (integers and floats)
	if (isdigit(input[*pos]) || (input[*pos] == '-' && input[*pos + 1] != '\0' && isdigit(input[*pos + 1])))
	{
		int start = *pos;
		if (input[*pos] == '-')
			(*pos)++;
		while (isdigit(input[*pos]) || input[*pos] == '.')
		{
			(*pos)++;
		}

		char *num_str = malloc(*pos - start + 1);
		if (!num_str)
			return alisp_make_error("Memory allocation failed");

		strncpy(num_str, input + start, *pos - start);
		num_str[*pos - start] = '\0';

		double num = atof(num_str);
		free(num_str);
		return alisp_make_number(num);
	}

	// Handle symbols
	int start = *pos;
	while (input[*pos] && !isspace(input[*pos]) && input[*pos] != '(' && input[*pos] != ')' && input[*pos] != '"')
	{
		(*pos)++;
	}

	if (*pos == start)
		return ALISP_ERROR("Unexpected character");

	char *sym = malloc(*pos - start + 1);
	if (!sym)
		return alisp_make_error("Memory allocation failed");

	strncpy(sym, input + start, *pos - start);
	sym[*pos - start] = '\0';
	return alisp_make_symbol(sym);
}

// Parse a list (s-expression in parentheses)
static alisp_value_t *parse_list(const char *input, int *pos)
{
	if (input[*pos] != '(')
		return ALISP_ERROR("Expected '('");
	(*pos)++; // Skip '('

	alisp_value_t *result = alisp_make_null();
	alisp_value_t *current = NULL;

	skip_whitespace(input, pos);

	while (input[*pos] && input[*pos] != ')')
	{
		alisp_value_t *expr = alisp_parse(input, pos);
		if (!expr)
			return NULL; // Error already handled

		if (result->type == ALISP_NULL)
		{
			result = alisp_make_pair(expr, alisp_make_null());
			current = result;
		}
		else
		{
			current->value.pair.cdr = alisp_make_pair(expr, alisp_make_null());
			current = current->value.pair.cdr;
		}

		skip_whitespace(input, pos);
	}

	if (input[*pos] != ')')
	{
		if (input[*pos] == '\0')
		{
			return ALISP_ERROR("Unexpected end of input, expected ')'");
		}
		else
		{
			return ALISP_ERROR("Expected ')'");
		}
	}
	(*pos)++; // Skip ')'

	return result;
}

// Main parsing function
alisp_value_t *alisp_parse(const char *input, int *pos)
{
	skip_whitespace(input, pos);

	if (input[*pos] == '\0')
		return alisp_make_null();
	if (input[*pos] == ')')
		return ALISP_ERROR("Unexpected ')'");

	if (input[*pos] == '(')
	{
		return parse_list(input, pos);
	}
	else
	{
		return parse_atom(input, pos);
	}
}

// Public read function
alisp_value_t *alisp_read(const char *input)
{
	int pos = 0;
	return alisp_parse(input, &pos);
}

// Primitive function implementations
static alisp_value_t *prim_add(alisp_env_t env, alisp_value_t *args)
{
	(void)env;
	double result = 0.0;
	alisp_value_t *current = args;

	while (current && current->type == ALISP_PAIR)
	{
		alisp_value_t *val = current->value.pair.car;
		if (val->type != ALISP_NUMBER)
		{
			return ALISP_ERROR("Arguments to + must be numbers");
		}
		result += val->value.number;
		current = current->value.pair.cdr;
	}

	return alisp_make_number(result);
}

static alisp_value_t *prim_sub(alisp_env_t env, alisp_value_t *args)
{
	(void)env;
	if (args->type != ALISP_PAIR)
		return ALISP_ERROR("Wrong number of arguments to -");

	double result = args->value.pair.car->value.number;
	alisp_value_t *current = args->value.pair.cdr;

	if (current->type == ALISP_NULL)
	{
		// Unary minus
		return alisp_make_number(-result);
	}

	while (current && current->type == ALISP_PAIR)
	{
		alisp_value_t *val = current->value.pair.car;
		if (val->type != ALISP_NUMBER)
		{
			return ALISP_ERROR("Arguments to - must be numbers");
		}
		result -= val->value.number;
		current = current->value.pair.cdr;
	}

	return alisp_make_number(result);
}

static alisp_value_t *prim_mul(alisp_env_t env, alisp_value_t *args)
{
	(void)env;
	double result = 1.0;
	alisp_value_t *current = args;

	while (current && current->type == ALISP_PAIR)
	{
		alisp_value_t *val = current->value.pair.car;
		if (val->type != ALISP_NUMBER)
		{
			return ALISP_ERROR("Arguments to * must be numbers");
		}
		result *= val->value.number;
		current = current->value.pair.cdr;
	}

	return alisp_make_number(result);
}

static alisp_value_t *prim_div(alisp_env_t env, alisp_value_t *args)
{
	(void)env;
	if (args->type != ALISP_PAIR)
		return ALISP_ERROR("Wrong number of arguments to /");

	double result = args->value.pair.car->value.number;
	alisp_value_t *current = args->value.pair.cdr;

	if (current->type == ALISP_NULL)
	{
		// Unary division (1/x)
		if (result == 0)
			return ALISP_ERROR("Division by zero");
		return alisp_make_number(1.0 / result);
	}

	while (current && current->type == ALISP_PAIR)
	{
		alisp_value_t *val = current->value.pair.car;
		if (val->type != ALISP_NUMBER)
		{
			return ALISP_ERROR("Arguments to / must be numbers");
		}
		if (val->value.number == 0)
			return ALISP_ERROR("Division by zero");
		result /= val->value.number;
		current = current->value.pair.cdr;
	}

	return alisp_make_number(result);
}

static alisp_value_t *prim_eq(alisp_env_t env, alisp_value_t *args)
{
	(void)env;
	if (args->type != ALISP_PAIR || args->value.pair.cdr->type != ALISP_PAIR ||
		args->value.pair.cdr->value.pair.cdr->type != ALISP_NULL)
	{
		return ALISP_ERROR("Wrong number of arguments to =");
	}

	alisp_value_t *a = args->value.pair.car;
	alisp_value_t *b = args->value.pair.cdr->value.pair.car;

	return alisp_make_number(alisp_eq(a, b) ? 1 : 0);
}

static alisp_value_t *prim_less_than(alisp_env_t env, alisp_value_t *args)
{
	(void)env;
	if (args->type != ALISP_PAIR || args->value.pair.cdr->type != ALISP_PAIR ||
		args->value.pair.cdr->value.pair.cdr->type != ALISP_NULL)
	{
		return ALISP_ERROR("Wrong number of arguments to <");
	}

	alisp_value_t *a = args->value.pair.car;
	alisp_value_t *b = args->value.pair.cdr->value.pair.car;

	if (a->type != ALISP_NUMBER || b->type != ALISP_NUMBER)
	{
		return ALISP_ERROR("Arguments to < must be numbers");
	}

	return alisp_make_number(a->value.number < b->value.number ? 1 : 0);
}

static alisp_value_t *prim_greater_than(alisp_env_t env, alisp_value_t *args)
{
	(void)env;
	if (args->type != ALISP_PAIR || args->value.pair.cdr->type != ALISP_PAIR ||
		args->value.pair.cdr->value.pair.cdr->type != ALISP_NULL)
	{
		return ALISP_ERROR("Wrong number of arguments to >");
	}

	alisp_value_t *a = args->value.pair.car;
	alisp_value_t *b = args->value.pair.cdr->value.pair.car;

	if (a->type != ALISP_NUMBER || b->type != ALISP_NUMBER)
	{
		return ALISP_ERROR("Arguments to > must be numbers");
	}

	return alisp_make_number(a->value.number > b->value.number ? 1 : 0);
}

// Main evaluation function
alisp_value_t *alisp_eval(alisp_value_t *expr, alisp_env_t env)
{
	if (!expr)
		return alisp_make_null();

	switch (expr->type)
	{
	case ALISP_NUMBER:
	case ALISP_STRING:
		return expr; // Literals evaluate to themselves

	case ALISP_SYMBOL: {
		alisp_value_t *val = alisp_env_get(env, expr);
		if (!val)
		{
			sprintf(error_msg, "Symbol '%s' not found", expr->value.symbol);
			return ALISP_ERROR(error_msg);
		}
		return val;
	}

	case ALISP_PAIR: {
		// This is a function call
		alisp_value_t *op = expr->value.pair.car;

		// Special forms
		if (op->type == ALISP_SYMBOL)
		{
			if (strcmp(op->value.symbol, "quote") == 0)
			{
				return expr->value.pair.cdr->value.pair.car;
			}

			if (strcmp(op->value.symbol, "if") == 0)

			{

				// Parse: (if condition body...)
				// where body contains expressions and optional elseif/else keywords
				alisp_value_t *args = expr->value.pair.cdr;

				if (!args || args->type != ALISP_PAIR)
				{

					return alisp_make_null(); // Malformed if expression
				}

				// Get condition
				alisp_value_t *condition = args->value.pair.car;
				args = args->value.pair.cdr;
				if (!args || args->type != ALISP_PAIR)
				{

					return alisp_make_null(); // Missing body
				}

				// Evaluate main condition
				alisp_value_t *evaluated_condition = alisp_eval(condition, env);
				int is_false = (evaluated_condition->type == ALISP_NULL) ||
							   (evaluated_condition->type == ALISP_NUMBER && evaluated_condition->value.number == 0.0);

				if (!is_false)
				{

					// Main condition is true, execute the then branch (which may contain multiple expressions)
					// Process expressions until we hit elseif or else keyword
					alisp_value_t *result = alisp_make_null();

					while (args && args->type == ALISP_PAIR)
					{

						alisp_value_t *current_item = args->value.pair.car;
						alisp_value_t *next_item = args->value.pair.cdr;

						// Check if this item is a keyword (elseif/else)
						if (current_item && current_item->type == ALISP_SYMBOL)
						{

							if (strcmp(current_item->value.symbol, "elseif") == 0 ||
								strcmp(current_item->value.symbol, "else") == 0)
							{
								// This is a keyword, so we're done with the then branch
								break;
							}
						}

						// It's an expression in the then branch
						result = alisp_eval(current_item, env);
						args = next_item;
					}

					return result;
				}
				else
				{

					// Main condition is false, need to find elseif or else branch
					// Skip expressions in the main then branch until we find a keyword
					while (args && args->type == ALISP_PAIR)
					{
						alisp_value_t *current_item = args->value.pair.car;
						alisp_value_t *next_item = args->value.pair.cdr;

						if (current_item && current_item->type == ALISP_SYMBOL)
						{
							// Found a keyword, check what type it is
							if (strcmp(current_item->value.symbol, "elseif") == 0)
							{
								// Process elseif: next item should be the elseif condition
								args = next_item; // Move to condition

								if (!args || args->type != ALISP_PAIR)
								{
									return alisp_make_null(); // Missing elseif condition
								}

								alisp_value_t *elseif_condition = args->value.pair.car;
								args = args->value.pair.cdr; // Move to potential then expressions

								// Evaluate elseif condition
								alisp_value_t *evaluated_elseif_condition = alisp_eval(elseif_condition, env);
								int elseif_is_false = (evaluated_elseif_condition->type == ALISP_NULL) ||
													  (evaluated_elseif_condition->type == ALISP_NUMBER &&
													   evaluated_elseif_condition->value.number == 0.0);

								if (!elseif_is_false)
								{
									// ElseIf condition is true, execute its expressions until next keyword
									alisp_value_t *result = alisp_make_null();
									while (args && args->type == ALISP_PAIR)
									{
										alisp_value_t *current_item_elseif = args->value.pair.car;
										alisp_value_t *next_item_elseif = args->value.pair.cdr;
										// Check if this is a keyword
										if (current_item_elseif && current_item_elseif->type == ALISP_SYMBOL)
										{
											if (strcmp(current_item_elseif->value.symbol, "elseif") == 0 ||
												strcmp(current_item_elseif->value.symbol, "else") == 0)
											{
												// This is another keyword, so we're done with this elseif branch
												break;
											}
										}

										// It's an expression in the elseif branch
										result = alisp_eval(current_item_elseif, env);
										args = next_item_elseif;
									}

									return result;
								}
								else
								{
									// ElseIf condition is false, skip all expressions in this elseif branch until next
									// keyword
									while (args && args->type == ALISP_PAIR)
									{
										alisp_value_t *current_item_elseif = args->value.pair.car;
										alisp_value_t *next_item_elseif = args->value.pair.cdr;

										if (current_item_elseif && current_item_elseif->type == ALISP_SYMBOL)
										{
											if (strcmp(current_item_elseif->value.symbol, "elseif") == 0 ||
												strcmp(current_item_elseif->value.symbol, "else") == 0)
											{
												// Found next keyword, go back to main loop to process it
												args = alisp_make_pair(current_item_elseif,
																	   next_item_elseif); // Put keyword back
												break; // Break inner while loop, continue outer loop
											}
										}

										// Continue skipping expressions in elseif branch
										args = next_item_elseif;
									}

									continue; // Continue main while loop
								}
							}
							else if (strcmp(current_item->value.symbol, "else") == 0)
							{
								// Process else branch: execute all remaining expressions
								args = next_item; // Move to expressions after 'else' keyword
								alisp_value_t *result = alisp_make_null();

								while (args && args->type == ALISP_PAIR)
								{
									alisp_value_t *current_item_else = args->value.pair.car;
									alisp_value_t *next_item_else = args->value.pair.cdr;

									// Execute this expression (no more keywords expected in else branch)
									result = alisp_eval(current_item_else, env);
									args = next_item_else;
								}

								return result;
							}
							else
							{
								// Unexpected keyword

								return alisp_make_null();
							}
						}
						else
						{
							// Expression in main then branch, skip it

							args = next_item;
						}
					}
				}

				// No condition was true and no else clause

				return alisp_make_null();
			}

			if (strcmp(op->value.symbol, "function") == 0)
			{
				// Parse (function name (params...) body)
				alisp_value_t *name = expr->value.pair.cdr->value.pair.car;
				alisp_value_t *params_and_body = expr->value.pair.cdr->value.pair.cdr;
				alisp_value_t *params = params_and_body->value.pair.car;
				alisp_value_t *body = params_and_body->value.pair.cdr->value.pair.car;

				// Create lambda function
				alisp_value_t *lambda = alisp_make_lambda(params, body, env);

				// Store in environment with the given name
				alisp_env_set(env, name, lambda);
				// Return null to prevent output of <lambda>
				return alisp_make_null();
			}
			if (strcmp(op->value.symbol, "define") == 0)
			{
				// Parse (assign variable value)
				alisp_value_t *var = expr->value.pair.cdr->value.pair.car;
				alisp_value_t *value_expr = expr->value.pair.cdr->value.pair.cdr->value.pair.car;
				alisp_value_t *value = alisp_eval(value_expr, env);
				alisp_env_set(env, var, value);
				return value;
			}
			if (strcmp(op->value.symbol, "return") == 0)
			{
				// Simply return the evaluated expression
				alisp_value_t *value_expr = expr->value.pair.cdr->value.pair.car;
				return alisp_eval(value_expr, env);
			}

			if (strcmp(op->value.symbol, "lambda") == 0)
			{
				alisp_value_t *params = expr->value.pair.cdr->value.pair.car;
				alisp_value_t *body = expr->value.pair.cdr->value.pair.cdr->value.pair.car;
				return alisp_make_lambda(params, body, env);
			}
		}

		// Regular function call
		alisp_value_t *func = alisp_eval(op, env);
		if (!func)
		{
			return ALISP_ERROR("Unknown function");
		}

		if (func->type != ALISP_PRIMITIVE && func->type != ALISP_LAMBDA)
		{
			return ALISP_ERROR("Attempt to call non-function");
		}

		// Evaluate arguments
		alisp_value_t *args = alisp_make_null();
		alisp_value_t *arg_exprs = expr->value.pair.cdr;
		alisp_value_t *arg_current = NULL;

		while (arg_exprs && arg_exprs->type == ALISP_PAIR)
		{
			alisp_value_t *arg_val = alisp_eval(arg_exprs->value.pair.car, env);
			if (!arg_val)
				return ALISP_ERROR("Error evaluating argument");

			if (args->type == ALISP_NULL)
			{
				args = alisp_make_pair(arg_val, alisp_make_null());
				arg_current = args;
			}
			else
			{
				arg_current->value.pair.cdr = alisp_make_pair(arg_val, alisp_make_null());
				arg_current = arg_current->value.pair.cdr;
			}

			arg_exprs = arg_exprs->value.pair.cdr;
		}

		if (func->type == ALISP_PRIMITIVE)
		{
			return func->value.primitive(env, args);
		}
		else if (func->type == ALISP_LAMBDA)
		{
			// Create new environment for lambda call
			alisp_env_t lambda_env = alisp_make_env(func->value.lambda.env);

			// Bind parameters to arguments
			alisp_value_t *params = func->value.lambda.parameters;
			alisp_value_t *arg_vals = args;

			while (params && params->type == ALISP_PAIR && arg_vals && arg_vals->type == ALISP_PAIR)
			{
				alisp_env_set(lambda_env, params->value.pair.car, arg_vals->value.pair.car);
				params = params->value.pair.cdr;
				arg_vals = arg_vals->value.pair.cdr;
			}

			// Evaluate lambda body in new environment
			return alisp_eval(func->value.lambda.body, lambda_env);
		}
		__attribute__((fallthrough));
	}

	default:
		return ALISP_ERROR("Cannot evaluate this expression");
	}

	return alisp_make_null();
}

// Print functions
void alisp_print(alisp_value_t *val, char *buffer, size_t size)
{
	if (!val)
	{
		snprintf(buffer, size, "null");
		return;
	}

	switch (val->type)
	{
	case ALISP_NULL:
		snprintf(buffer, size, "()");
		break;
	case ALISP_NUMBER:
		snprintf(buffer, size, "%.2f", val->value.number);
		break;
	case ALISP_SYMBOL:
		snprintf(buffer, size, "%s", val->value.symbol);
		break;
	case ALISP_STRING:
		snprintf(buffer, size, "\"%s\"", val->value.string);
		break;
	case ALISP_PRIMITIVE:
		snprintf(buffer, size, "<primitive>");
		break;
	case ALISP_LAMBDA:
		snprintf(buffer, size, "<lambda>");
		break;
	case ALISP_PAIR: {
		strncat(buffer, "(", size - 1);
		alisp_value_t *current = val;
		int first = 1;
		while (current && current->type == ALISP_PAIR)
		{
			if (!first)
				strncat(buffer, " ", size - 1);
			char temp[256];
			alisp_print(current->value.pair.car, temp, sizeof(temp));
			strncat(buffer, temp, size - 1);

			current = current->value.pair.cdr;
			if (current && current->type != ALISP_NULL && current->type != ALISP_PAIR)
			{
				strncat(buffer, " . ", size - 1);
				alisp_print(current, temp, sizeof(temp));
				strncat(buffer, temp, size - 1);
				break;
			}
			first = 0;
		}
		strncat(buffer, ")", size - 1);
		break;
	}
	}
}

void alisp_println(alisp_value_t *val)
{
	char buffer[512];
	buffer[0] = '\0';
	alisp_print(val, buffer, sizeof(buffer));
	printf("%s\n", buffer);
}

// Memory management
void alisp_free(alisp_value_t *val)
{
	if (!val)
		return;

	switch (val->type)
	{
	case ALISP_SYMBOL:
		free(val->value.symbol);
		break;
	case ALISP_STRING:
		free(val->value.string);
		break;
	case ALISP_PAIR:
		alisp_free(val->value.pair.car);
		alisp_free(val->value.pair.cdr);
		break;
	case ALISP_LAMBDA:
		alisp_free(val->value.lambda.parameters);
		alisp_free(val->value.lambda.body);
		break;
	default:
		break;
	}

	free(val);
}

// Destroy environment and all contained values
void alisp_destroy(alisp_env_t env)
{
	if (!env || env->type != ALISP_PAIR)
		return;

	// Free all bindings in the environment
	alisp_value_t *bindings = env->value.pair.car;
	while (bindings && bindings->type == ALISP_PAIR)
	{
		alisp_value_t *binding = bindings->value.pair.car;
		if (binding && binding->type == ALISP_PAIR)
		{
			// Free the key-value pair in the binding
			// Be careful not to free primitive functions or other protected values
			alisp_free(binding);
		}
		bindings = bindings->value.pair.cdr;
	}

	// Free the environment pair itself
	// Note: Don't recursively destroy outer env if it's shared
	free(env);
}

// Initialize the default environment with built-in functions
alisp_env_t alisp_make_default_env()
{
	alisp_env_t env = alisp_make_env(NULL);

	// Add built-in primitive functions
	alisp_env_set(env, alisp_make_symbol("+"), alisp_make_primitive(prim_add));
	alisp_env_set(env, alisp_make_symbol("-"), alisp_make_primitive(prim_sub));
	alisp_env_set(env, alisp_make_symbol("*"), alisp_make_primitive(prim_mul));
	alisp_env_set(env, alisp_make_symbol("/"), alisp_make_primitive(prim_div));
	alisp_env_set(env, alisp_make_symbol("="), alisp_make_primitive(prim_eq));
	alisp_env_set(env, alisp_make_symbol("<"), alisp_make_primitive(prim_less_than));
	alisp_env_set(env, alisp_make_symbol(">"), alisp_make_primitive(prim_greater_than));

	return env;
}

// Error checking
int alisp_is_error(alisp_value_t *val)
{
	// Check if value is a string that represents an error message
	// We'll use a heuristic: if it starts with common error prefixes
	if (val && val->type == ALISP_STRING)
	{
		return (strncmp(val->value.string, "Unexpected", 10) == 0 || strncmp(val->value.string, "Expected", 8) == 0 ||
				strncmp(val->value.string, "Symbol", 6) == 0 || strncmp(val->value.string, "Unknown", 7) == 0 ||
				strncmp(val->value.string, "Attempt", 7) == 0 || strncmp(val->value.string, "Cannot", 6) == 0 ||
				strncmp(val->value.string, "Error", 5) == 0 || strncmp(val->value.string, "Unterminated", 12) == 0);
	}
	return 0;
}

// Execute Lisp code from a string (processes multiple expressions)
alisp_value_t *alisp_execute(const char* content, alisp_env_t env)
{
	// This function needs to be used carefully since the ALisp implementation
	// uses global state for error handling (setjmp/longjmp)
	// For this implementation, we'll use a single global error handling context
	// which means this function is not thread-safe

	// Handle both parsing and evaluation with a single error context
	int pos = 0;
	alisp_value_t *last_result = NULL;
	
	if (setjmp(error_jmp) != 0)
	{
		// An error occurred during either parsing or evaluation
		// The error is already stored in error_msg via longjmp
		// Create an error value to return
		// Create a new string with the error message
		alisp_value_t *error_result = alisp_make_string(error_msg);
		
		// Clean up environment
		alisp_destroy(env);
		
		return error_result;
	}
	
	// Loop through all expressions in the content
	while (1)
	{
		// Skip whitespace
		while (content[pos] && isspace(content[pos]))
		{
			pos++;
		}
		
		// If we've reached the end of content, break
		if (!content[pos])
		{
			break;
		}
		
		// Parse the next expression
		alisp_value_t *expr = alisp_parse(content, &pos);
		if (!expr)
		{
			// If parsing failed, exit the loop
			break;
		}

		// Evaluate the expression
		alisp_value_t *result = alisp_eval(expr, env);
		
		// Free the expression but keep the result
		alisp_free(expr);
		
		// If this is not a null result, store it as the last result
		if (result && result->type != ALISP_NULL)
		{
			// Free the previous last result if it exists
			if (last_result)
			{
				alisp_free(last_result);
			}
			// Copy the result to preserve it after environment is destroyed
			last_result = alisp_copy(result);
		}
		else
		{
			// Free the null result since we don't need to keep it
			if (result)
			{
				alisp_free(result);
			}
		}
	}
	
	// Clean up environment
	alisp_destroy(env);
	
	return last_result;
}

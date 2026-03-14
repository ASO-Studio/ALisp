#include "alisp.h"
#include "alisp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ALISP_REPL_BUFFER_SIZE 1024

int shell_main()
{
	printf("aLisp - A minimal Lisp interpreter in C\n");
	printf("Type 'exit' or 'quit' to quit.\n\n");

	// Create the default environment with built-in functions
	alisp_env_t env = alisp_make_default_env();
	alisp_utils_register_all(env);

	int done = 0;
	while (!done)
	{
		// Accumulate input until we have a complete expression
		char full_input[ALISP_REPL_BUFFER_SIZE * 10]; // Larger buffer for multi-line
		full_input[0] = '\0';
		int paren_count = 0;
		int in_string = 0;
		char quote_char = 0;
		int reached_eof = 0;

		while (1)
		{
			char temp_line[ALISP_REPL_BUFFER_SIZE];

			// Only print prompts in interactive mode (not when reading from file)
			if (strlen(full_input) > 0)
			{
				// If we already have partial input, show continuation prompt
				printf("... ");
			}
			else
			{
				printf("> ");
			}
			fflush(stdout);

			if (!fgets(temp_line, sizeof(temp_line), stdin))
			{
				reached_eof = 1;
				if (paren_count > 0 || in_string)
				{
					// We have partial input but reached EOF
					printf("\nError: Unexpected end of input\n");
				}
				break; // EOF or error
			}

			// Check for exit commands in interactive mode only
			if (strlen(full_input) == 0 && (strcmp(temp_line, "exit\n") == 0 || strcmp(temp_line, "quit\n") == 0))
			{
				done = 1; // Mark to exit outer loop
				break;
			}

			// Count parentheses and quotes to determine if expression is complete
			int i;
			for (i = 0; temp_line[i] != '\0'; i++)
			{
				if (!in_string)
				{
					if (temp_line[i] == '"' || temp_line[i] == '\'')
					{
						in_string = 1;
						quote_char = temp_line[i];
					}
					else if (temp_line[i] == '(')
					{
						paren_count++;
					}
					else if (temp_line[i] == ')')
					{
						paren_count--;
					}
				}
				else
				{
					if (temp_line[i] == quote_char && temp_line[i - 1] != '\\')
					{
						in_string = 0;
					}
				}
			}

			// Append to full input
			strcat(full_input, temp_line);

			// Check if we have a complete expression
			if (paren_count <= 0 && !in_string)
			{
				// Remove trailing newline from last line if it's the complete expression
				size_t len = strlen(full_input);
				if (len > 0 && full_input[len - 1] == '\n')
				{
					full_input[len - 1] = '\0';
				}
				break; // Complete expression
			}
		}

		// If we have a complete expression, process it
		if (strlen(full_input) > 0 && paren_count <= 0 && !in_string)
		{
			// Handle both parsing and evaluation with a single error context
			alisp_value_t *result = NULL;
			if (setjmp(error_jmp) == 0)
			{
				// Parse the input
				alisp_value_t *expr = alisp_read(full_input);
				if (!expr)
				{
					printf("Error: Could not parse expression\n");
					continue;
				}

				// Evaluate the expression
				result = alisp_eval(expr, env);
				alisp_free(expr);
			}
			else
			{
				// An error occurred during either parsing or evaluation
				printf("Error: %s\n", error_msg);
				continue;
			}

			if (!result)
			{
				printf("Error: Evaluation failed\n");
			}
			else
			{
				if (alisp_is_error(result))
				{
					// Print the error message from the result
					printf("Error: %s\n", result->value.string);
				}
				else
				{
					alisp_println(result);
				}
			}
		}
		else if (done || reached_eof)
		{
			// No more expressions to process
			break;
		}
	}
	// Clean up environment
	alisp_destroy(env);

	printf("Goodbye!\n");
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc <= 1)
	{ // No arguments gave
		return shell_main();
	}

	const char *file = argv[1];
	char *file_content = NULL;
	FILE *fp = fopen(file, "r");
	long file_size = 0;
	int ret = 0;
	alisp_value_t *result = NULL;
	alisp_env_t env;

	if (!fp)
	{
		perror("fopen");
		return 1;
	}

	// Seek to the end of the file then use ftell() to get the size of file
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);

	if ((file_content = malloc(file_size)) == NULL)
	{
		fclose(fp);
		return 1;
	}

	fread(file_content, 1, file_size, fp);
	fclose(fp);

	env = alisp_make_default_env();
	alisp_utils_register_all(env);

	result = alisp_execute(file_content, env);
	if (alisp_is_error(result))
	{
		fprintf(stderr, "Error: %s\n", result->value.string);
		ret = 1;
	}
	else if (result)
	{
		alisp_free(result);  // Free the copied result
	}
	alisp_destroy(env);
	free(file_content);

	return ret;
}

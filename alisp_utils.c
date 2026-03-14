#include "alisp_utils.h"

static alisp_value_t *__print(alisp_value_t *args)
{
	alisp_value_t *arg = NULL, *link = NULL;
	alisp_for_each(arg, link, args)
	{
		switch (arg->type)
		{
		case ALISP_NUMBER:
			printf("%lf", arg->value.number);
			break;
		case ALISP_STRING:
			printf("%s", arg->value.string);
			break;
		case ALISP_SYMBOL:
			printf("<symbol: %s>", arg->value.symbol);
			break;
		case ALISP_PRIMITIVE:
			printf("<primitive: %p>", arg->value.primitive);
			break;
		case ALISP_LAMBDA:
			printf("<lambda>");
			break;
		default:
			printf("<unknown>");
			break;
		}
	}

	printf("\n");
	alisp_value_t *ret = malloc(sizeof(alisp_value_t));
	ret->type = ALISP_NUMBER;
	ret->value.number = 1.0;

	return ret;
}

void alisp_utils_register_all(alisp_env_t env)
{
	alisp_env_set(env, alisp_make_symbol("print"), alisp_make_primitive(__print));
}

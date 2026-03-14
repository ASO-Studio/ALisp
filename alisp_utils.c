#include "alisp_utils.h"
#include "alisp_config.h"

#ifdef HAVE_DLFCN
# include <dlfcn.h>
#endif

// (print a b c...)
static alisp_value_t *__print(alisp_env_t env, alisp_value_t *args)
{
	(void)env;
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
	alisp_value_t *ret = alisp_make_number(1.0);
	return ret;
}

// (include_native "libname.so" func_name)
static alisp_value_t *__include_native(alisp_env_t env, alisp_value_t *args)
{
#ifndef HAVE_DLFCN
	alisp_value_t *ret = alisp_make_number(0.0);
	return ret;
#else
	const char *libname = NULL, *func_name = NULL;
	int count = 0;
	void *dlhandle = NULL;
	alisp_value_t *ret = alisp_make_number(1.0);
	alisp_value_t *arg, *link;

	alisp_for_each(arg, link, args)
	{
		if (count == 0)
		{
			if (arg->type != ALISP_STRING)
			{
				return alisp_make_error("Expected a string");
			}
			else
			{
				libname = arg->value.string;
			}
		}

		if (count == 1)
		{
			if (arg->type != ALISP_STRING)
			{
				return alisp_make_error("Expected a string or a symbol");
			}
			else
			{
				func_name = arg->value.string;
			}
		}

		count++;
		if (count >= 2)
			break;
	}

	dlhandle = dlopen(libname, RTLD_NOW | RTLD_GLOBAL | RTLD_LOCAL);
	if (!dlhandle)
	{
		return alisp_make_error(dlerror());
	}

	alisp_primitive_func_t func = dlsym(dlhandle, func_name);
	if (!func)
	{
		dlclose(dlhandle);
		return alisp_make_error(dlerror());
	}

	alisp_env_set(env, alisp_make_symbol(func_name), alisp_make_primitive(func));
#endif

	return ret;
}

void alisp_utils_register_all(alisp_env_t env)
{
	alisp_env_set(env, alisp_make_symbol("print"), alisp_make_primitive(__print));
	alisp_env_set(env, alisp_make_symbol("include_native"), alisp_make_primitive(__include_native));
}

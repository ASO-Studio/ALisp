#include "alisp.h"

alisp_value_t *test_func(alisp_env_t env, alisp_value_t *args)
{
	(void)env; (void)args;
	return alisp_make_number(114514.0);
}

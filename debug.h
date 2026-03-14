#ifndef _DEBUG_H
#define _DEBUG_H 1

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef DEBUG
// Common debug messages
#define LOG(...) do {\
			fprintf(stderr, "(\033[34;1mD\033[0m) at %s:%s:%d: \033[34m", __FILE__, __func__, __LINE__);\
			fprintf(stderr, __VA_ARGS__);\
			fprintf(stderr, "\033[0m");\
		} while(0)
// Not important messages
#define LOGN(...) do {\
			fprintf(stderr, "(N) at %s:%s:%d: \033[90m", __FILE__, __func__, __LINE__);\
			fprintf(stderr, __VA_ARGS__);\
			fprintf(stderr, "\033[0m");\
		} while(0)
// Error messages
#define LOGE(...) do {\
			fprintf(stderr, "(\033[31;1mE\033[0m) at %s:%s:%d: \033[31m", __FILE__, __func__, __LINE__);\
			fprintf(stderr, __VA_ARGS__);\
			fprintf(stderr, "\033[0m");\
		} while(0)

# define LOGEN(...) do {\
			LOGE(__VA_ARGS__);\
			fprintf(stderr, "\033[31m: %s\033[0m\n", strerror(errno));\
		} while(0)

#else
# define LOG(...)
# define LOGN(...)
# define LOGE(...) do {\
			fprintf(stderr, "Error: ");\
			fprintf(stderr, __VA_ARGS__);\
		} while(0)
# define LOGEN(...) do {\
			LOGE(__VA_ARGS__);\
			fprintf(stderr, ": %s\n", strerror(errno));\
		} while(0)
#endif

#endif

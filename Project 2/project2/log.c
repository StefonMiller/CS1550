#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"

/**
 * Print a scary error message to the console.
 */
static void verror(const char *format, va_list ap)
{
	if (isatty(STDERR_FILENO)) {
		fprintf(stderr, "\n\033[31;1m*** ERROR ***\033[0m\n");
	} else {
		fprintf(stderr, "\n*** ERROR ***\n");
	}

	vfprintf(stderr, format, ap);
	fputs("", stderr);
}

/**
 * Print an info message to the console.
 */
static void vinfo(const char *format, va_list ap)
{
	if (isatty(STDERR_FILENO)) {
		fprintf(stderr, "\033[32;1mINFO: \033[0m");
	} else {
		fprintf(stderr, "INFO: ");
	}

	vfprintf(stderr, format, ap);
}

int error(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	verror(format, args);
	va_end(args);

	exit(-1);

	return -1;
}

int info(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vinfo(format, args);
	va_end(args);

	return 0;
}

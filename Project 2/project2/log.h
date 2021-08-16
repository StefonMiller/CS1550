/**
 * Print a scary error message to the console and exit.
 */
__attribute__((format (printf, 1, 2)))
int error(const char *format, ...);

/**
 * Print an info message to the console.
 */
__attribute__((format (printf, 1, 2)))
int info(const char *format, ...);

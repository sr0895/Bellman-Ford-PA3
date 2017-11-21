#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdarg.h>

typedef enum {FALSE, TRUE} bool;

#define ERROR(err_msg) {perror(err_msg); exit(EXIT_FAILURE);}

/* https://scaryreasoner.wordpress.com/2009/02/28/checking-sizeof-at-compile-time/ */
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)])) // Interesting stuff to read if you are interested to know how this works

uint16_t CONTROL_PORT;

void lprint(const char* line, ...) {
	va_list args;

   	va_start(args, format);
   	FILE* file = fopen("log_pa3.txt", "a");
   	vfprintf(file, format, args);
   	fclose(file);
   	va_end(args);
}

#endif
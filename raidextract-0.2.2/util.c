#ifdef LINUXSMALL
#include "linuxsmall.h"
#else
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#endif
#include "util.h"

void die(const char *fmt, ...)
{
        va_list ap;
	static int exiting=0;
	if (exiting) exit(1);
	exiting=1;

        va_start(ap, fmt);
        fprintf(stderr, "%s: ", progname);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        exit(1);
}

void warn(const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        fprintf(stderr, "%s: ", progname);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
}

void setprogname(const char *newprogname)
{
        char *slash=strrchr(newprogname, '/');
        progname=slash? slash+1: newprogname;
}


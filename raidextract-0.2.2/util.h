#ifndef UTIL_H
#define UTIL_H

extern const char *progname;
extern void die(const char *fmt, ...);
extern void warn(const char *fmt, ...);
extern void setprogname(const char *newprogname);

#endif

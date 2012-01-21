#ifndef LIB_H
#define LIB_H

typedef long long off_t;
typedef unsigned long size_t;
typedef int uint32_t;

#include <stdarg.h>

#define BUFSIZ 4096

typedef struct
{
	int tail, fd, autoflush;
	char *buf;
} FILE;

extern FILE *stdout, *stderr;

extern void fprintf(FILE *file, const char *format, ...);
extern void vfprintf(FILE *stream, const char *format, va_list ap);
extern int atoi(const char *nptr);
extern long long atoll(const char *nptr);

extern int errno;

#define __syscall_return(type, res) \
do { \
	if ((unsigned long)(res) >= (unsigned long)(-125)) { \
		errno = -(res); \
		res = -1; \
	} \
	return (type) (res); \
} while (0)
#define _syscall0(type,name) \
type name(void) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name)); \
__syscall_return(type,__res); \
}
#define _syscall1mem(type,name,type1,arg1) \
type name(type1 arg1) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name),"b" ((long)(arg1)) \
        ); \
__syscall_return(type,__res); \
}
#define _syscall1(type,name,type1,arg1) \
type name(type1 arg1) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name),"b" ((long)(arg1)) \
	: "memory" ); \
__syscall_return(type,__res); \
}
#define _syscall2mem(type,name,type1,arg1,type2,arg2) \
type name(type1 arg1,type2 arg2) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name),"b" ((long)(arg1)),"c" ((long)(arg2)) \
	: "memory" ); \
__syscall_return(type,__res); \
}
#define _syscall2(type,name,type1,arg1,type2,arg2) \
type name(type1 arg1,type2 arg2) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name),"b" ((long)(arg1)),"c" ((long)(arg2)) \
        ); \
__syscall_return(type,__res); \
}
#define _syscall3mem(type,name,type1,arg1,type2,arg2,type3,arg3) \
type name(type1 arg1,type2 arg2, type3 arg3) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name),"b" ((long)(arg1)),"c" ((long)(arg2)), \
		  "d" ((long)(arg3)) \
	: "memory" ); \
__syscall_return(type,__res); \
}
#define _syscall3(type,name,type1,arg1,type2,arg2,type3,argc) \
type name(type1 arg1,type2 arg2, type3 arg3) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name),"b" ((long)(arg1)),"c" ((long)(arg2)), \
		  "d" ((long)(arg3)) \
        ); \
__syscall_return(type,__res); \
}
#define _syscall6mem(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4, \
	  type5,arg5,type6,arg6) \
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,type6 arg6) \
{ \
long __res; \
__asm__ volatile ("push %%ebp ; movl %%eax,%%ebp ; movl %1,%%eax ; int $0x80 ; pop %%ebp" \
	: "=a" (__res) \
	: "i" (__NR_##name),"b" ((long)(arg1)),"c" ((long)(arg2)), \
	  "d" ((long)(arg3)),"S" ((long)(arg4)),"D" ((long)(arg5)), \
	  "0" ((long)(arg6)) \
	: "memory"); \
__syscall_return(type,__res); \
}

#define __NR__exit	1
#define __NR_fork	2
#define __NR_read	3
#define __NR_write	4
#define __NR_open	5
#define __NR_close	6
#define __NR_pipe	42
#define __NR_munmap	91
#define __NR__llseek	140
#define __NR_msync	144
#define __NR_mmap2	192

static inline _syscall1(int,_exit,int,exitcode)
static inline _syscall0(int,fork)
static inline _syscall3mem(int,read,int,fd,void *,buf,size_t,count)
static inline _syscall3mem(int,write,int,fd,const void *,buf,size_t,count)
static inline _syscall3(off_t,_llseek,int,fd,off_t,offset,int,count)
static inline _syscall3(int,open,const char *,file,int,flag,int,mode)
static inline _syscall1(int,close,int,fd)
static inline _syscall6mem(int,mmap2,unsigned long,addr,unsigned long,len,
			unsigned long,prot,unsigned long,flags,
			unsigned long,fd,unsigned long,pgoff)
static inline _syscall2mem(int,munmap,void *,start,size_t,length)
static inline _syscall3mem(int,msync,const void *,start,size_t,length,int,flags)
static inline _syscall1mem(int,pipe,int *,fds)

extern void exit(int status);
extern const char *strerror(int err);
extern void *mmap(void *start, size_t length, int prot, int flags, int fd,
		  off_t offset);

#define	EINTR		 4	/* Interrupted system call */
#define STDOUT_FILENO    1
#define PROT_READ	0x1		/* page can be read */
#define PROT_WRITE	0x2		/* page can be written */
#define MAP_SHARED	0x01		/* Share changes */
#define MAP_ANONYMOUS	0x20		/* don't use a file */
#define MS_INVALIDATE	2		/* invalidate the caches */
#define MS_SYNC		4		/* synchronous memory sync */
#define MAP_FAILED	((void *)-1)
#define O_RDONLY	     00
#define O_LARGEFILE	0100000
#define NULL ((void *)0)
#define SEEK_SET	0	/* Seek from beginning of file.  */

extern int strcmp(const char *s1, const char *s2);
extern size_t strlen(const char *s);
extern void *memcpy(void *dest, const void *src, size_t n);
extern char *strrchr(const char *s, int c);

extern int _main(int argc, char *ptr);
extern int main(int argc, char **argv);

/* We halt immediately when an error occurs - there is no error flag */
static inline int ferror(FILE *file) { file=file; return 0; } 
extern int fclose(FILE *file);

#define lseek64 _llseek
#define putchar(c) putc(c, stdout)
#define printf(fmt, ...) fprintf(stdout, fmt, ## __VA_ARGS__)

extern void fflush(FILE *file);
static inline void putc(int c, FILE *file)
{
	file->buf[file->tail++]=c;
	if (file->tail==BUFSIZ) fflush(file);
}

#endif

#include "lib.h"
#include "util.h"

char outbuf[BUFSIZ], errbuf[BUFSIZ];
FILE stdoutobj={0,1,0,outbuf}, stderrobj={0,2,1,errbuf};
FILE *stdout=&stdoutobj, *stderr=&stderrobj;

int errno;

void fflush(FILE *file)
{
	char *ptr=file->buf;
	int len=file->tail;
	int fd=file->fd;
	
	while (len)
	{
		int bytes=write(fd, ptr, len);
		if (bytes==-1 && errno==EINTR) continue;
		if (bytes==-1) die("write (fd %i): %s\n", fd, strerror(errno));
		len-=bytes, ptr+=bytes;			
	}
	file->tail=0;
}

int fclose(FILE *file)
{
	if (close(file->fd)) die("close %s\n", strerror(errno));
	return 0;
}

static inline void putstring(FILE *file, const char *s)
{
	int len=strlen(s);
	int tlen=BUFSIZ-file->tail;

	if (tlen>len) tlen=len;
	memcpy(file->buf+file->tail, s, tlen);
	file->tail+=tlen;
	len-=tlen;
	s+=tlen;
	if (file->tail==BUFSIZ) fflush(file);

	while (len>=BUFSIZ)
	{
		memcpy(file->buf+file->tail, s, BUFSIZ);
		file->tail=BUFSIZ;
		len-=BUFSIZ;
		s+=BUFSIZ;
		fflush(file);
	}

	memcpy(file->buf, s, len);
	file->tail+=len;
}

static inline void putulonglong(FILE *file, unsigned long long lli)
{
	char buf[256];
	char *ptr=buf+256;
	
	*--ptr=0;
	do
	{
		*--ptr = '0' + (lli % 10);
		lli /= 10;		
	} while (lli);
	putstring(file, ptr);
}

static inline void putlonglong(FILE *file, long long lli)
{
	char buf[256];
	char *ptr=buf+256;
	int sign=0;
	
	if (lli<0) sign=1,lli=-lli;
	*--ptr=0;
	do
	{
		*--ptr = '0' + (lli % 10);
		lli /= 10;		
	} while (lli);
	if (sign) *ptr-- = '-';
	putstring(file, ptr);
}

void vfprintf(FILE *file, const char *format, va_list ap)
{
	int fmt=0, lfmt=0;
	char c;
	const char *s, *oformat=format;
	long long lli=0;

	while ((c=*format++))
	{
		if (fmt)
		{
			switch(c)
			{
			case 'l':
				lfmt++;
				break;
			case 'c':
				c=va_arg(ap, int);
				putc(c, file);
				fmt=0;
				break;
			case 's':
				s=va_arg(ap, const char *);
				putstring(file, s);
				fmt=0;
				break;
			case 'd':
			case 'i':
				switch(lfmt)
				{
				case 0:
					lli=va_arg(ap, int);
					break;
				case 1:
					lli=va_arg(ap, long);
					break;
				case 2:
					lli=va_arg(ap, long long);
					break;
				default:
					die("Bad integer format in %s\n",
					    format);
				}
				putlonglong(file, lli);
				fmt=0;
				break;
			case 'u':
				switch(lfmt)
				{
				case 0:
					lli=va_arg(ap, int);
					break;
				case 1:
					lli=va_arg(ap, long);
					break;
				case 2:
					lli=va_arg(ap, long long);
					break;
				default:
					die("Bad integer format in %s\n",
					    format);
				}
				putulonglong(file, lli);
				fmt=0;
				break;
			default:
				die("Bad format char %c in %s\n", c, oformat);
			}
		}
		else
		{
			if (c=='%')
				fmt=1, lfmt=0;
			else
				putc(c, file);
		}
	}
	if (file->autoflush) fflush(file);
}

void fprintf(FILE *file, const char *format, ...)
{
        va_list ap;
        va_start(ap, format);
        vfprintf(file, format, ap);
        va_end(ap);
}

void exit(int status)
{
	fflush(stdout);
	fflush(stderr);
	_exit(status);
}

extern void *mmap(void *start, size_t length, int prot, int flags, int fd,
		  off_t offset)
{
	return (void *)mmap2((unsigned long)start, length, prot, flags, fd, offset);
}


const char *strerror(int err)
{
	const char *errmsg[125]=
		{
			"Success",
			"Operation not permitted",
			"No such file or directory",
			"No such process",
			"Interrupted system call",
			"I/O error",
			"No such device or address",
			"Arg list too long",
			"Exec format error",
			"Bad file number",
			"No child processes",
			"Try again",
			"Out of memory",
			"Permission denied",
			"Bad address",
			"Block device required",
			"Device or resource busy",
			"File exists",
			"Cross-device link",
			"No such device",
			"Not a directory",
			"Is a directory",
			"Invalid argument",
			"File table overflow",
			"Too many open files",
			"Not a typewriter",
			"Text file busy",
			"File too large",
			"No space left on device",
			"Illegal seek",
			"Read-only file system",
			"Too many links",
			"Broken pipe",
			"Math argument out of domain of func",
			"Math result not representable",
			"Resource deadlock would occur",
			"File name too long",
			"No record locks available",
			"Function not implemented",
			"Directory not empty",
			"Too many symbolic links encountered",
			"Unknown error 41",
			"No message of desired type",
			"Identifier removed",
			"Channel number out of range",
			"Level 2 not synchronized",
			"Level 3 halted",
			"Level 3 reset",
			"Link number out of range",
			"Protocol driver not attached",
			"No CSI structure available",
			"Level 2 halted",
			"Invalid exchange",
			"Invalid request descriptor",
			"Exchange full",
			"No anode",
			"Invalid request code",
			"Invalid slot",
			"Unknown error 58",
			"Bad font file format",
			"Device not a stream",
			"No data available",
			"Timer expired",
			"Out of streams resources",
			"Machine is not on the network",
			"Package not installed",
			"Object is remote",
			"Link has been severed",
			"Advertise error",
			"Srmount error",
			"Communication error on send",
			"Protocol error",
			"Multihop attempted",
			"RFS specific error",
			"Not a data message",
			"Value too large for defined data type",
			"Name not unique on network",
			"File descriptor in bad state",
			"Remote address changed",
			"Can not access a needed shared library",
			"Accessing a corrupted shared library",
			".lib section in a.out corrupted",
			"Attempting to link in too many shared libraries",
			"Cannot exec a shared library directly",
			"Illegal byte sequence",
			"Interrupted system call should be restarted",
			"Streams pipe error",
			"Too many users",
			"Socket operation on non-socket",
			"Destination address required",
			"Message too long",
			"Protocol wrong type for socket",
			"Protocol not available",
			"Protocol not supported",
			"Socket type not supported",
			"Operation not supported on transport endpoint",
			"Protocol family not supported",
			"Address family not supported by protocol",
			"Address already in use",
			"Cannot assign requested address",
			"Network is down",
			"Network is unreachable",
			"Network dropped connection because of reset",
			"Software caused connection abort",
			"Connection reset by peer",
			"No buffer space available",
			"Transport endpoint is already connected",
			"Transport endpoint is not connected",
			"Cannot send after transport endpoint shutdown",
			"Too many references: cannot splice",
			"Connection timed out",
			"Connection refused",
			"Host is down",
			"No route to host",
			"Operation already in progress",
			"Operation now in progress",
			"Stale NFS file handle",
			"Structure needs cleaning",
			"Not a XENIX named type file",
			"No XENIX semaphores available",
			"Is a named type file",
			"Remote I/O error",
			"Quota exceeded",
			"No medium found",
			"Wrong medium type",
		};
	
	if (err<0) return "invalid error";
	if (err>=125) return "unknown error";
	return errmsg[err];
}

int strcmp(const char *s1, const char *s2)
{
	char c, d;
	while ((c=*s1)==(d=*s2) && *s1) s1++,s2++;
	return c-d;
}

size_t strlen(const char *s)
{
	const char *p=s;
	while (*p) p++;
	return p-s;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	char *d=(char *)dest;
	const char *s=(const char *)src;
	while (n--) *d++=*s++;
	return dest;
}

char *strrchr(const char *s, int c)
{
	unsigned char *p=(unsigned char *)s+strlen(s);
	while (p-- > (unsigned char *)s)
		if (*p==c) return p;
	return NULL;	
}

int atoi(const char *nptr)
{
	char c;
	int i=0;
	int sign=0;
	if (*nptr=='-')
		sign=1,nptr++;
	else if (*nptr=='+')
		nptr++;		
	while ((c=*nptr++))
	{
		if (c<'0' || c>'9') break;
		i=i*10+(c-'0');
	}
	return sign? -i:i;
}

long long atoll(const char *nptr)
{
	char c;
	long long i=0;
	int sign=0;
	if (*nptr=='-')
		sign=1,nptr++;
	else if (*nptr=='+')
		nptr++;		
	while ((c=*nptr++))
	{
		if (c<'0' || c>'9') break;
		i=i*10+(c-'0');
	}
	return sign? -i:i;
}

int _main(int argc, char *ptr)
{
	char *argv[argc+1]; /* gcc / C99 feature */
	int i=0;

	while (i<argc)
	{
		argv[i++]=ptr;
		while (*ptr++);
	}
	argv[i]=0;
	return main(argc, argv);;
}

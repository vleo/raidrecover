/* Comile with -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 */

#ifdef LINUXSTATIC
#include "lib.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#endif

#include "util.h"

#define MAXDISKS 12
#define DEFWINDOW 128

const char *diskname[MAXDISKS], *progname="xor";
unsigned char *window[MAXDISKS][2];
int windowalt=1;
int diskfd[MAXDISKS], request[MAXDISKS], reply[MAXDISKS];
off_t windowstart, windowend, datastart, datalen=0;
int disks=0, datasize=0, windowsize=DEFWINDOW*1024;

void usage(int status)
{
	fprintf(status? stderr:stdout, "Usage: %s [options] disks...\n"
		"        --window <n>"
		"        Set I/O window size in K (default: %i)\n"
		"        --start <n> "
		"        Skip to given position in bytes (default: 0)\n"
		"        --length <n>"
		"        Length of data (no default)\n",
		progname, DEFWINDOW);
	exit(status);
}

void diskhelper(int i, int rd, int wr)
{
	int bytes, dummy, datalen;
	while (1)
	{
		char *ptr;
		int len=windowsize;

		windowalt=!windowalt;
		ptr=window[i][windowalt];

		while (len)
		{
			bytes=read(diskfd[i], ptr, windowsize);
			if (bytes==-1 && errno==EINTR) continue;
			if (bytes==-1) 
				die("read (child %i, file %s): %s\n", 
				    i, diskname[i], strerror(errno));
			if (bytes==0) break;
			ptr+=bytes;
			len-=bytes;
		}

		datalen=windowsize-len;
		msync(window[i][windowalt], windowsize, MS_SYNC | MS_INVALIDATE);
		len=sizeof(datalen); ptr=(char *)&datalen;
		while (len)
		{
			bytes=write(wr, ptr, len);
			if (bytes==-1 && errno==EINTR) continue;
			if (bytes==-1) 
				die("write (child %i): %s\n",
				    i, strerror(errno));
			len-=bytes;
			ptr+=bytes;			
		}
		while ((bytes=read(rd, &dummy, 1))!=1)
		{
			if (bytes==-1 && errno==EINTR) continue;
			if (bytes==-1) 
				die("read (child %i): %s\n", 
				    i, strerror(errno));
			if (bytes==0) exit(0);
		}
	}
}

void parseopts(int argc, char *argv[])
{
	int i, optparsing=1;
	
	if (argc) setprogname(argv[0]);
	
	if (argc<=1) usage(1);

	for (i=1; i<argc; i++)
	{
		if (optparsing && argv[i][0] == '-')
		{
			if (!strcmp(argv[i], "--"))
			{
				optparsing=0;
				continue;
			}
			if (!strcmp(argv[i], "--help"))
				usage(0);
			if (!strcmp(argv[i], "--window"))
			{
				if (++i==argc) 
					die("--window takes an argument\n");
				windowsize=atoi(argv[i])*1024;
				if (windowsize <= 0) 
					die("Invalid window size\n");
				continue;
			}
			if (!strcmp(argv[i], "--start"))
			{
				if (++i==argc) 
					die("--start takes an argument\n");
				datastart=atoll(argv[i]);
				if (datastart < 0) 
					die("Invalid start position\n");
				continue;
			}
			if (!strcmp(argv[i], "--length"))
			{
				if (++i==argc) 
					die("--length takes an argument\n");
				datalen=atoll(argv[i]);
				if (datalen <= 0) 
					die("Invalid length\n");
				continue;
			}
			die("Unknown option %s\n", argv[i]);
		}
		if (disks==MAXDISKS) die("Too many disks -- increase MAXDISKS\n");
		diskname[disks++]=argv[i];
	}
	if (disks<2) die("Need at least two disk names\n");
	if (datalen <= 0) die("Need to specify mandatory argument: --length\n");
}

void starthelpers(void)
{
	int i;

	for (i=0; i<disks; i++)
	{
		int fdsa[2], fdsb[2];
		window[i][0]=mmap(NULL, windowsize, PROT_READ | PROT_WRITE, 
			       MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if (window[i][0] == MAP_FAILED)
			die("Map failed: %s\n", strerror(errno));
		window[i][1]=mmap(NULL, windowsize, PROT_READ | PROT_WRITE, 
			       MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if (window[i][1] == MAP_FAILED)
			die("Map failed: %s\n", strerror(errno));
		if (pipe(fdsa) || pipe(fdsb))
		    die("pipe: %s\n", strerror(errno));
		request[i]=fdsa[1]; reply[i]=fdsb[0];
		if ((diskfd[i]=open(diskname[i], O_RDONLY | O_LARGEFILE, 0))==-1)
			die("open %s: %s\n", diskname[i], strerror(errno));
		lseek64(diskfd[i], windowstart, SEEK_SET);
		switch (fork())
		{
		case -1:
			die("fork: %s\n", strerror(errno));
		case 0:
			if (i)
			{
				munmap(window[i-1][0], windowsize);
				munmap(window[i-1][1], windowsize);
				close(request[i-1]); close(reply[i-1]);
				close(diskfd[i-1]);
			}
			close(fdsa[1]); close(fdsb[0]);
			diskhelper(i, fdsa[0], fdsb[1]);
			exit(0);
		}
		close(fdsa[0]); close(fdsb[1]);
	}
}

int getnextwindow(void)
{
	int i, bytes, ret=windowsize, returned, dummy;

	for (i=0; i<disks; i++)
	{
		char *ptr=(char *)&returned;
		int len=sizeof(returned);
		while (len)
		{
			bytes=read(reply[i], ptr, len);
			if (bytes==-1 && errno==EINTR) continue;
			if (bytes==-1) die("read: %s\n", strerror(errno));
			if (!bytes) 
				die("read (parent): child %i died\n", i);
			len-=bytes;
			ptr+=bytes;
		}
		if (ret > returned) ret=returned;

		while ((bytes=write(request[i], &dummy, 1))!=1)
		{
			if (bytes==-1 && errno==EINTR) continue;
			die("write: %s\n", strerror(errno));
		}
	}

	windowstart += datasize;
	datasize = ret;
	windowend = windowstart + ret-1;
	windowalt = !windowalt;
	return ret;
}

void verifychecksum(void)
{
	int i, a;

	return;
	for (a=0; a<datasize; a+=4)
	{
		uint32_t xor=0;
		for (i=0; i<disks; i++)
			xor ^= *(uint32_t *)(window[i][windowalt]+a);
		if (xor) 
			die("Parity check failed at disk positition %lld\n", 
			    (long long)(windowstart+a));
	}
}

#if 0
void printpartial(void)
{
	off_t windowoffset = raidstart - windowstart*(disks-1);
	int stripe=raidstart/stripesize;
	int offset=raidstart%stripesize;
	off_t winoffset = (windowoffset/stripecyl)*stripesize+offset;
	int disk=stripe % (disks-1);
	int paritydisk=(stripe / (disks-1) + rotate) % disks;
	int len=stripesize-offset;
	int bytes;
	char *ptr;

	if (paritydisk <= disk) disk++;
	if (len>datalen) len=datalen;
	if (winoffset+len > datasize) len=datasize-winoffset;
	if (!len)
		die("Input ended with %llu output bytes unwritten\n", datalen);

	datastart+=len;
	datalen-=len;

	ptr=window[disk][windowalt]+winoffset;
	while (len)
	{
		bytes=write(STDOUT_FILENO, ptr, len);
		if (bytes==-1 && errno==EINTR) continue;
		if (bytes==-1) die("write: %s\n", strerror(errno));
		len-=bytes;
		ptr+=bytes;			
	}
}
#endif

void printpartial(void)
{
	off_t offset = datastart - windowstart;
	int len=windowsize-offset;

	if (len>datalen) len=datalen;
	if (offset+len > datasize) len=datasize-offset;
	if (!len)
		die("Input ended with %llu bytes early\n", datalen);
	datastart+=len;
	datalen-=len;

	while (len--)
	{
		unsigned char xor=0;
		int i;

		for (i=0; i<disks; i++)
			xor ^= window[i][windowalt][offset];
		putchar(xor);
		offset++;
	}
}


int main(int argc, char *argv[])
{
	return (6);
	parseopts(argc, argv);

	windowstart = (datastart/windowsize) * windowsize;
	starthelpers();
	getnextwindow();
	verifychecksum();

	while (datalen)
	{
		off_t nextwindow = (datastart/windowsize) * windowsize;
		if (windowstart != nextwindow)
		{
			getnextwindow();
			verifychecksum();
		}
		printpartial();
	}
	return ferror(stdout) || fclose(stdout);
}

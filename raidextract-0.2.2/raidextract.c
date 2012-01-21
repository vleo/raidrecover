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
#define DEFSTRIPE 64
#define DEFWINDOW 128

const char *diskname[MAXDISKS], *progname="raidextract";
char *window[MAXDISKS][2];
int windowalt=1, failed=-1;
int diskfd[MAXDISKS], request[MAXDISKS], reply[MAXDISKS];
off_t windowstart, windowend, windowcyl, stripecyl,
	raidstart=0, raidlen=0;
int disks=0, diskstart, datasize=0, rotate=0,
	stripesize=DEFSTRIPE*1024, windowsize=DEFWINDOW*1024;
int noparity = 0;

void usage(int status)
{
	fprintf(status? stderr:stdout, "Usage: %s [options] disks...\n"
		"        --stripe <n>"
		"        Set stripe size in K (default: %i)\n"
		"        --window <n>"
		"        Set I/O window size in K (default: %i)\n"
		"        --start <n> "
		"        Skip to given output position in bytes (default: 0)\n"
		"        --length <n>"
		"        Length of output (no default)\n"
		"        --rotate <n>"
		"        Rotate parity disk position (default: 0)\n"
		"        --noparity  "
		"        There is no parity, it's not raid5\n"
		"        --failed <n>"
		"        Ignore data on disk at index n as failed (default: none)\n",
		progname, DEFSTRIPE, DEFWINDOW);
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
			if (!strcmp(argv[i], "--stripe"))
			{
				if (++i==argc) 
					die("--stripe takes an argument\n");
				stripesize=atoi(argv[i])*1024;
				if (stripesize <= 0) 
					die("Invalid stripe size\n");
				continue;
			}
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
				raidstart=atoll(argv[i]);
				if (raidstart < 0) 
					die("Invalid start position\n");
				continue;
			}
			if (!strcmp(argv[i], "--noparity"))
			{
				noparity = 1;
				continue;
			}
			if (!strcmp(argv[i], "--length"))
			{
				if (++i==argc) 
					die("--length takes an argument\n");
				raidlen=atoll(argv[i]);
				if (raidlen <= 0) 
					die("Invalid length\n");
				continue;
			}
			if (!strcmp(argv[i], "--rotate"))
			{
				if (++i==argc) 
					die("--rotate takes an argument\n");
				rotate=atoi(argv[i]);
				continue;
			}
			if (!strcmp(argv[i], "--failed"))
			{
				if (++i==argc) 
					die("--failed takes an argument\n");
				failed=atoi(argv[i]);
				if (failed < 0) 
					die("Invalid failed disk\n");
				continue;
			}
			die("Unknown option %s\n", argv[i]);
		}
		if (disks==MAXDISKS) die("Too many disks -- increase MAXDISKS\n");
		diskname[disks++]=argv[i];
	}
	if (disks<2) die("Need at least two disk names\n");
	if (failed >= disks) die("Invalid failed disk\n");
	if (windowsize % stripesize) 
		die("Window size must be a multiple of stripe size\n");
	if (raidlen <= 0) die("Need to specify mandatory argument: --length\n");
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
		if (lseek64(diskfd[i], windowstart, SEEK_SET)==-1)
		    die("lseek failed: %s\n", strerror(errno));
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

	ret=(ret/stripesize) * stripesize;
	windowstart += datasize;
	datasize = ret;
	windowend = windowstart + ret-1;
	windowalt = !windowalt;
	return ret;
}

void verifychecksum(void)
{
	int i, a;
	int lastwarn=-stripesize;

	if (noparity)
		return;

	if (failed==-1)
	{
		for (a=0; a<datasize; a+=4)
		{
			uint32_t xor=0;
			for (i=0; i<disks; i++)
				xor ^= *(uint32_t *)(window[i][windowalt]+a);
			if (xor) 
				die("Parity check failed at disk positition %lld at byte %d\n", 
				    (long long)(windowstart+a), a);
		}
	}
	else
	{
		for (a=0; a<datasize; a+=4)
		{
			uint32_t xor=0;
			for (i=0; i<disks; i++)
				xor ^= *(uint32_t *)(window[i][windowalt]+a);
			if (xor) 
			{
				if (a-lastwarn>=stripesize)
				{
					warn("Parity check failed at disk positition %lld\n", 
					     (long long)(windowstart+a));
					lastwarn=a;
				}
			}
			xor ^= *(uint32_t *)(window[failed][windowalt]+a);
			*(uint32_t *)(window[failed][windowalt]+a) = xor;
		}
		
	}
}

void printpartial(void)
{
	off_t windowoffset = raidstart - windowstart*(disks-(!noparity));
	int stripe=raidstart/stripesize;
	int offset=raidstart%stripesize;
	off_t winoffset = (windowoffset/stripecyl)*stripesize+offset;
	int disk=stripe % (disks-(!noparity));
	int paritydisk=(stripe / (disks-1) + rotate) % disks;
	int len=stripesize-offset;
	int bytes;
	char *ptr;

	if (!noparity && paritydisk <= disk) disk++;
	if (len>raidlen) len=raidlen;
	if (winoffset+len > datasize) len=datasize-winoffset;
	if (!len)
		die("Input ended with %llu output bytes unwritten\n", raidlen);

/* 	fprintf(stderr, "\nraidlen=%lli\n", raidlen); */
/* 	fprintf(stderr, "raidstart=%llu\n", (long long)raidstart); */
/* 	fprintf(stderr, "windowstart=%llu\n", (long long)windowstart); */
/* 	fprintf(stderr, "windowoffset=%llu\n", (long long)windowoffset); */
/* 	fprintf(stderr, "stripe=%i\n", stripe); */
/* 	fprintf(stderr, "disk=%i\n", disk); */
/* 	fprintf(stderr, "paritydisk=%i\n", paritydisk); */
/*  	fprintf(stderr, "len=%i\n", len); */
/*  	fprintf(stderr, "print data [%lli] [%i..%i]\n", windowoffset/stripecyl, offset, offset+len-1); */

	raidstart+=len;
	raidlen-=len;

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

int main(int argc, char *argv[])
{
	parseopts(argc, argv);

	windowcyl = windowsize * (disks-(!noparity));
	stripecyl = stripesize * (disks-(!noparity));
	windowstart = (raidstart/windowcyl) * windowsize;
	starthelpers();
	getnextwindow();
	verifychecksum();

	while (raidlen)
	{
		off_t nextwindow = (raidstart/windowcyl) * windowsize;
		if (windowstart != nextwindow)
		{
			getnextwindow();
			verifychecksum();
		}
		printpartial();
	}

	return 0;
}

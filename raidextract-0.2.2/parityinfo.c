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

const char *diskname[MAXDISKS], *progname="parityinfo";
int diskfd[MAXDISKS], request[MAXDISKS], reply[MAXDISKS];
off_t windowstart, windowend, windowcyl, stripecyl,
	raidstart=0, raidlen=0;
int disks=0, diskstart, datasize=0, rotate=0,
	stripesize=DEFSTRIPE*1024, windowsize=DEFWINDOW*1024;

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
		"        Rotate parity disk position (default: 0)\n",
		progname, DEFSTRIPE, DEFWINDOW);
	exit(status);
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
			die("Unknown option %s\n", argv[i]);
		}
		if (disks==MAXDISKS) die("Too many disks -- increase MAXDISKS\n");
		diskname[disks++]=argv[i];
	}
	if (disks<2) die("Need at least two disk names\n");
	if (windowsize % stripesize) 
		die("Window size must be a multiple of stripe size\n");
	if (raidlen <= 0) die("Need to specify mandatory argument: --length\n");
}

int getnextwindow(void)
{
	windowstart += datasize;
	datasize = windowsize;
	windowend = windowstart + windowsize-1;
	printf("Reading from all disks, position %lli to %lli "
	       "(stripe %lli to %lli)\n", 
	       windowstart, windowend, 
	       windowstart/stripesize, windowend/stripesize);
	return windowsize;
}

void printpartial(void)
{
	off_t windowoffset = raidstart - windowstart*(disks-1);
	int stripe=raidstart/stripesize;
	int offset=raidstart%stripesize;
	off_t winoffset = (windowoffset/stripecyl)*stripesize+offset;
	int disk=stripe % (disks-1);
	int paritydisk=(stripe / (disks-1) + rotate) % disks;
	int len=stripesize-offset;

	if (paritydisk <= disk) disk++;
	if (len>raidlen) len=raidlen;
	if (winoffset+len > datasize) len=datasize-winoffset;

	printf("\nOutput position %lli (stripe %i)\n",
	       raidstart, stripe);
	printf("  data disk=%i %s\n", disk, diskname[disk]);
	printf("  parity disk=%i %s\n", paritydisk, diskname[paritydisk]);
 	printf("  data %lli (stripe %lli) [%i..%i]\n", windowstart+(windowoffset/stripecyl)*stripesize, windowstart/stripesize+(windowoffset/stripecyl), offset, offset+len-1);

	raidstart+=len;
	raidlen-=len;
}

int main(int argc, char *argv[])
{
	parseopts(argc, argv);

	windowcyl = windowsize * (disks-1);
	stripecyl = stripesize * (disks-1);
	printf("Length of stripe across all disks (except parity): "
		"%llu\n", stripecyl);
	printf("Number of disks: %i\n", disks);
	windowstart = (raidstart/windowcyl) * windowsize;
	getnextwindow();

	while (raidlen)
	{
		off_t nextwindow = (raidstart/windowcyl) * windowsize;
		if (windowstart != nextwindow)
		{
			getnextwindow();
		}
		printpartial();
	}

	return 0;
}

#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif

#include <sys/syscall.h>
#include <linux/linkage.h>

        .text
ENTRY(_start)
        call _main
	pushl %eax
	call exit

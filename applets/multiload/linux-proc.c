/* From wmload.c, v0.9.2, licensed under the GPL. */
#include <sys/types.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>

#include <assert.h>

#include <glibtop.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>

#include "linux-proc.h"

#define NCPUSTATES 4

static long cp_time[NCPUSTATES];
static long last[NCPUSTATES];

static unsigned needed_cpu_flags =
(1 << GLIBTOP_CPU_USER) +
(1 << GLIBTOP_CPU_NICE) +
(1 << GLIBTOP_CPU_SYS) +
(1 << GLIBTOP_CPU_IDLE);

static unsigned needed_mem_flags =
(1 << GLIBTOP_MEM_USED) +
(1 << GLIBTOP_MEM_FREE);

static unsigned needed_swap_flags = 0;

void
GetLoad (int Maximum, int *usr, int *nice, int *sys, int *free)
{
	glibtop_cpu cpu;
	int total;
	
	glibtop_get_cpu (&cpu);
	
	assert ((cpu.flags & needed_cpu_flags) == needed_cpu_flags);
	
	cp_time [0] = cpu.user;
	cp_time [1] = cpu.nice;
	cp_time [2] = cpu.sys;
	cp_time [3] = cpu.idle;
	
	*usr  = cp_time [0] - last [0];
	*nice = cp_time [1] - last [1];
	*sys  = cp_time [2] - last [2];
	*free = cp_time [3] - last [3];

	total = *usr + *nice + *sys + *free;
	
	last [0] = cp_time [0];
	last [1] = cp_time [1];
	last [2] = cp_time [2];
	last [3] = cp_time [3];
	
	*usr  = rint (Maximum * (float)(*usr)  / total);
	*nice = rint (Maximum * (float)(*nice) / total);
	*sys  = rint (Maximum * (float)(*sys)  / total);
	*free = rint (Maximum * (float)(*free) / total);

#ifdef DEBUG
	fprintf (stderr, "CPU: %d - %d - (%d, %d, %d, %d)\n",
		 Maximum, total, *usr, *sys, *nice, *free);
#endif
}

void
GetMemory (int Maximum, int *used, int *shared, int *buffer, int *cached)
{
	glibtop_mem mem;
	
	glibtop_get_mem (&mem);
	
	assert ((mem.flags & needed_mem_flags) == needed_mem_flags);

#ifdef DEBUG
	fprintf (stderr, "Memory: %Lu - %Lu - %Lu - (%Lu, %Lu, %Lu)\n",
		 mem.total, mem.free, mem.used, mem.shared, mem.buffer,
		 mem.cached);
#endif

	mem.total = mem.free + mem.used + mem.shared +
		mem.buffer + mem.cached;

	*used    = rint (Maximum * (float)mem.used   / mem.total);
	*shared  = rint (Maximum * (float)mem.shared / mem.total);
	*buffer  = rint (Maximum * (float)mem.buffer / mem.total);
	*cached  = rint (Maximum * (float)mem.cached / mem.total);

#ifdef DEBUG
	fprintf (stderr, "MEM: %d - %Lu - (%d, %d, %d, %d)\n",
		 Maximum, mem.total, *used, *shared, *buffer, *cached);
#endif
}

void
GetSwap (int Maximum, int *used, int *free)
{
	glibtop_swap swap;
	
	glibtop_get_swap (&swap);
	
	assert ((swap.flags & needed_swap_flags) == needed_swap_flags);

#ifdef DEBUG
	fprintf (stderr, "Swap: %Lu - %Lu - %Lu\n",
		 swap.total, swap.free, swap.used);
#endif

	swap.total = swap.free + swap.used;

	if (swap.total == 0) {	/* Avoid division by zero */
		*used = *free = 0;
		return;
	}

	*used    = rint (Maximum * (float)swap.used / swap.total);
	*free    = rint (Maximum * (float)swap.free / swap.total);

#ifdef DEBUG
	fprintf (stderr, "SWAP: %d - %Lu - (%d, %d)\n",
		 Maximum, swap.total, *used, *free);
#endif
}

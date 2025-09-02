#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

int
main(int argc, char **argv)
{
	if (argc > 1)
	{
		printf(1, "Usage: disppstat\n");
		exit(1);
	}

	// static char *states[] = {
	// 	[0] = "unused",
	// 	[1] = "embryo",
	// 	[2] = "sleep ",
	// 	[3] = "runble",
	// 	[4] = "run   ",
	// 	[5] = "zombie"
	// };
	struct pstat *pstat = (struct pstat *) malloc(sizeof(struct pstat));
	getpinfo(pstat);

	// int* buf = (int *) malloc(sizeof(int) * 64);
	// proclist(buf);
	int i;

    printf(1, "+-----+-------+---------+----------+------------+\n");
    printf(1, "| PID | Inuse | Tickets | Runticks | Boostsleft |\n");
    printf(1, "+-----+-------+---------+----------+------------+\n");

	for (i = 0; i < NPROC; i++) {
		printf(1, "|  %d  |   %d   |    %d    |     %d    |     %d     |\n",
                  pstat->pid[i],
                  pstat->inuse[i],
                  pstat->tickets[i],
                  pstat->runticks[i],
                  pstat->boostsleft[i]);
	}
	free(pstat);

	exit(0);
}

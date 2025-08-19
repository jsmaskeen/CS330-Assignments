#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char **argv)
{
	static char *states[] = {
		[0] = "unused",
		[1] = "embryo",
		[2] = "sleep ",
		[3] = "runble",
		[4] = "run   ",
		[5] = "zombie"
	};

	int* buf = (int *) malloc(sizeof(int) * 64);
	proclist(buf);
	int i;

	printf(1, "Process ID\tParent PID\tProcess Name\tState\tNumber of System Calls\n");

	for (i = 0; i < 64; i++) {
		if (buf[i] == -1)
			continue;
		char proc_name[16];
		get_procname(buf[i], proc_name);
		printf(1, "%d\t\t%d\t\t%s\t\t%s\t%d\n",
			buf[i],
			get_parproc(buf[i]),
			proc_name,
			states[get_procstate(buf[i])],
			get_procnsyscalls(buf[i]));
	}
	free(buf);

	exit();
}

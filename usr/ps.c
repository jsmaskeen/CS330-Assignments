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

    printf(1, "+-----+------------+------+-------+------------------+\n");
    printf(1, "| PID | Parent PID | Name | State | No. of Sys Calls |\n");
    printf(1, "+-----+------------+------+-------+------------------+\n");

	for (i = 0; i < 64; i++) {
		if (buf[i] == -1)
			continue;
		char proc_name[16];
        int parent = get_parproc(buf[i]);
		get_procname(buf[i], proc_name);
		printf(1, "| %d | %d | %s | %s | %d |\n",
                  buf[i],
                  parent > -1 ? parent : -1,
                  proc_name,
                  states[get_procstate(buf[i])],
                  get_procnsyscalls(buf[i]));
	}
	free(buf);

	exit();
}

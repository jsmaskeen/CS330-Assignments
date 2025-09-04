#include "types.h"
#include "stat.h"
#include "user.h"

struct pstat {
  int inuse[64]; // whether this slot of the process table is in use (1 or 0)
  int pid[64]; // PID of each process
  int tickets[64];  // how many tickets does this process have?
  int runticks[64];  // total number of timer ticks this process has been scheduled
  int boostsleft[64]; // how many more ticks will this process be boosted?
};

int
main(int argc, char **argv)
{
	// int c1 = 0;
	// int c2 = 0;
	struct pstat *pstat_table = (struct pstat *)malloc(sizeof(struct pstat));

	for (int i = 0; i < 10; i++) {
		int pid1 = -1, pid2 = -1;
		pid1 = fork();
		if (pid1 > 0) {
			settickets(pid1, 10);
			pid2 = fork();
		} 
		
		if (pid2 > 0) {
			settickets(pid2, 5);
		}
		// TODO: set the number of tickest for both the processes

		if (pid1 == 0) {
			for (int j = 0; j < 10000000; j++);
			int curpid = getpid();
			getpinfo(pstat_table);

			for (int i = 0; i < 64; i++) {
				if (pstat_table->pid[i] == curpid) {
					printf(1, "Ticks for PID %d: %d\n", curpid, pstat_table->runticks[i]);
				}
			}
			break;
		} else if (pid2 == 0) {
			for (int j = 0; j < 10000000; j++);
			int curpid = getpid();
			getpinfo(pstat_table);

			for (int i = 0; i < 64; i++) {
				if (pstat_table->pid[i] == curpid) {
					printf(1, "Ticks for PID %d: %d\n", curpid, pstat_table->runticks[i]);
				}
			}
			break;
		}

		if (pid1 > 0 && pid2 > 0) {
			wait();
			wait();

			printf(1, "Child pids: %d %d\n", pid1, pid2);
		}
	}
    
    exit(0);
}

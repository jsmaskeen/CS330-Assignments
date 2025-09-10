
#include "types.h"
#include "stat.h"
#include "user.h"

struct pstat {
  int inuse[64];        // whether this slot of the process table is in use (1 or 0)
  int pid[64];          // PID of each process
  int tickets[64];      // how many tickets does this process have?
  int runticks[64];     // total number of timer ticks this process has been scheduled
  int boostsleft[64];   // how many more ticks will this process be boosted?
};

int main(int argc, char **argv) {
    struct pstat *pstat_table = (struct pstat *)malloc(sizeof(struct pstat));

    // Fork 2 child processes
    int pid1 = fork();
    if (pid1 == 0) {
        // Child 1
        settickets(getpid(), 50);  // Give 50 tickets to Child 1
        for (int j = 0; j < 100000000; j++);  // Run for a long time
    }

    int pid2 = fork();
    if (pid2 == 0) {
        // Child 2
        settickets(getpid(), 20);  // Give 20 tickets to Child 2
        for (int j = 0; j < 100000000; j++);  // Run for a long time
    }

    // Parent process
    if (pid1 > 0 && pid2 > 0) {
        // Wait for both child processes to finish
        wait();
        wait();

        // After the children are done, get their runticks
        getpinfo(pstat_table);

        int total_runticks1 = 0, total_runticks2 = 0, total_runticks3 = 0;
        
        for (int i = 0; i < 64; i++) {
            // Process runticks for Parent (pid = getpid())
            if (pstat_table->pid[i] == getpid()) {
                total_runticks3 = pstat_table->runticks[i];
            }
            // Process runticks for Child 1 (pid1)
            if (pstat_table->pid[i] == pid1) {
                total_runticks1 = pstat_table->runticks[i];
            }
            // Process runticks for Child 2 (pid2)
            if (pstat_table->pid[i] == pid2) {
                total_runticks2 = pstat_table->runticks[i];
            }
        }

        // Print the runticks of all three processes
        printf(1, "Parent (PID %d) Ticks: %d\n", getpid(), total_runticks3);
        printf(1, "Child 1 (PID %d) Ticks: %d\n", pid1, total_runticks1);
        printf(1, "Child 2 (PID %d) Ticks: %d\n", pid2, total_runticks2);

        // Compare the ratio of runticks
        printf(1, "Ratio of runticks (Child 1 / Parent): %d/%d = %f\n", total_runticks1, total_runticks3, (float)total_runticks1 / total_runticks3);
        printf(1, "Ratio of runticks (Child 2 / Parent): %d/%d = %f\n", total_runticks2, total_runticks3, (float)total_runticks2 / total_runticks3);
    }

    exit(0);
}

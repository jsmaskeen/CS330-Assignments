//to test whether the ratio of total ticks the processes are scheduled for is comparable to the ratio of the tickets assigned to them.

#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

void cpu_workload()
{
    for (volatile unsigned long long i = 0; i < 99999999999ULL; i++) {
    }
}

int
main(int argc, char *argv[])
{
    struct pstat st;
    int pid_high, pid_low;
    int final_ticks_high = -1;
    int final_ticks_low = -1;
    int low_tickets = 10;
    int high_tickets = 50;

    printf(1, "Starting proportional test...\n");
    settickets(getpid(), 1000000); //set very large number of tickets for parent so that when it wakes up it is scheduled for sure.
    
    pid_low = fork();
    if (pid_low == 0) {
        settickets(getpid(), low_tickets);
        cpu_workload();
        exit(0);
    }

    pid_high = fork();
    if (pid_high == 0) {
        settickets(getpid(), high_tickets);
        cpu_workload();
        exit(0);
    }

    printf(1, "Created two CPU-bound children:\n  - PID %d with %d tickets\n  - PID %d with %d tickets\n", pid_low, low_tickets, pid_high, high_tickets);
    printf(1, "Running test for 200 ticks...\n\n");

    sleep(200); //put parent to sleep so the children can run.

    if (getpinfo(&st) != 0) {
        printf(2, "getpinfo failed\n");
        kill(pid_low);
        kill(pid_high);
        wait();
        wait();
        exit(0);
    }

    kill(pid_low);
    kill(pid_high);

    wait();
    wait();

    for (int i = 0; i < 64; i++) {
        if (st.pid[i] == pid_high) {
            final_ticks_high = st.runticks[i];
        }
        if (st.pid[i] == pid_low) {
            final_ticks_low = st.runticks[i];
        }
    }

    printf(1, "--- Test Finished ---\n");
    printf(1, "Final Run Ticks for High Priority (PID %d, %d tickets): %d\n", pid_high, high_tickets, final_ticks_high);
    printf(1, "Final Run Ticks for Low Priority (PID %d, %d tickets): %d\n", pid_low, low_tickets, final_ticks_low);
    printf(1, "Ratio of scheduled time: %d/100\n", (int)(final_ticks_high * 100 /final_ticks_low));
    printf(1, "---------------------\n");

    exit(0);
}


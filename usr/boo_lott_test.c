#include "types.h"
#include "stat.h"
#include "user.h"

struct pstat {
  int inuse[64];       
  int pid[64];         
  int tickets[64];     
  int runticks[64];    
  int boostsleft[64]; 
};

void cpu_workload()
{
    for (volatile unsigned long long i = 0; i < 999999999ULL; i++) {
        ; 
    }
}

int
main(int argc, char *argv[])
{
    struct pstat st;
    int pid_high, pid_low;
    int final_ticks_high = -1;
    int final_ticks_low = -1;

    printf(1, "Starting proportional test...\n");

    pid_low = fork();
    if (pid_low == 0) {
        cpu_workload();
        exit(0);
    }
    settickets(pid_low, 10);


    pid_high = fork();
    if (pid_high == 0) {
        cpu_workload();
        exit(0); 
    }
    settickets(pid_high, 40);

    printf(1, "Created two CPU-bound children:\n  - PID %d with 10 tickets\n  - PID %d with 50 tickets\n", pid_low, pid_high);
    printf(1, "Running test for 40 ticks (approx 5 seconds)...\n\n");

    sleep(50);

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
    printf(1, "Final Run Ticks for High Priority (PID %d, 40 tickets): %d\n", pid_high, final_ticks_high);
    printf(1, "Final Run Ticks for Low Priority (PID %d, 10 tickets): %d\n", pid_low, final_ticks_low);
    printf(1, "---------------------\n");

    exit(0);
}


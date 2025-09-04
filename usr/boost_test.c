#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

void light_task()
{
    for (volatile int i = 0; i < 50000000; i++)
    {
        ;
    }
}

void heavy_task()
{
    while (1)
    {
        for (volatile int i = 0; i < 100000000; i++)
        {
            ;
        }
    }
}

int main(int argc, char *argv[])
{
    struct pstat st;
    int pid_sleeping_proc, pid_non_sleeping_proc;
    int ticks_sleeping_final = -1;
    int ticks_non_sleeping_final = -1;

    printf(1, "Boost accumulation test\n");

    if (settickets(getpid(), 300) < 0)
    {
        printf(2, "settickets failed for parent process\n");
    }

    pid_non_sleeping_proc = fork();
    if (pid_non_sleeping_proc == 0)
    {
        if (settickets(getpid(), 50) < 0)
        {
            printf(2, "settickets failed for non sleeping process\n");
        }
        heavy_task();
        exit(0);
    }

    pid_sleeping_proc = fork();
    if (pid_sleeping_proc == 0)
    {
        if (settickets(getpid(), 50) < 0)
        {
            printf(2, "settickets failed for sleeping process\n");
        }
        // Idea is that proc goes to sleep before using all boosts
        for (int i = 0; i < 5; i++)
        {
            printf(1, "Sleeping process (PID %d) doing work, loop %d/5.\n", getpid(), i + 1);
            light_task();
            printf(1, "Sleeping process (PID %d) going to sleep. Current boosts: %d\n", getpid());
            sleep(50);
        }
        printf(1, "Sleeping process (PID %d) finished its loops.\n", getpid());
        exit(0);
    }

    printf(1, "Created two child processes with 50 tickets each:\n");
    printf(1, "Non sleeping process PID: %d (runs continuously)\n", pid_non_sleeping_proc);
    printf(1, "Sleeping process PID: %d (sleeps before using all boosts)\n", pid_sleeping_proc);
    printf(1, "\nRunning test for about 350 ticks\n\n");

    sleep(350);

    if (getpinfo(&st) != 0)
    {
        printf(2, "getpinfo failed\n");
        kill(pid_non_sleeping_proc);
        wait();
        wait();
        exit(1);
    }

    kill(pid_non_sleeping_proc);
    wait();
    wait();

    for (int i = 0; i < NPROC; i++)
    {
        if (st.pid[i] == pid_sleeping_proc)
        {
            ticks_sleeping_final = st.runticks[i];
        }
        if (st.pid[i] == pid_non_sleeping_proc)
        {
            ticks_non_sleeping_final = st.runticks[i];
        }
    }

    printf(1, "\nTest Finished\n");
    printf(1, "Ticks the non sleeping process ran (PID %d): %d\n", pid_non_sleeping_proc, ticks_non_sleeping_final);
    printf(1, "Ticks the sleeping process ran (PID %d): %d\n", pid_sleeping_proc, ticks_sleeping_final);
    // The sleeping process should have a high number of run ticks,
    // showing that its boosts accumulated each time it went to sleep
    // before using up the previous boosts.
    exit(0);
}

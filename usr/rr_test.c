#include "types.h"
#include "stat.h"
#include "user.h"

#define NCHILD 3

int
main(int argc, char *argv[])
{
    int i, pid;

    printf(1, "rr sched test starting with %d children...\n", NCHILD);

    for (i = 0; i < NCHILD; i++) {
        pid = fork();
        if (pid == 0) {
            // Child process
            int count = 0;
            for (int k = 0; k < 50; k++) {
                // simulate CPU work
                for (volatile int j = 0; j < 1000000; j++) {
                    ;
                }
                count++;
                printf(1, "Child %d turn %d (pid %d)\n", i, count,getpid());
            }
            printf(1, "Child %d finished with %d turns\n", i, count);
            exit(0);
        }
    }

    for (i = 0; i < NCHILD; i++) {
        wait();
    }

    printf(1, "rr test done.\n");
    exit(0);
}

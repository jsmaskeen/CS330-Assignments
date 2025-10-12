/*----------xv6 sync lab----------*/
#include "types.h"
#include "arm.h"
#include "spinlock.h"
#include "defs.h"
#include "barrier.h"

// define any variables needed here
static struct spinlock barrierlock;
static int barrier_N;
static int barrier_counter;
static int barrier_isinit = 0;

int barrier_init(int n)
{
    if (!barrier_isinit)
    {
        initlock(&barrierlock, "barrier lock");
        barrier_isinit = 1;
    }
    acquire(&barrierlock);
    barrier_N = n;
    barrier_counter = 0;
    release(&barrierlock);
    return 0;
}

int barrier_check(void)
{
    acquire(&barrierlock);
    barrier_counter++;
    if (barrier_counter < barrier_N)
        sleep(&barrier_counter, &barrierlock);
    else
        wakeup(&barrier_counter);
    release(&barrierlock);
    return 0;
}

/*----------xv6 sync lock end----------*/

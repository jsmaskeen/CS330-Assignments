#include "types.h"
#include "arm.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "pstat.h"
#include "barrier.h"
#include "spinlock.h"

static int available_channel = 1;

extern struct {
    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;

int sys_fork(void)
{
    return fork();
}

int sys_exit(void)
{
    int exit_id;

    if (argint(0, &exit_id) < 0)
    {
        return -1;
    }

    exit(exit_id);
    return 0;
}

int sys_wait(void)
{
    return wait();
}

int sys_kill(void)
{
    int pid;

    if (argint(0, &pid) < 0)
    {
        return -1;
    }

    return kill(pid);
}

int sys_getpid(void)
{
    return proc->is_main_thread ? proc->pid : proc->parent->pid; // return the pid of the parent
}

int sys_sbrk(void)
{
    int addr;
    int n;

    if (argint(0, &n) < 0)
    {
        return -1;
    }

    addr = proc->sz;

    if (growproc(n) < 0)
    {
        return -1;
    }

    return addr;
}

int sys_sleep(void)
{
    int n;
    uint ticks0;

    if (argint(0, &n) < 0)
    {
        return -1;
    }

    acquire(&tickslock);

    ticks0 = ticks;
    proc->wakeup_tick = ticks0 + n;
    // for debugging / visualizing
    // while(ticks - ticks0 < n){
    while (ticks < proc->wakeup_tick)
    {
        if (proc->killed)
        {
            release(&tickslock);
            return -1;
        }
        // cprintf("%s (%d) sleepcall\n",proc->name, proc->pid);
        sleep(&ticks, &tickslock);
    }
    proc->wakeup_tick = 0;
    release(&tickslock);
    proc->boosts += n; // give boosts for sleep time
    return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int sys_uptime(void)
{
    uint xticks;

    acquire(&tickslock);
    xticks = ticks;
    release(&tickslock);

    return xticks;
}

// returns the clock frequency in HZ
int sys_clockfreq(void)
{
    return HZ;
}

int sys_uartputc(void)
{
    int c;
    if (argint(0, &c) < 0)
    {
        return -1;
    }

    uartputc(c);
    return 0;
}

int sys_get_proclist(void)
{
    char *buf;

    if (argptr(0, &buf, 8) < 0)
    {
        return -1;
    }

    return get_proclist((int *)buf);
}

int sys_get_parproc(void)
{
    int pid;

    if (argint(0, &pid) < 0)
    {
        return -1;
    }

    return get_parproc(pid);
}

int sys_get_procname(void)
{
    int pid;
    char *buf;

    if (argint(0, &pid) < 0)
    {
        return -1;
    }

    if (argptr(1, &buf, 8) < 0)
    {
        return -1;
    }

    return get_procname(pid, buf);
}

int sys_get_procstate(void)
{
    int pid;

    if (argint(0, &pid) < 0)
    {
        return -1;
    }

    return get_procstate(pid);
}

int sys_get_procnsyscalls(void)
{
    int pid;

    if (argint(0, &pid) < 0)
    {
        return -1;
    }

    return get_nsyscall(pid);
}

int sys_getpinfo(void)
{
    struct pstat *pstat;
    if (argptr(0, (char **)&pstat, sizeof(*pstat)) < 0)
    {
        return -1;
    }
    return get_pinfo(pstat);
}

int sys_srand(void)
{
    int seed;
    if (argint(0, &seed) < 0)
        return -1;
    srand(seed);
    return 0;
}

int sys_settickets(void)
{
    int pid, n_tickets;
    if (argint(0, &pid) < 0 || argint(1, &n_tickets) < 0)
        return -1;

    return settickets(pid, n_tickets);
}

int sys_pgdump(void)
{
    int print_full;
    if (argint(0, &print_full) < 0)
    {
        return -1;
    }
    pgdump1(proc->pgdir, print_full);
    return 0;
}

int sys_kpgdump(void)
{
    kpgdump1();
    return 0;
}

int sys_thread_create(void)
{
    char* tid_ptr;
    char* function_ptr;
    char* args;

    if (argptr(0, &tid_ptr, 8) < 0 || argptr(1, &function_ptr, 8) < 0 || argptr(2, &args, 8)) {
        return -1;
    }

    return thread_create((int *) tid_ptr, function_ptr, args);
}

int sys_thread_exit(void)
{
    return thread_exit();
}

int sys_thread_join(void)
{
    int tid;
    if (argint(0, &tid) < 0) 
        return -1;
    return thread_join(tid);
}

int sys_barrier_init(void)
{
    int n;
    if (argint(0, &n) < 0)
        return -1;
    return barrier_init(n);
}

int sys_barrier_check(void)
{
    return barrier_check();
}

int sys_waitpid(void)
{
    return -1;
}

int sys_sleepChan(void)
{
    int chan;
    if (argint(0, &chan) < 0) return -1;
    if (chan < 0) return -1;

    // struct spinlock* lock = &CHANNE_LOCKS[chan];
    acquire(&ptable.lock);
    sleep((void *)(chan), &ptable.lock); //technically any lock works
    release(&ptable.lock);

    return 0;
}

int sys_getChannel(void)
{
    int chan;
    acquire(&ptable.lock);
    chan = available_channel++;
    release(&ptable.lock);
    return chan;
}

int sys_sigChan(void)
{
    int chan;
    if (argint(0, &chan) < 0) return -1;

    if (chan < 0) return 0;
    wakeup((void *)(chan));

    return 0;
}

int sys_sigOneChan(void)
{
    int chan;
    if (argint(0, &chan) < 0) return -1;

    if (chan < 0) return 0;
    wakeup_one((void *)(chan));
    return 0;
}

int sys_interruptoff(void)
{
    cli();
    // pushcli();
    return 0;
}

int sys_interrupton(void)
{
    sti();
    // popcli();
    return 0;
}
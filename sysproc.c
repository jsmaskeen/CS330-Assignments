#include "types.h"
#include "arm.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "pstat.h"

int sys_fork(void)
{
    return fork();
}

int sys_exit(void)
{
    int exit_id;
    
    if (argint(0, &exit_id) < 0){
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

    if(argint(0, &pid) < 0) {
        return -1;
    }

    return kill(pid);
}

int sys_getpid(void)
{
    return proc->pid;
}

int sys_sbrk(void)
{
    int addr;
    int n;

    if(argint(0, &n) < 0) {
        return -1;
    }

    addr = proc->sz;

    if(growproc(n) < 0) {
        return -1;
    }

    return addr;
}

int sys_sleep(void)
{
    int n;
    uint ticks0;

    if(argint(0, &n) < 0) {
        return -1;
    }

    acquire(&tickslock);

    ticks0 = ticks;
    proc->wakeup_tick = ticks0 + n;
    // for debugging / visualizing
    // while(ticks - ticks0 < n){
    while(ticks < proc->wakeup_tick){
        if(proc->killed){
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


int sys_uartputc (void){
    int c;
    if(argint(0, &c) < 0) {
        return -1;
    }

    uartputc(c);
    return 0;
}


int sys_get_proclist(void) {
    char* buf;

    if (argptr(0, &buf, 8) < 0) {
        return -1;
    }

    return get_proclist((int *)buf);
}

int sys_get_parproc(void) {
    int pid;

    if (argint(0, &pid) < 0) {
        return -1;
    }

    return get_parproc(pid);
}

int sys_get_procname(void) {
    int pid;
    char* buf;

    if (argint(0, &pid) < 0) {
        return -1;
    }

    if (argptr(1, &buf, 8) < 0) {
        return -1;
    }

    return get_procname(pid, buf);
}

int sys_get_procstate(void) {
    int pid;

    if (argint(0, &pid) < 0) {
        return -1;
    }

    return get_procstate(pid);
}

int sys_get_procnsyscalls(void) {
    int pid;

    if (argint(0, &pid) < 0) {
        return -1;
    }

    return get_nsyscall(pid);
}

int sys_getpinfo(void) {
    struct pstat *pstat;
    if(argptr(0, (char**)&pstat, sizeof(*pstat)) < 0) {
        return -1;
    }
    return get_pinfo(pstat);
}

int sys_srand(void) {
    int seed;
    if (argint(0, &seed) < 0) 
        return -1;
    srand(seed);
    return 0;
}

int sys_settickets(void) {
    int pid, n_tickets;
    if (argint(0, &pid) < 0 || argint(1, &n_tickets) < 0) 
        return -1;

    return settickets(pid, n_tickets);
}

int sys_pgdump(void) {
    pgdump1(proc->pgdir);
    return 0;
}
int sys_kpgdump(void) {
    kpgdump1();
    return 0;
}
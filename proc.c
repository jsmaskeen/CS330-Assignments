#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "arm.h"
#include "proc.h"
#include "spinlock.h"
#include "pstat.h"
#include "usyscall.h"

#define RAND_MAX 0x7fffffff
uint rseed = 0;

uint rand() {
    return rseed = (rseed * 1103515245 + 12345) & RAND_MAX;
}

//
// Process initialization:
// process initialize is somewhat tricky.
//  1. We need to fake the kernel stack of a new process as if the process
//     has been interrupt (a trapframe on the stack), this would allow us
//     to "return" to the correct user instruction.
//  2. We also need to fake the kernel execution for this new process. When
//     swtch switches to this (new) process, it will switch to its stack,
//     and reload registers with the saved context. We use forkret as the
//     return address (in lr register). (In x86, it will be the return address
//     pushed on the stack by the process.)
//
// The design of context switch in xv6 is interesting: after initialization,
// each CPU executes in the scheduler() function. The context switch is not
// between two processes, but instead, between the scheduler. Think of scheduler
// as the idle process.
//
struct {
    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;

static struct proc *initproc;
struct proc *proc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void pinit(void)
{
    initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc* allocproc(void)
{
    struct proc *p;
    char *sp;

    acquire(&ptable.lock);

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if(p->state == UNUSED) {
            goto found;
        }

    }

    release(&ptable.lock);
    return 0;

    found:
    p->state = EMBRYO;
    p->pid = nextpid++;
    p->thread_id = p->pid;
    p->wakeup_tick = 0;
    release(&ptable.lock);

    // Allocate kernel stack.
    if((p->kstack = alloc_page ()) == 0){
        p->state = UNUSED;
        return 0;
    }

    // allocate the usyscall page
    if((p->usyscall = (struct usyscall*)alloc_page()) == 0){
        // if alloc_page fails!
        free_page(p->kstack);
        p->kstack = 0;
        p->state = UNUSED;
        return 0;
    }

    sp = p->kstack + KSTACKSIZE;

    // Leave room for trap frame.
    sp -= sizeof (*p->tf);
    p->tf = (struct trapframe*)sp;

    // Set up new context to start executing at forkret,
    // which returns to trapret.
    sp -= 4;
    *(uint*)sp = (uint)trapret;

    sp -= 4;
    *(uint*)sp = (uint)p->kstack + KSTACKSIZE;

    sp -= sizeof (*p->context);
    p->context = (struct context*)sp;
    memset(p->context, 0, sizeof(*p->context));

    // skip the push {fp, lr} instruction in the prologue of forkret.
    // This is different from x86, in which the harderware pushes return
    // address before executing the callee. In ARM, return address is
    // loaded into the lr register, and push to the stack by the callee
    // (if and when necessary). We need to skip that instruction and let
    // it use our implementation.
    p->context->lr = (uint)forkret+4;

    // set the number of syscalls
    p->nsyscalls = 0;
    // set the number of tickets for this process
    p->tickets = 1;
    // intially the process will have no boosts
    p->boosts = 0;
    // initially the process has not consumed any ticks
    p->runticks = 0;

    // p->pg_queue_head = 0;
    // p->pg_queue_tail = 0;
    // memset(p->pg_queue, -1, sizeof(p->pg_queue));

    return p;
}

void error_init ()
{
    panic ("failed to craft first process\n");
}


//PAGEBREAK: 32
// hand-craft the first user process. We link initcode.S into the kernel
// as a binary, the linker will generate __binary_initcode_start/_size
void userinit(void)
{
    struct proc *p;
    extern char _binary_initcode_start[], _binary_initcode_size[];

    p = allocproc();
    initproc = p;

    if((p->pgdir = kpt_alloc()) == NULL) {
        panic("userinit: out of memory?");
    }

    inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);

    p->sz = PTE_SZ;
    // set the number of syscalls
    p->nsyscalls = 0;

    // map the usyscall page and set the PID. AP_KUR gives user read-only access.
    // cprintf("HERE\n");
    mappages(p->pgdir, (void*)USYSCALL, PTE_SZ, v2p(p->usyscall), AP_KUR);
    p->usyscall->pid = p->pid;
    // cprintf("HERE Done\n");

    // craft the trapframe as if
    memset(p->tf, 0, sizeof(*p->tf));

    p->tf->r14_svc = (uint)error_init;

    p->tf->spsr = spsr_usr ();
    p->tf->sp_usr = PTE_SZ;	// set the user stack
    p->tf->lr_usr = 0;

    // set the user pc. The actual pc loaded into r15_usr is in
    // p->tf, the trapframe.
    p->tf->pc = 0;					// beginning of initcode.S

    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");

    p->state = RUNNABLE;
    p->is_main_thread = 1;
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
    uint sz;

    sz = proc->sz;

    if(n > 0){
        if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0) {
            return -1;
        }

    } else if(n < 0){
        if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0) {
            return -1;
        }
    }

    proc->sz = sz;
    switchuvm(proc);

    return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int fork(void)
{
    int i, pid;
    struct proc *np;

    // Allocate process.
    if((np = allocproc()) == 0) {
        return -1;
    }

    // Copy process state from p.
    if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
        free_page(np->kstack);
        np->kstack = 0;
        np->state = UNUSED;
        return -1;
    }

    np->sz = proc->sz;
    np->parent = proc;
    *np->tf = *proc->tf;
    // set the number of syscalls
    np->nsyscalls = 0;
    // set the number of pickets of the child to be the same as the parent
    np->tickets = proc->tickets;

    // map the usyscall page for the new process and set the PID.
    mappages(np->pgdir, (void*)USYSCALL, PTE_SZ, v2p(np->usyscall), AP_KUR);
    np->usyscall->pid = np->pid;

    // np->pg_queue_head = proc->pg_queue_head;
    // np->pg_queue_tail = proc->pg_queue_tail;
    
    // for (int i = np->pg_queue_head; i < np->pg_queue_tail; i++) {
    //     np->pg_queue[i] = proc->pg_queue[i];
    // }

    // Clear r0 so that fork returns 0 in the child.
    np->tf->r0 = 0;

    for(i = 0; i < NOFILE; i++) {
        if(proc->ofile[i]) {
            np->ofile[i] = filedup(proc->ofile[i]);
        }
    }

    np->cwd = idup(proc->cwd);

    pid = np->pid;
    np->state = RUNNABLE;
    safestrcpy(np->name, proc->name, sizeof(proc->name));
    np->is_main_thread = 1;

    return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void exit(int exit_code)
{
    struct proc *p;
    int fd;

    if(proc == initproc) {
        panic("init exiting");
    }

    if (!proc->is_main_thread)
        panic("WTF why is a thread calling exit this is not allowed");

    // Close all open files.
    for(fd = 0; fd < NOFILE; fd++){
        if(proc->ofile[fd]){
            fileclose(proc->ofile[fd]);
            proc->ofile[fd] = 0;
        }
    }

    iput(proc->cwd);
    proc->cwd = 0;

    acquire(&ptable.lock);

    // Parent might be sleeping in wait().
    wakeup1(proc->parent);

    // Pass abandoned children to init.
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->parent == proc && p->is_main_thread) { // only pass on the forked processes not threads
            p->parent = initproc;

            if(p->state == ZOMBIE) {
                wakeup1(initproc);
            }
        } else if (p->parent == proc) {
            // TODO: kill him but nicely
            p->killed = 1;

            if (p->state == SLEEPING) 
                p->state = RUNNABLE;

            // but who will cleanup?
            panic("No one to cleanup the body!!");
        }
    }

    // Jump into the scheduler, never to return.
    proc->state = ZOMBIE;
    sched();

    panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(void)
{
    struct proc *p;
    int havekids, pid;

    acquire(&ptable.lock);

    for(;;){
        // Scan through table looking for zombie children.
        havekids = 0;

        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if(p->parent != proc) {
                continue;
            }

            havekids = 1;

            if(p->state == ZOMBIE) {
                // Found one.
                pid = p->pid;
                free_page(p->kstack);
                p->kstack = 0;
                p->usyscall = 0;
                if (p->is_main_thread) { // only let the main thread clear the memory
                    // cprintf("Cleanup %d\n", p->pid);
                    freevm(p->pgdir);
                }
                p->state = UNUSED;
                p->pid = 0;
                p->thread_id = 0;
                p->nsyscalls = 0;
                p->parent = 0;
                p->name[0] = 0;
                p->killed = 0;
                p->wakeup_tick = 0;
                release(&ptable.lock);

                return pid;
            }
        }

        // No point waiting if we don't have any children.
        if(!havekids || proc->killed){
            release(&ptable.lock);
            return -1;
        }

        // Wait for children to exit.  (See wakeup1 call in proc_exit.)
        sleep(proc, &ptable.lock);  //DOC: wait-sleep
    }
}

int get_total_tickets() // returns the total number of tickets to be used during the lottery including artificially boosted tickets
{
    int total = 0;
    for (struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
        if (p->state != RUNNABLE) continue;
        total += ((p->boosts ? 2 : 1) * p->tickets);
    }
    return total;
}

// returns a pointer to the lottery winner.
struct proc *hold_lottery(int total_tickets) {
    // total_tickets here includes the tickets obtained from artificial boosting during the lottery.
    if (total_tickets <= 0) {
        cprintf("this function should only be called when at least 1 process is RUNNABLE");
        return 0;
    }

    uint random_number = rand();    // This number is between 0->4 billion
    uint left = random_number % total_tickets + 1; // we pick the process that makes this number <= 0 when iterating through all the processes.

    struct proc* winner;
    int winner_found = -1;

    for (struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
        if (p->state != RUNNABLE) continue;
        if (p->boosts){
            p->boosts--;
            if (2 * p->tickets >= left && winner_found == -1) 
            {
                    winner = p; 
                    winner_found = 1;
            }
            left -= 2 * p->tickets;
        }
        else{
            if (p->tickets >= left && winner_found == -1)
            {
                    winner = p; 
                    winner_found = 1;
            }
            left -= p->tickets;
        }
    }
    return winner;
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void scheduler(void)
{
    struct proc *p;

    for(;;){
        // Enable interrupts on this processor.
        sti();

        // Loop over process table looking for process to run.
        acquire(&ptable.lock);
        // lottery sched
        int total_tickets_during_lottery = get_total_tickets();
        if (total_tickets_during_lottery == 0) goto next;

        p = hold_lottery(total_tickets_during_lottery);
        proc = p;
        switchuvm(p);
        p->state = RUNNING;
        p->runticks += 1; // this process has been scheduled for one more tick

        swtch(&cpu->scheduler, proc->context);
        // Process is done running for now.
        // It should have changed its p->state before coming back
        proc = 0;
        next:
        release(&ptable.lock);
    }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void sched(void)
{
    int intena;

    //show_callstk ("sched");

    if(!holding(&ptable.lock)) {
        panic("sched ptable.lock");
    }

    if(cpu->ncli != 1) {
        panic("sched locks");
    }

    if(proc->state == RUNNING) {
        panic("sched running");
    }

    if(int_enabled ()) {
        panic("sched interruptible");
    }

    intena = cpu->intena;
    swtch(&proc->context, cpu->scheduler);
    cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
    acquire(&ptable.lock);  //DOC: yieldlock
    proc->state = RUNNABLE;
    sched();
    release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void forkret(void)
{
    static int first = 1;

    // Still holding ptable.lock from scheduler.
    release(&ptable.lock);

    if (first) {
        // Some initialization functions must be run in the context
        // of a regular process (e.g., they call sleep), and thus cannot
        // be run from main().
        first = 0;
        initlog();
    }

    // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
    //show_callstk("sleep");

    if(proc == 0) {
        panic("sleep");
    }

    if(lk == 0) {
        panic("sleep without lk");
    }

    // Must acquire ptable.lock in order to change p->state and then call
    // sched. Once we hold ptable.lock, we can be guaranteed that we won't
    // miss any wakeup (wakeup runs with ptable.lock locked), so it's okay
    // to release lk.
    if(lk != &ptable.lock){  //DOC: sleeplock0
        acquire(&ptable.lock);  //DOC: sleeplock1
        release(lk);
    }

    // Go to sleep.
    proc->chan = chan;
    proc->state = SLEEPING;
    sched();

    // Tidy up.
    proc->chan = 0;

    // Reacquire original lock.
    if(lk != &ptable.lock){  //DOC: sleeplock2
        release(&ptable.lock);
        acquire(lk);
    }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan. The ptable lock must be held.
static void wakeup1(void *chan)
{
    struct proc *p;

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if(p->state == SLEEPING && p->chan == chan) {
            // dor debugging / visualizing
            // if (chan == &ticks){
            //     cprintf("%s (%d) wakeup\n",p->name, p->pid);
            // }
            // p->state = RUNNABLE;
            if (p->wakeup_tick == 0 || ticks >= p->wakeup_tick){
                // if (chan == &ticks){
                //     cprintf("%s (%d) wakeup\n",p->name, p->pid);
                // }
                p->state = RUNNABLE;
            }
        }
    }
}

static void wakeup2(void *chan)
{
    struct proc *p;

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if(p->state == SLEEPING && p->chan == chan) {
            // dor debugging / visualizing
            // if (chan == &ticks){
            //     cprintf("%s (%d) wakeup\n",p->name, p->pid);
            // }
            // p->state = RUNNABLE;
            if (p->wakeup_tick == 0 || ticks >= p->wakeup_tick){
                // if (chan == &ticks){
                //     cprintf("%s (%d) wakeup\n",p->name, p->pid);
                // }
                p->state = RUNNABLE;
                break;
            }
        }
    }
}

// Wake up all processes sleeping on chan.
void wakeup(void *chan)
{
    acquire(&ptable.lock);
    wakeup1(chan);
    release(&ptable.lock);
}

void wakeup_one(void * chan)
{
    acquire(&ptable.lock);
    wakeup2(chan);
    release(&ptable.lock);
}

// Kill the process with the given pid. Process won't exit until it returns
// to user space (see trap in trap.c).
int kill(int pid)
{
    struct proc *p;

    acquire(&ptable.lock);

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->pid == pid){
            p->killed = 1;

            // Wake process from sleep if necessary.
            if(p->state == SLEEPING) {
                p->state = RUNNABLE;
            }

            release(&ptable.lock);
            return 0;
        }
    }

    release(&ptable.lock);
    return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging. Runs when user
// types ^P on console. No lock to avoid wedging a stuck machine further.
void procdump(void)
{
    static char *states[] = {
            [UNUSED]    "unused",
            [EMBRYO]    "embryo",
            [SLEEPING]  "sleep ",
            [RUNNABLE]  "runble",
            [RUNNING]   "run   ",
            [ZOMBIE]    "zombie"
    };

    struct proc *p;
    char *state;

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state == UNUSED) {
            continue;
        }

        if(p->state >= 0 && p->state < NELEM(states) && states[p->state]) {
            state = states[p->state];
        } else {
            state = "???";
        }

        cprintf("%d %s %s\n", p->pid, state, p->name);
        cprintf(" --------- Pages/Page Table Entries -------- \n");
        for (int i = 0; i < (1 << 8); i++) {
            if (p->pgdir[i] == 0)
                continue;
            cprintf("PDE %d: %x, Pointer to this entry: %x\n", i, p->pgdir[i], &p->pgdir[i]);
            if (p->pgdir[i] != 0) {
                pte_t *pte = (pte_t*) p2v(PT_ADDR(p->pgdir[i]));
                // this should point to the pgtable
                for (int j = 0; j < (1 << 8); j++) {
                    if (pte[j] == 0)
                        continue;
                    cprintf("    PTE %d: %x, Pointer to this entry: %x\n", j, (pte[j]), &pte[j]);
                }
            }
        }
    }

    show_callstk("procdump: \n");
}

int getpid(struct proc* process) {
    return process->is_main_thread ? process->pid : getpid(process->parent);
}

struct proc* get_process(int pid) {
    struct proc *p;

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->pid == pid) {
            return p;
        }
    }

    return 0; // return 0 if the process is not found
}

int get_nsyscall(int pid) {
    acquire(&ptable.lock);
    int res = get_process(pid)->nsyscalls;
    release(&ptable.lock);
    return res;
}

int get_proclist(int* buffer){
    acquire(&ptable.lock);
    for (int i = 0; i < NPROC; i ++)
    {
       struct proc* current = &ptable.proc[i];

       if (current->state == UNUSED){
            *(buffer + i) = -1; 
        }
       else {
            *(buffer + i) = current->pid;
        }
    } 
    release(&ptable.lock);
    return 0;
}

int get_parproc(int pid){
    acquire(&ptable.lock);
    int res = get_process(pid)->parent->pid;
    release(&ptable.lock);
    return res;
}

int get_procname(int pid, char* buffer){
    acquire(&ptable.lock);
    safestrcpy(buffer, get_process(pid)->name, NPROC);
    release(&ptable.lock);
    return 0;
}

int get_procstate(int pid) {
    acquire(&ptable.lock);
    int res = get_process(pid)->state;
    release(&ptable.lock);
    return res;
}

int get_pinfo(struct pstat *pstat) {
    struct proc* p;

    acquire(&ptable.lock);
    for (int i = 0; i < NPROC; i++) {
        p = &ptable.proc[i];
        pstat->inuse[i] = (p->state == UNUSED) ? 0 : 1;
        pstat->pid[i] = p->pid;
        pstat->tickets[i] = p->tickets;
        pstat->boostsleft[i] = p->boosts;
        pstat->runticks[i] = p->runticks;
    }
    release(&ptable.lock);

    return 0;
}

void srand(uint seed) {
    rseed = seed;
}

int settickets(int pid, int n_tickets) {
    acquire(&ptable.lock);
    struct proc *p = get_process(pid);
    if (p == 0 || n_tickets < 0) 
        return -1;
    p->tickets = n_tickets;
    release(&ptable.lock);

    return 0;
}

void print_stack(char* mem) {
    for (int i = 0; i < PTE_SZ; i++) {
        cprintf("%p: %d\n", &mem);
        mem++;
    }
}

int thread_create(int* tid_ptr, char* func_ptr, char* args) {
    int i, pid;
    struct proc *np;
    {
        char *sp;

        acquire(&ptable.lock);

        for(np = ptable.proc; np < &ptable.proc[NPROC]; np++) {
            if(np->state == UNUSED) {
                goto found;
            }

        }

        release(&ptable.lock);
        return 0;

        found:
        np->state = EMBRYO;
        np->pid = nextpid++;
        np->thread_id = np->pid;
        np->wakeup_tick = 0;
        release(&ptable.lock);

        // Allocate kernel stack.
        if((np->kstack = alloc_page ()) == 0){
            np->state = UNUSED;
            return -1;
        }

        np->usyscall = proc->usyscall;

        sp = np->kstack + KSTACKSIZE;

        // Leave room for trap frame.
        sp -= sizeof (*np->tf);
        np->tf = (struct trapframe*)sp;

        // Set up new context to start executing at forkret,
        // which returns to trapret.
        sp -= 4;
        *(uint*)sp = (uint)trapret;

        sp -= 4;
        *(uint*)sp = (uint)np->kstack + KSTACKSIZE;

        sp -= sizeof (*np->context);
        np->context = (struct context*)sp;
        memset(np->context, 0, sizeof(*np->context));

        // skip the push {fp, lr} instruction in the prologue of forkret.
        // This is different from x86, in which the harderware pushes return
        // address before executing the callee. In ARM, return address is
        // loaded into the lr register, and push to the stack by the callee
        // (if and when necessary). We need to skip that instruction and let
        // it use our implementation.
        np->context->lr = (uint)forkret+4;
    }
    // Copy process state from p.
    np->pgdir = proc->pgdir;

    np->sz = proc->sz;
    np->parent = proc; // this is setting the parent but cleanup twice?
    *np->tf = *proc->tf;
    // set the number of syscalls
    np->nsyscalls = 0;
    // set the number of pickets of the child to be the same as the parent
    np->tickets = proc->tickets;

    // Clear r0 so that fork returns 0 in the child.
    np->tf->r0 = 0;

    for(i = 0; i < NOFILE; i++) {
        if(proc->ofile[i]) {
            np->ofile[i] = proc->ofile[i];
        }
    }

    np->cwd = idup(proc->cwd);

    pid = np->pid;
    np->state = RUNNABLE;
    safestrcpy(np->name, proc->name, sizeof(proc->name));
    np->is_main_thread = 0;

    *tid_ptr = pid;

    // alloc 2 pages for the user stack follow the same method, one is empty to detect stack overflow and the one below that is the stack
    int sz = np->sz;
    uint sp;
    // cprintf("Original Size: %d\n", sz);

    sz = align_up (sz, PTE_SZ);
    // cprintf("Alignup Size: %d\n", sz);

    if ((sz = allocuvm(np->pgdir, sz, sz + 2 * PTE_SZ)) == 0) {
        goto bad;
    }

    clearpteu(np->pgdir, (char*) (sz - 2 * PTE_SZ));
    // print_stack((char *)align_up(proc->tf->sp_usr, PTE_SZ));
    // print_stack((char *)align_up(np->tf->sp_usr, PTE_SZ));
    sp = sz;
    // cprintf("Final Size: %d\n", sz);

    np->sz = sz;
    proc->sz = sz;
    np->killed = 0;

    // pgdump1(np->pgdir, 1);
    // pgdump1(proc->pgdir, 1);
    np->tf->pc = (uint) func_ptr;
    np->tf->sp_usr = sp;
    np->tf->r0 = (uint) args; // TODO: find a better way put the data on the stack and call that instead of doing this.
    
    return pid;

    bad:
    // clear the alloted uvm
    // deallocuvm(np->pgdir, sz + 2 * PTE_SZ, sz);
    panic("Thread creation failed");
    return -1;
}

int thread_exit() { // note that some one should call join to cleanup this guy
    // clean up thread
    if (proc->is_main_thread)
        return 0; // should be noop
    
    struct proc *p;

    iput(proc->cwd);
    proc->cwd = 0;

    acquire(&ptable.lock);

    // TODO: Correct this
    // Parent might be sleeping in wait().
    wakeup1(proc->parent);

    // Pass abandoned children to init.
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->parent == proc && p->is_main_thread) { // only pass on the forked processes not threads
            p->parent = initproc;

            if(p->state == ZOMBIE) {
                wakeup1(initproc);
            }
        } else if (p->parent == proc) {
            // Thread can't kill right?
            // or should it kill
            // TODO: Figure this out for now I have kept it that we will kill all the subthreads also
            p->killed = 1;

            if (p->state == SLEEPING) 
                p->state = RUNNABLE;
        }
    }

    // Jump into the scheduler, never to return.
    proc->state = ZOMBIE;
    sched();

    panic("zombie exit");
    return -1;
}

int thread_join(uint tid) {
    // TODO: Fix edge cases like joining the same thread
    acquire(&ptable.lock);
    struct proc* thread = get_process(tid);

    if (thread == 0) 
        panic("You are waiting for someone that doesn't exist");
        
    if (thread->parent != proc) {
        panic("You don't have ownership of this thread");
    }

    while (1) {
        if (thread->state == ZOMBIE) {
            // uint sp = thread->tf->sp_usr;
            free_page(thread->kstack);
            thread->kstack = 0;
            thread->usyscall = 0;
            // TODO: delete the page table entries for this stack
            // TODO: Free stack

            thread->state = UNUSED;
            thread->pid = 0;
            thread->thread_id = 0;
            thread->nsyscalls = 0;
            thread->parent = 0;
            thread->name[0] = 0;
            thread->killed = 0;
            thread->wakeup_tick = 0;
            release(&ptable.lock);
            return 0;
        }
        
        if(proc->killed){
            release(&ptable.lock);
            return -1;
        }
        
        sleep(proc, &ptable.lock);
    }

    release(&ptable.lock);
    return -1;
}
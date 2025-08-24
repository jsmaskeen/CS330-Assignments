Assignment 2



---

[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/jcUVsoav)


In this assignment, we will modify xv6 to schedule processes using lottery scheduling. Further, the scheduler will boost the priority of sleeping processes so they get their fair share of CPU time. 

# Assignment 2

## 1) Boosted Lottery Scheduler Requirements

The current xv6 scheduler implements a simple Round Robin (RR) policy. For example, if there are three processes A, B and C, then the round-robin scheduler will run the jobs in the order `A B C A B C` … The xv6 scheduler runs each process for at most one timer tick (10 ms); after each timer tick, the scheduler moves the previous job to the end of the ready queue and dispatches the next job in the list. The xv6 scheduler does not do anything special when a process sleeps or blocks (and is unable to be scheduled); the blocked job is simply skipped until it is ready and it is again its turn in the RR order. 

Lottery Scheduling is a proportional share policy which works by asking users to grant tickets to processes. The scheduler holds a lottery every tick to determine the next process. Since important processes are given more tickets, they end up running more often on the CPU over a period of time.

You will implement a variant of lottery scheduler with the following properties:

1. At each tick, your scheduler holds a lottery between runnable processes and schedules the winner.
2. If a process is blocked and unable to participate in the lottery for x ticks, its tickets would be doubled for the next x lotteries.
3. For this assignment, you will need to improve the sleep/wakeup mechanism of xv6 so processes are unblocked only after their sleep interval has expired instead of every tick.

### 1.1. Basic Lottery Scheduler

In your scheduler, each process runs for an entire tick until interrupted by the xv6 timer. At each tick, the scheduler holds a lottery between RUNNABLE processes and schedules the winner. When a new process arrives, it should have the same number of tickets as its parent. The first process of xv6 should start with 1 ticket. You will also create a system call settickets(int pid, int tickets), to change the number of tickets held by the specified process.

As an example, if there is a process A with 1 ticket and process B with 4 tickets, this is what the scheduling might look like: `B B A B B B B B B A A B B B A B B B B B`.

In this case, A was scheduled for 4 ticks, and B for 16. Given sufficiently large number of ticks, we can expect the ratio of CPU time per process to be approximately the ratio of their tickets.

Scheduling is relatively easy when processes are just using the CPU; scheduling gets more interesting when jobs are arriving and exiting, or performing I/O. There are three events in xv6 that may cause a new process to be scheduled:

- whenever the xv6 10 ms timer tick occurs
- when a process exits
- when a process sleeps.

Your scheduler should also not trigger a new scheduling event when a new process arrives or wakes; you should simply mark the process as "RUNNABLE". The next scheduling decision will be made the next time a timer tick occurs.

### 1.2 Compensating processes for blocking

Many schedulers contain some type of incentive for processes with no useful work to sleep (or block) and voluntarily relinquish the CPU so another process can run; this improves overall system throughput since the process isn't wastefully holding on to the CPU doing nothing.  To provide this incentive, your scheduler will track the number of ticks a process was sleeping. Once it's runnable, it will boost (double) the number of tickets for the same number of ticks. For instance, if a process slept for 5 ticks, its tickets would be artificially doubled for the next 5 lotteries it participates in.

**Example 1:** a process A has 1 ticket and B has 4 tickets. Process A sleeps for 2 ticks, and is therefore given double tickets for the next 2 lotteries after it wakes. This allows the scheduler to keep the CPU usage ratio equal to the ratio of tickets, even when process A is blocked. The timeline could look like the following in this case (description below the timeline):

| timer tick       | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 |
|------------------|---|---|---|---|---|---|---|---|---|----|----|----|----|----|----|----|----|----|----|----|
| scheduled        | B | A | B | B | B | A | A | B | B | B  | B  | B  | B  | A  | B  | B  | B  | B  | B  | B  |
| sleeping/blocked |   | A | A |   |   |   |   |   |   |    |    |    |    |    | A  | A  |   |   |   |   |   |
| A lottery tickets| 1 | 1 | - | - | 2 | 2 | 1 | 1 | 1 | 1  | 1  | 1  | 1  | 1  | -  | -  | 2  | 2  | 1  | 1  |
| B lottery tickets| 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4  | 4  | 4  | 4  | 4  | 4  | 4  | 4  | 4  | 4  | 4  |

Note the following:

- At each tick, we consider the tickets a runnable process has. Ticks 3, 4, 15 and 16 only have 1 runnable process, thus only B participates in the lottery.
- Once A is no longer blocked, it starts participating in lotteries. If it slept for x ticks, it will participate in the next x lotteries with double the tickets.
- Make sure that when a process sleeps, it is given compensation ticks for the amount of time it was actually sleeping, not the amount of time it wasn't scheduled.  For example, process A becomes RUNNABLE at tick 17. You should boost its tickets only for the next 2 lotteries, regardless of whether A actually wins the lottery or not.

**Example 2:** Assume A sleeps for 3 ticks, thus its tickets are boosted for the next 3 ticks. What happens if it sleeps for 3 ticks again while it still has `remaining` boosts left? In this case, its tickets should be boosted for the next (3+`remaining`) rounds after it wakes. This is important for maintaining the lottery scheduler's proportional-CPU-share property. Given that A couldn't even participate in the lottery when it was blocked, we need to ensure it has a higher chance of winning in the corresponding number of lotteries.

This is a potential timeline for the described situation. Realize that after tick 5, A was supposed to be boosted for 3 ticks. Since it got blocked before that, we ensure that it doesn't lose the number of favourable/boosted rounds once it wakes.

| interval         | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 |
|------------------|---|---|---|---|---|---|---|---|---|----|----|----|----|----|----|----|----|----|----|----|
| scheduled        | B | A | B | B | B | A |   |   | B | A  | A  | B  | B  | B  | B  | B  | B  | A  | B  | B  |
| sleeping         |   | A | A | A | A |   | A | A |   |    |    |    |    |    |    |    |    |    |    |    |
| A tickets        | 1 | 1 | - | - | - | 2 | - | - | 2 | 2  | 2  | 2  | 2  | 1  | 1  | 1  | 1  | 1  | 1  | 1  |
| B tickets        | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4  | 4  | 4  | 4  | 4  | 4  | 4  | 4  | 4  | 4  | 4  |


**Example 3:** In many workloads, there will be multiple processes that are blocked.  The tickets of all of these processes will be independently doubled based on how long each slept. This isn't a special case; just ensure that you aren't assuming only one process is sleeping for a fixed amount of time. Different processes could be blocked for varying number of ticks.

**Example 4:** Assume process A has 5 tickets and 3 boosted ticks remaining:

- Its children will always inherit 5 tickets, whether or not process A was boosted at the time of child creation.
- Children processes will not inherit boostsleft from their parents.
- When running the lottery, assume that this process has double tickets(10) since it has boosts left(3).
- `getpinfo` should return tickets=5 and boostsleft=3. Do not return tickets=10 even when the process is boosted.


### 1.3 Improving the Sleep/Wake Mechanism

Finally, you need to change the existing implementation of `sleep()` syscall.  The sleep syscall allows processes to sleep for a specified number of ticks. Unfortunately, the current xv6 implementation of the `sleep()` syscall forces the sleeping process to wake on every timer tick to check if it has slept for the requested number of timer ticks or not.  This has two drawbacks.  First, it is inefficient to schedule every sleeping process and force it to perform this check.  Second, and more importantly for this project, if the process was scheduled on a timer tick (even just to perform this check), it will lose at least 1 favourable round since it ended up participating in the lottery.

Additional details about the xv6 code are given below.  You are required to fix this problem of extra wakeups by changing `wakeup1()` in `proc.c`.  You are likely to add additional condition checking to avoid falsely waking up the sleeping process until it is the right time.  You may want to add more fields to `struct proc` to help `wakeup1()` decide whether if it is time to wake a process.  You might also use this mechanism to count how many ticks a process was sleeping for so it can be boosted accordingly.

In order to hold a lottery, we first need a way to generate a random number. Here's the code to do so. Copy the following at the top of `proc.c` after the headers:

```c
#define RAND_MAX 0x7fffffff
uint rseed = 0;

uint rand() {
    return rseed = (rseed * 1103515245 + 12345) & RAND_MAX;
}
```

Now that we have a way to generate a random number, you have everything you need to implement a lottery scheduler. You need to do 2 things now: (a) hold a lottery among the RUNNABLE jobs, (b) modify the current RR policy to actually use your policy. Please place the lottery code in a separate function:

```c
// returns a pointer to the lottery winner.
struct proc *hold_lottery(int total_tickets) {
    if (total_tickets <= 0) {
        cprintf("this function should only be called when at least 1 process is RUNNABLE");
        return 0;
    }

    uint random_number = rand();    // This number is between 0->4 billion
    uint winner_ticket_number = ... // Ensure that it is less than total number of tickets.

    // pick the winning process from ptable.
    // return winner.
}
```

You should call hold_lottery when you're modifying the existing scheduler code.


## 2) New system calls

You’ll need to create 3 new system calls for this assignment.

1. `int settickets(int pid, int n_tickets)`.  This syscall is used to set the number of tickets alloted to a process. It returns 0 if the pid value is valid and n_tickets is positive. Else, it returns -1.
2. `void srand(uint seed)`: This sets the rseed variable you defined in proc.c. (Hint: in order to effectively mutate that variable in your sys_srand function, you will need to define `extern uint rseed`).
3. `int getpinfo(struct pstat *)`.   Because your scheduler is all in the kernel, you need to extract useful information for each process by creating this system call so as to better test whether your implementation works as expected. To be more specific, this system call returns 0 on success and -1 on failure (e.g., the argument is not a valid pointer).  If successful, some basic information about each running process will be returned.

You can decide if you want to update your `pstat` statistics whenever a change occurs, or if you have an equivalent copy of these statistics in `ptable` and want to copy out information from the `ptable` when `getpinfo()` is called.  

Create a new file called `pstat.h` with the following contents to return the data:
```c
#include "param.h"

struct pstat {
  int inuse[NPROC]; // whether this slot of the process table is in use (1 or 0)
  int pid[NPROC]; // PID of each process
  int tickets[NPROC];  // how many tickets does this process have?
  int runticks[NPROC];  // total number of timer ticks this process has been scheduled
  int boostsleft[NPROC]; // how many more ticks will this process be boosted?
};
```
Do not change the names of the fields in `pstat.h`. You may want to add a header guard to `pstat.h` and `param.h` to avoid redefinition. 

During a boost, you should not double the number of tickets you return in `getpinfo`. Instead, return the original number of tickets. 


## 3) xv6 Scheduling Details

Most of the code for the scheduler is localized and can be found in `proc.c`.  The header file `proc.h` describes the fields of an important structure, `struct proc` in the global `ptable`.  

You should first look at the routine `scheduler()` which is essentially looping forever.  After it grabs an exclusive lock over ptable, it then ensures that each of the RUNNABLE processes in the ptable is scheduled for a timer tick. Thus, the scheduler is implementing a simple Round Robin (RR) policy.  

A timer tick is about 10ms, and this timer tick is equivalent to a single iteration of the for loop over the ptable in the `scheduler()` code. This is based on the timer interrupt frequency setup in xv6. You should examine the relevant code for incrementing the value of ticks in `trap.c`.

When does the current scheduler make a scheduling decision? Basically, whenever a thread calls `sched()`. You'll see that `sched()` switches between the current context back to the context for `scheduler()`; the actual context switching code for `swtch()` is written in assembly and you can ignore those details.   

When does a thread call `sched()`?  You'll find three places: when a process exits, when a process sleeps, and during `yield()`. When a process exits and when a process sleeps are intuitive, but when is `yield()` called?  You'll want to look at `trap.c` to see how the timer interrupt is handled and control is given to scheduling code; you may or may not want to change the code in `trap()`. 

For sleep and wake-up, the current xv6 implementation of the `sleep()` syscall has the following control flow:

- First, it puts the process into a SLEEPING state and marks its wait channel (field `chan` in `struct proc`) as the address of the global variable `ticks`. This operation means, this process "waits on" a change of `ticks`. "Channel" here is a waiting mechanism that could be any address. When process A updates a data structure and expects that some other processes could be waiting on a change of this data structure, A can scan and check other SLEEPING processes' `chan`; if process B's `chan` holds the address of the data structure process A just updated, then A would wake B up.
- Every time a tick occurs and the global variable `ticks` is incremented (in `trap.c` when handling a timer interrupt), the interrupt handler scans through the `ptable` and wakes up every blocked process waiting on `ticks`.  This causes every sleeping process to get falsely scheduled.
- When each sleeping process is scheduled but finds it hasn't slept long enough, it puts itself back to sleep again (see the while-loop of `sys_sleep()`).

As mentioned previously, this implementation causes the process that invoked syscall `sleep()` to falsely wake inside the kernel on each tick, and as a result, the process will consume a boosted ticket (favourable round) and start participating in the lottery even when it can't do useful work. To address this problem, you should change `wakeup1()` in `proc.c` to have some additional condition checking to avoid falsely waking up the sleeping process (e.g. checking whether `chan == &ticks`, and whether it is the right time to wake up, etc). Feel free to add more fields to `struct proc` to help `wakeup1()` decides whether the processing it is going to wake up really needs to be wakened up at this moment.

---

# Original Readme : xv6-pi5 Documentation

## Overview
xv6-pi5 is a port of the MIT xv6 teaching operating system to the ARM architecture, with a focus on compatibility with the Raspberry Pi 5 platform. It provides a minimal Unix-like kernel, shell, file system, and basic user programs, serving as a hands-on resource for learning operating system fundamentals on ARM hardware.

## Repository Structure
| Path/Directory | Purpose |
|----------------|---------|
| `arm.c`, `arm.h` | ARM CPU initialization, context switching, MMU setup |
| `asm.S` | Assembly routines for low-level CPU and trap handling |
| `entry.S` | Kernel entry point and bootstrap code |
| `swtch.S` | Context switch (process switching) assembly routine |
| `trap_asm.S` | Trap and interrupt entry assembly |
| `mmu.h` | ARM MMU and paging definitions |
| `kernel.ld` | Linker script for ARM memory layout |
| `initcode.S` | Minimal user-mode program for system initialization |
| `device/` | Device drivers (UART, timer, interrupt controller, etc.) |
| `console.c` | Console (UART) driver and kernel I/O |
| `main.c`, `start.c` | Kernel initialization and main loop |
| `Makefile` | Build system configuration for ARM toolchain |
| `usr/` | User programs (e.g., sh, ls, cat, etc.) |
| `tools/` | Build utilities (e.g., mkfs for file system creation) |
| Other `.c`/`.h` | Core kernel subsystems (proc, vm, file system, etc.) |

## Getting Started

### Prerequisites
- **ARM GCC Toolchain**: `arm-none-eabi-gcc` (for ARMv6/ARMv7) or `aarch64-linux-gnu-gcc` (for ARMv8/AArch64, Pi 5).
- **QEMU**: For ARM system emulation and testing.
- **Make**: Standard build utility.

### Building xv6-pi5
1. **Clone the Repository**:
   ```bash
   git clone -b xv6-pi5 https://github.com/bobbysharma05/OS.git
   cd OS/src
   ```
2. **Build the Kernel and User Programs**:
   ```bash
   make clean
   make
   ```
3. **Run in QEMU**:
   ```bash
   qemu-system-arm -M versatilepb -m 128 -cpu arm1176 -nographic -kernel kernel.elf
   ```
   You should see the xv6 shell prompt: `$`

## Features
- **Minimal Unix-like Kernel**: Process management, virtual memory, system calls.
- **ARM Support**: All low-level CPU, trap, and MMU code adapted for ARM.
- **Shell and Userland**: Simple shell and standard Unix utilities (ls, cat, echo, etc.).
- **File System**: xv6-style file system with support for basic file operations.
- **UART Console**: Serial console for kernel and shell I/O.
- **QEMU Compatibility**: Easily testable in QEMU before deploying to hardware.

## Key ARM-Specific Components
- **CPU and MMU Initialization**: Implemented in `arm.c`, `arm.h`, `mmu.h`, and associated assembly files. Handles setting up the ARM page tables, enabling the MMU, and configuring CPU modes.
- **Trap and Interrupt Handling**: Assembly files (`asm.S`, `trap_asm.S`, `entry.S`) provide the trap vector and interrupt entry points. Kernel C code handles dispatch and processing.
- **UART/Console**: `console.c` and device drivers in `device/` configure and use the Raspberry Pi’s UART for boot and shell interaction.
- **Linker Script**: `kernel.ld` ensures the kernel is loaded at the correct physical address for ARM.

## Porting Notes
- **Architecture-Specific Files**: All files related to CPU initialization, assembly, MMU, and device drivers are ARM-specific and differ from the x86/RISC-V versions of xv6.
- **Build System**: The `Makefile` and build scripts are set up for ARM toolchains. Adjust toolchain paths if necessary for your environment.
- **Testing**: QEMU is used for initial bring-up. For real Raspberry Pi 5 hardware, further adaptation (especially for new peripherals) may be required.

## Usage Example
```bash
$ ls
.              1 1 512
..             1 1 512
cat            2 2 8620
echo           2 3 8340
grep           2 4 9528
init           2 5 8560
kill           2 6 8332
ln             2 7 8364
ls             2 8 9332
mkdir          2 9 8412
rm             2 10 8404
sh             2 11 13532
stressfs       2 12 8616
usertests      2 13 32956
wc             2 14 8904
zombie         2 15 8184
UNIX           2 16 7828
console        3 17 0
$
```
This demonstrates a successful boot, shell launch, and file system access.

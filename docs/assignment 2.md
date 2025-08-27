# Assignment 2

Authors:
- Aarsh Wankar
- Abhinav Khot
- Jaskirat Singh Maskeen
- Karan Sagar Gandhi

## 1.3 Improving the Sleep/Wake Mechanism

First job was to decipher how the sleep and wakeup work.
Here is our understanding of the same:

### Original `sleep`
1. when `sleep(n)` is called from usersapce, then that essentially passes to `sys_sleep()` in kernel space [ref](./../sysproc.c). It retrieves the number of ticks `n` the process has to sleep for. Then initial ticks, `ticks0` are saved, after locking the global `ticks`. 
2. The `while` loop in `sys_sleep()` runs, till `ticks` - `ticks0` < n (the elapsed ticks is less than n), and if the process is not yet killed, it calls `sleep(&ticks, &tickslock)` defined in [ref](./../proc.c). Here `&ticks` is the wait channel. locks are required to prevent race conditions.
3. The `sleep(&ticks, &tickslock)` changes the state from `RUNNING` to `SLEEPING`, and sets the `proc->chan = &ticks`. Then `sched()` is called to schedule the next process. Hence in this way we put the current process to sleep!

### Original `wakeup`
1. `wakeup` runs periodically (called from `isr_timer` in [ref](./../device/timer.c)) as `wakeup(&ticks);`, which after locking the `ptable`, it calls `wakeup1(&ticks)`.
2. `wakeup1(&ticks)` iterates through the process table, and checks if the process is in `SLEEPING` state and `proc->chan == &ticks`, then it sets the state to `RUNNING`.

### The issue:
So essentially, every `ticks` increment is a wakeup call. when the process, which is currently sleeping (and has more ticks remaning to sleep), wakes up, it goes back to  the `while` loop to check `ticks - ticks0 < n`, and again goes to sleep.

This behavior is indicated with trivial logging, in the following video:

<video width="640" controls>
  <source src="./media/original_sleep.mp4" type="video/mp4">
</video>

### Improved Process:
We add a new field to the `proc` struct, that is, `uint wakeup_tick;`. This will store the tick at which the intended process has to wake up.
We initialise this to `0` when we run `allocproc` or `wait` in [ref](./../proc.c).

`sys_sleep()` is modified in the following way:
```diff
 int sys_sleep(void)
 {
     int n;
     uint ticks0;
     if(argint(0, &n) < 0) {
         return -1;
     }
     acquire(&tickslock);
     ticks0 = ticks;
     
+    proc->wakeup_tick = ticks0 + n;

-    while(ticks - ticks0 < n){
+    while(ticks < proc->wakeup_tick){
         if(proc->killed){
             release(&tickslock);
             return -1;
         }
+     // cprintf("%s (%d) sleepcall\n",proc->name, proc->pid);
         sleep(&ticks, &tickslock);
     }
+    proc->wakeup_tick = 0;
     release(&tickslock);
     return 0;
 }
```

This doesn't really help much if we do not fix the wakeup1 implementation as well. The idea is to for wakeup1 to see if `ticks <= proc->wakeup_tick`, then only set the state to `RUNNING`.

Here is the modified `wakeup1()`


```diff
static void wakeup1(void *chan)
 {
     struct proc *p;

     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
         if(p->state == SLEEPING && p->chan == chan) {
-            p->state = RUNNABLE;
+            if (p->wakeup_tick == 0 || ticks >= p->wakeup_tick){
+                   if (chan == &ticks){
+                    cprintf("%s (%d) wakeup\n",p->name, p->pid);
+                }
+                p->state = RUNNABLE;
+               }
         }
     }
 }
```

With these temporary logging messages, we can see the running of this improved sleep mechanism in the following video:

<video width="640" controls>
  <source src="./media/improved_sleep.mp4" type="video/mp4">
</video>
To implement the uptime command, we use the sys_call for uptime.
It returns the time in ticks, [ref](./../sysproc.c), which reads the `tickslock`. Now this is initialised in `timer_init` [ref](./../device/timer.c), where a frequency is required, which is defined in [ref](./../param.h), which is `HZ`.

So we can return the uptime in seconds as well, by doing, 
$$
\text{uptime (s)} = \frac{\text{ticks}}{\text{HZ}}
$$

But to access the frequency, since it is in the kernel space, I will need to expose the frequency as a syscall.
So i define `sys_clockfreq` in [ref](./../sysproc.c), then add a call number in [ref](./../syscall.h), add it in [ref](./../syscall.c), update [ref](./../usr/usys.S), add `clockfreq` in [ref](./../usr/user.h).

Also I had to add the uptime.c file [ref](./../usr/uptime.c) to UPROGS in [ref](./../usr/Makefile).

Now I have access to the fucntion `clockfreq()` which will tell me the clock frequecny in HZ.

That is all for this command.

![](./media/uptime_1.png)



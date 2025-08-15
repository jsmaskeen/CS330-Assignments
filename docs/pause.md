I looked through the other commands, to find how to take in command line input.
Then I made a pause.c file [ref](./../usr/pause.c), and wrote the functionality.
Then I also added the program to UPROGS in [ref](./../usr/Makefile).

For the pause functionality, I first convert argument to interger using `atoi` (If no command line argument is there then write to stderr, or if `num_seconds < 0` then write to stderr.). Then get the number of ticks, using `clockfreq()` syscall defined while writing `uptime` command.
Then I invoke the syscall for `sleep(num_ticks)`. Once it completes it exits (`exit(0)`).

<video width="640" controls>
  <source src="./media/pause_1.mp4" type="video/mp4">
</video>

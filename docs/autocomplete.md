So to implement the autocomplete command we faced a lot of difficulties

failed attempts @aarsh write here pls! thx

So finally we have the following way:
The commands are commands which are defined in the Makefile inside usr [ref](./../usr/Makefile)
So, inside usr directory, we will have `_.*` as the command name. We will consider only those as commands for autocomplete.

We cannot read this at runtime everytime because this autocomplete logic runs inside keeyboard interrupt handler. We cant wait for disc operation during this otherwise it will panic.

So, we edit the main Makefile [ref](./../Makefile), to automatically generate a C source file, at `build/autocmds.c` everytime we comiple the OS.
We basically read the Makefile in usr dir, and extract out the list of commands starting with `_`.

This is how the resultant file looks like:

```c
const char *commands[] = {
    "cat",
    "echo",
    "ls",
    // ... other commands
    0
};
```

It ends with 0, so that `console.c` [ref](./../console.c) knows when the commands array ends. In commands.c there is `extern const char *commands[];` which will tell the linker to find the array.

So the final autocomplete logic is in the console.c file, [ref](./../console.c), there, `\t` triggers the console interrupt handler, then finds the word typed from the buffer, compares with the commands array, and if its unique, autocompletes otherwise it prints the list of commands and in the new line writes till what user had currently written.

PS: backspaces work!!!

<video width="640" controls>
  <source src="./media/autocomplete_1.mp4" type="video/mp4">
</video>

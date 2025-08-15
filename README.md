[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/h2nLSOuW)

---
TODO:

- [x] (2 pts.) "uptime" command to print how long has the system been running
- [x] (3 pts.) pause <no. of seconds>

    - You can put your code in user/pause.c. Look at some of the other programs in user/ (e.g., user/echo.c, user/grep.c, and user/rm.c) to see how command-line arguments are passed to a program.
    - Add your pause program to UPROGS in Makefile; once you've done that, make qemu will compile your program and you'll be able to run it from the xv6 shell.
    - If the user forgets to pass an argument, sleep should print an error message.
    - The command-line argument is passed as a string; you can convert it to an integer using atoi (see user/ulib.c).
    - Use the system call sleep to enforce this program.
    - See kernel/sysproc.c for the xv6 kernel code that implements the sleep system call (look for sys_sleep), user/user.h for the C definition of sleep callable from a user program, and user/usys.S for the assembler code that jumps from user code into the kernel for sleep.
    - pause's main should call exit(0) when it is done.

- [ ] (3 pts.) modify the shell to support tab completion, i.e., pressing the tab key should try to complete the command if it is unique and list the possible commands if they are not unique as per the string matching upto the text provided by the user
- [ ] (4 pts.) ps command that prints details of all processes running on the system

    - You may have to implement various system calls as part of this command
    - You should print the process-id, parent's process id, name of the process, state of the process, and an additional field which prints the number of system calls the process has invoked. 







---

# xv6-pi5 Documentation

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

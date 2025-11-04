#ifndef _USYSCALL_H
#define _USYSCALL_H

// defines the user-kernel interface for the shared page.

struct usyscall { // for shared syscall page!
  int pid;  
};


#define USYSCALL 0x0FFFF000 //   (UADDR_SZ - PTE_SZ)

#endif
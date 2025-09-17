## Task 1 : Page Dump and Super pages

## Implementation of `pdgump` and `kpgdump`

We implement two syscalls, `pgdump(int print_full)` and `kpgdump()`.

The `print_full` parameter if set to `1`, prints the full page table for the current process.
If it is set to `0`, then only the top 10 and the bottom 10 pages are printed.


<img src="https://i.ibb.co/M58ggqkV/image.png" alt="image" border="0">

How does it work ?

We traverse the pagetable of the current running process and print each page table entry (or the page directory entry incase it is a superpage). The printed lines include the virtual address, the physical address, the flags, and if the page is valid or not.

Kernel page dump works in a similar manner, only difference is that the kernel entries start from the address `KERNBASE`.

<img src="https://i.ibb.co/ycZVpLWD/image.png" alt="image" border="0">


## Implementing `ugetpid` syscall

In the starte xv6, user processes obtain their process ID (PID) via a system call (`getpid`). While functional, this requires a context switch into the kernel each time, which is expensive. 

To reduce this overhead, ugetpid is implemented, which allows processes to read their PID directly from a shared, read-only page mapped into their address space [But this is a read only page that the kernel maintains].

This page `struct usyscall` contains information that the user program can read without invoking a syscall. Currently, the only exposed field is the process ID (pid).

We add this field in the `proc` struct.

Here are the things which happens to make this work:

- In `allocproc()` we allocate a page for usyscall.
- In `userinit()` and `fork()` we map the page at USYSCALL and set pid.
- In `wait()` we free the page when process exits.
- We also modify `copyuvm()` to skip copying the parent's usyscall page, since the child gets its own fresh one.

We have a user level call in `ulib.c`, as follows:

```
int ugetpid(void) {
  struct usyscall *u = (struct usyscall *)USYSCALL;
  return u->pid;
}
```

The test file `usr/pgtable_test.c` has the test for this usyscall.

# Superpages in xv6
Superpages are a memory management feature that allows the operating system to manage larger blocks of memory more efficiently. Instead of using standard page sizes (typically 4KB), superpages use larger page sizes (such as 1MB or 2MB), which can reduce the overhead associated with managing many small pages.

The standard memory management in xv6 uses a two-level page table structure to map 4 KiB pages. For large processes, this results in large page tables and frequent lookups. Our implementation allows for a more efficient mapping by using a single Level 1 page directory entry (PDE) to map a contiguous 1 MiB virtual memory region to a 1 MiB physical memory region whenever a process requests a large, aligned block of memory via the sbrk() system call.

## Implementation Details
The main implementation changes were made in `vm.c`, `mmu.h`, and `buddy.c`.

1. **mmu.h**: We defined constants for superpage size, shift, and mask. We also added a macro to align addresses to superpage boundaries.
```c
#define SUPERPAGE_SIZE (1024 * 1024)
#define SUPERPAGE_SHIFT 20
#define SUPERPAGE_MASK (SUPERPAGE_SIZE - 1)
#define SUPERPAGE_ADDR(pde) align_dn(pde, SUPERPAGE_SIZE)
```

The new macro, `SUPERPAGE_ADDR(pde)`, was created to correctly extract the 20-bit physical base address from a superpage PDE, as the existing PTE_ADDR macro was only suitable for 4 KiB pages.

2. **Physical Allocator (buddy.c)**: The buddy allocator's maximum allocation order was increased to support requests for 1 MiB contiguous physical memory blocks.
The `MAX_ORD` was increased from 12 (4 KiB) to 20 (1 MiB).

3. **Virtual Memory Subsystem (vm.c)**
Many functions were modified to handle superpages:

* `allocuvm`: This function was modified to be opportunistic. When a process's memory is being grown, it checks if the current virtual address is 1 MiB-aligned and if the remaining allocation size is at least 1 MiB. If both are true, it allocates a 1 MiB physical block and maps it directly in the page directory using the KPDE_TYPE flag. Otherwise, it falls back to allocating regular 4 KiB pages.

* `deallocuvm & copyuvm`: These functions were updated to check if a page directory entry is a superpage. If so, they use the SUPERPAGE_ADDR macro and SUPERPAGE_SHIFT to free or copy the entire 1 MiB block in one operation, ensuring correctness and efficiency.

* `walkpgdir`: This function, which traverses the page table hierarchy, was modified to recognize superpage PDEs. Since a superpage entry is a direct mapping and does not point to a second-level page table, `walkpgdir` now correctly terminates its search and returns 0 when it encounters one.

* `uva2ka & copyout`

    * `uva2ka` (User Virtual Address to Kernel Address) checks if a given virtual address falls within a superpage or a regular page and calculates the exact final physical address by adding the correct offset.

    * `copyout` was updated to be aware of the underlying page size. It determines if a memory copy operation is happening within a 1 MiB or 4 KiB page and chunks the copy accordingly, preventing incorrect memory access at the boundaries.

## Testing
Run the `pgtable_test` to test this.

## Task 2 : On Demand Paging
We implement a minimal version of on-demand-paging. Usually on-demand paging involves moving pages to disk when memory runs out and a new page needs to be allocated for the process. Here we omit the part where we move pages to disk and raise page faults to find them while accessing them, instead we just evict them.

We make sure to not evict any important pages for processes, for example the page where the code segment is stored.

For demonstration purposes, we let any given process can have atmost `10k` pages in memory at a given time. 

We also add a two new flags to the page table entry:
* EVICTABLE (PTE_E) : Indicates that this page is non important and can be evicted to allocate another page
* VALID (PTE_V)     : To signal the OS whether the page exists, this is mainly used for clean up purposes.

The main workflow of our implementation is as follows:
* We maintian a global queue of allocated pages of size `32k`. Each queue  entry stores a pointer to the **page table entry** for that page.
* When a process tries to allocated more than the allowed number of pages, we find EVICTABLE page and add a new page in its place. When evicting we make sure to set the VALID flag for the old page table entry to FALSE and also add a page table entry for the new page.

We write tests to demonstrate that our technique actually works. In case our tests work, we should see the same physical address with different virtual addressses.

To run the test we have defined the user command `test_demand_paging`.

Below is the result of a run of the test.

<img width="641" height="409" alt="Screenshot from 2025-09-17 23-34-53" src="https://github.com/user-attachments/assets/e02e2438-acf1-40a4-a9b6-a28d1b6ad93f" />

--------------------------------------------------------------------------


<img width="641" height="409" alt="Screenshot from 2025-09-17 23-35-18" src="https://github.com/user-attachments/assets/6ac982a7-0452-4051-b0e3-9f214f9dd943" />

The above images show two different virtual addresses being mapped to the same physical address. Also notice that only one of them is valid since the previous one is evicted.

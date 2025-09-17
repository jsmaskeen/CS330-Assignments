We implement a minimal version of on-demand-paging. Usually on-demand paging involves moving pages to disk when memory runs out and a new page needs to be allocated for the process. Here we omit the part where we move pages to disk and raise page faults to find them while accessing them, instead we just evict them.

We make sure to not evict any important pages for the process, for example the page where the code segment is stored.

For demonstration purposes, we let any given process allocate atmost `20k` pages. 

We also add a two new flags to the page table entry:
    - EVICTABLE (PTE_E) : Indicates that this page is non important and can be evicted to allocate another page
    - VALID (PTE_V)     : To signal the OS whether the page exists, this is mainly used for clean up purposes.

The main workflow of our implementation is as follows:
    - We maintian a global queue of allocated pages of size `40k`. Each queue  entry stores a pointer to the **page table entry** for that page.
    - When a process tries to allocated more than the allowed number of pages, we find EVICTABLE page and add a new page in its place. When evicting we make sure to set the VALID flag for the old page table entry to FALSE and also add a page table entry for the new page.

We write tests to demonstrate that our technique actually works. In case our tests work, we should see the same physical address with different virtual addressses.

To run the test we have defined the user command `test_demand_paging`.

Below is the result of a run of the test.


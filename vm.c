#include "param.h"
#include "types.h"
#include "defs.h"
#include "arm.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "elf.h"
#include "usyscall.h"

extern char data[];  // defined by kernel.ld
pde_t *kpgdir;  // for use in scheduler()

#define PG_QUEUE_SZ 20000 // 128 mb / pg_size = 128 mb / 4 kb = 32000
#define MAX_PROC_PAGES 20
// Xv6 can only allocate memory in 4KB blocks. This is fine
// for x86. ARM's page table and page directory (for 28-bit
// user address) have a size of 1KB. kpt_alloc/free is used
// as a wrapper to support allocating page tables during boot
// (use the initial kernel map, and during runtime, use buddy
// memory allocator. 
struct run {
    struct run *next;
};

struct {
    struct spinlock lock;
    struct run *freelist;
} kpt_mem;

struct {
    int pg_queue_head;           // position where to add the next element in the queue
    int pg_queue_tail;           // position where to add the next element in the queue
    char* pg_queue[PG_QUEUE_SZ];   // the queue which holds the next page to evict
} page_queue;

void init_vmm (void)
{
    initlock(&kpt_mem.lock, "vm");
    kpt_mem.freelist = NULL;
    page_queue.pg_queue_head = 0;
    page_queue.pg_queue_tail = 0;
}

static void _kpt_free (char *v)
{
    struct run *r;

    r = (struct run*) v;
    r->next = kpt_mem.freelist;
    kpt_mem.freelist = r;
}

static void kpt_free (char *v)
{
    if (v >= (char*)P2V(INIT_KERNMAP)) {
        kfree(v, PT_ORDER);
        return;
    }
    
    acquire(&kpt_mem.lock);
    _kpt_free (v);
    release(&kpt_mem.lock);
}

// add some memory used for page tables (initialization code)
void kpt_freerange (uint32 low, uint32 hi)
{
    while (low < hi) {
        _kpt_free ((char*)low);
        low += PT_SZ;
    }
}

void* kpt_alloc (void)
{
    struct run *r;
    
    acquire(&kpt_mem.lock);
    
    if ((r = kpt_mem.freelist) != NULL ) {
        kpt_mem.freelist = r->next;
    }

    release(&kpt_mem.lock);

    // Allocate a PT page if no inital pages is available
    if ((r == NULL) && ((r = kmalloc (PT_ORDER)) == NULL)) {
        panic("oom: kpt_alloc");
    }

    memset(r, 0, PT_SZ);
    return (char*) r;
}

void push_pg_queue(char* pg_no) {
    if ((page_queue.pg_queue_tail + 1) % PG_QUEUE_SZ == page_queue.pg_queue_head) {
        panic("page queue size is full\n");
        return;
    }
    // cprintf("Entry pushed to the queue: %p, PID: %d\n", pg_no, proc->pid);
    page_queue.pg_queue[page_queue.pg_queue_tail] = pg_no;
    page_queue.pg_queue_tail++;
    page_queue.pg_queue_tail %= PG_QUEUE_SZ;
}

void pop_pg_queue() {
    if (page_queue.pg_queue_head == page_queue.pg_queue_tail) {
        panic("page queue is empty nothing to remove\n");
        return;
    }
    page_queue.pg_queue_head++;
    page_queue.pg_queue_head %= PG_QUEUE_SZ;
}

char* pg_queue_front() {
    if (page_queue.pg_queue_head == page_queue.pg_queue_tail) {
        panic("no element in front of the page queue\n");
    }
    return page_queue.pg_queue[page_queue.pg_queue_head];
}

// Return the address of the PTE in page directory that corresponds to
// virtual address va.  If alloc!=0, create any required page table pages.
static pte_t* walkpgdir (pde_t *pgdir, const void *va, int alloc)
{
    pde_t *pde;
    pte_t *pgtab;

    // pgdir points to the page directory, get the page direcotry entry (pde)
    pde = &pgdir[PDE_IDX(va)];

    if (*pde & PE_TYPES) {
        pgtab = (pte_t*) p2v(PT_ADDR(*pde));

    } else {
        if (!alloc || (pgtab = (pte_t*) kpt_alloc()) == 0) {
            return 0;
        }

        // Make sure all those PTE_P bits are zero.
        memset(pgtab, 0, PT_SZ);

        // The permissions here are overly generous, but they can
        // be further restricted by the permissions in the page table
        // entries, if necessary.
        *pde = v2p(pgtab) | UPDE_TYPE;
    }

    return &pgtab[PTE_IDX(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
int mappages (pde_t *pgdir, void *va, uint size, uint pa, int ap)
{
    char *a, *last;
    pte_t *pte;

    a = (char*) align_dn(va, PTE_SZ);
    last = (char*) align_dn((uint)va + size - 1, PTE_SZ);

    for (;;) {
        if ((pte = walkpgdir(pgdir, a, 1)) == 0) {
            return -1;
        }

        if (*pte & PE_TYPES) {
            panic("remap");
        }
        *pte = pa | ((ap & 0x3) << 4) | PE_CACHE | PE_BUF | PTE_TYPE | PTE_E | PTE_V; // set the correct flags

        if (a == last) {
            break;
        }

        a += PTE_SZ;
        pa += PTE_SZ;
    }

    return 0;
}

// flush all TLB
static void flush_tlb (void)
{
    uint val = 0;
    asm("MCR p15, 0, %[r], c8, c7, 0" : :[r]"r" (val):);

    // invalid entire data and instruction cache
    asm ("MCR p15,0,%[r],c7,c10,0": :[r]"r" (val):);
    asm ("MCR p15,0,%[r],c7,c11,0": :[r]"r" (val):);
}

// Switch to the user page table (TTBR0)
void switchuvm (struct proc *p)
{
    uint val;

    pushcli();

    if (p->pgdir == 0) {
        panic("switchuvm: no pgdir");
    }

    val = (uint) V2P(p->pgdir) | 0x00;

    asm("MCR p15, 0, %[v], c2, c0, 0": :[v]"r" (val):);
    flush_tlb();

    popcli();
}

// Load the initcode into address 0 of pgdir. sz must be less than a page.
void inituvm (pde_t *pgdir, char *init, uint sz)
{
    char *mem;

    if (sz >= PTE_SZ) {
        panic("inituvm: more than a page");
    }

    mem = alloc_page();
    memset(mem, 0, PTE_SZ);
    // cprintf("mem in inituvm: %p\n", mem);
    mappages(pgdir, 0, PTE_SZ, v2p(mem), AP_KU);
    memmove(mem, init, sz);
    // cprintf("Pointer to the pte: %x\n", walkpgdir(pgdir, 0, 0));
    push_pg_queue((char *) walkpgdir(pgdir, 0, 0));
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int loaduvm (pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
{
    uint i, pa, n;
    pte_t *pte;

    if ((uint) addr % PTE_SZ != 0) {
        panic("loaduvm: addr must be page aligned");
    }

    for (i = 0; i < sz; i += PTE_SZ) {
        // cprintf("Address for the code is: %p\n", addr + i);
        if ((pte = walkpgdir(pgdir, addr + i, 0)) == 0) {
            panic("loaduvm: address should exist");
        }
        // cprintf("PTE ptr: %p\n", p2v(PTE_ADDR(*pte)));

        *pte = (*pte & ~PTE_E); // don't evict the code page!!
        pa = PTE_ADDR(*pte);

        if (sz - i < PTE_SZ) {
            n = sz - i;
        } else {
            n = PTE_SZ;
        }

        if (readi(ip, p2v(pa), offset + i, n) != n) {
            return -1;
        }
    }

    return 0;
}

// evicts the first page from the current process
int evict_page(pde_t *pgdir) {
    // flush_tlb();
    pte_t *pte;
    uint pa;

    while (1) {
        // cprintf("Trying to evict\n");
        char* current_page_addr = pg_queue_front();
        pop_pg_queue();
        pte = (pte_t *) current_page_addr;
        if (pte == 0 || *pte == 0 || (*pte & PTE_E) == 0 || (*pte & PTE_V) == 0)
            continue;
        pa = PTE_ADDR(*pte);
        if (pa == 0) {
            panic("Something went wrong");
        }
        free_page(p2v(pa));
        *pte = (*pte & ~PTE_V); // no longer valid
        // cprintf("Evict sucess");
        break;
    }
    flush_tlb();
    
    return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int allocuvm (pde_t *pgdir, uint oldsz, uint newsz)
{
    char *mem;
    uint a;

    if (newsz >= UADDR_SZ) {
        return 0;
    }

    if (newsz < oldsz) {
        return oldsz;
    }

    a = align_up(oldsz, PTE_SZ);

    for (; a < newsz; a += PTE_SZ) {
        mem = alloc_page();

        // cprintf("Mem: %p\n", mem);
        if (mem == 0 || a >= MAX_PROC_PAGES * PTE_SZ) {
            // just dealloc the last page
            // cprintf("allocuvm out of memory\n"); // TODO: Fix this
            evict_page(pgdir); // this will dealloc the page also and make it invalid
            if (mem != 0) 
                goto next;

            mem = alloc_page();
            if (mem == 0) {
                panic("Still no memory, wtf is going on?\n");
                return -1;
            }
            // deallocuvm(pgdir, newsz, oldsz);
            // cprintf("Mem: %p, newsz: %p, oldz: %p\n", mem, newsz, oldsz);
            // return -1;
        }
    next:;
        memset(mem, 0, PTE_SZ);
        mappages(pgdir, (char*) a, PTE_SZ, v2p(mem), AP_KU);
        // cprintf("Pointer to the pte: %x\n", walkpgdir(pgdir, (void *)a, 0));
        push_pg_queue((char *) walkpgdir(pgdir, (void *)a, 0));
    }

    // cprintf("Trying to get the first element in the queue: %x\n", *(pte_t *)page_queue.pg_queue[1]);

    return newsz;
    // return min(10000, (newsz + PTE_SZ - 1) / PTE_SZ) * PTE_SZ;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int deallocuvm (pde_t *pgdir, uint oldsz, uint newsz)
{
    pte_t *pte;
    uint a;
    uint pa;

    if (newsz >= oldsz) {
        return oldsz;
    }

    for (a = align_up(newsz, PTE_SZ); a < oldsz; a += PTE_SZ) {
        pte = walkpgdir(pgdir, (char*) a, 0);
        if (!pte) {
            // pte == 0 --> no page table for this entry
            // round it up to the next page directory
            a = align_up (a, PDE_SZ);

        } else if ((*pte & PE_TYPES) != 0) {
            if ((*pte & PTE_V) == 0) { // don't evict the page if it is not valid
                *pte = 0;
                continue;
            }
            pa = PTE_ADDR(*pte);

            if (pa == 0) {
                panic("deallocuvm");
            }

            free_page(p2v(pa));
            *pte = 0;
        }
    }

    return newsz;
}

// Free a page table and all the physical memory pages
// in the user part.
void freevm (pde_t *pgdir)
{
    uint i;
    char *v;

    if (pgdir == 0) {
        panic("freevm: no pgdir");
    }

    // release the user space memroy, but not page tables
    deallocuvm(pgdir, UADDR_SZ, 0);

    // release the page tables
    for (i = 0; i < NUM_UPDE; i++) {
        if (pgdir[i] & PE_TYPES) {
            v = p2v(PT_ADDR(pgdir[i]));
            kpt_free(v);
        }
    }

    kpt_free((char*) pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible page beneath
// the user stack (to trap stack underflow).
void clearpteu (pde_t *pgdir, char *uva)
{
    pte_t *pte;

    pte = walkpgdir(pgdir, uva, 0);
    if (pte == 0) {
        panic("clearpteu");
    }

    // in ARM, we change the AP field (ap & 0x3) << 4)
    *pte = (*pte & ~(0x03 << 4)) | AP_KO << 4;
}

// Given a parent process's page table, create a copy
// of it for a child.
pde_t* copyuvm (pde_t *pgdir, uint sz)
{
    pde_t *d;
    pte_t *pte;
    uint pa, i, ap;
    char *mem;

    // allocate a new first level page directory
    d = kpt_alloc();
    if (d == NULL ) {
        return NULL ;
    }

    // copy the whole address space over (no COW)
    for (i = 0; i < sz; i += PTE_SZ) {
        if(i == USYSCALL){
            // Don't copy the parent's usyscall page cause child will get its own in fork().
            continue;
        }
        if ((pte = walkpgdir(pgdir, (void *) i, 0)) == 0) {
            panic("copyuvm: pte should exist");
        }

        if (!(*pte & PE_TYPES)) {
            panic("copyuvm: page not present");
        }

        pa = PTE_ADDR (*pte);
        ap = PTE_AP (*pte);

        if ((mem = alloc_page()) == 0) {
            goto bad;
        }

        memmove(mem, (char*) p2v(pa), PTE_SZ);

        if (mappages(d, (void*) i, PTE_SZ, v2p(mem), ap) < 0) {
            goto bad;
        }
    }
    return d;

bad: freevm(d);
    return 0;
}

//PAGEBREAK!
// Map user virtual address to kernel address.
char* uva2ka (pde_t *pgdir, char *uva)
{
    pte_t *pte;

    pte = walkpgdir(pgdir, uva, 0);

    // make sure it exists
    if ((*pte & PE_TYPES) == 0) {
        return 0;
    }

    // make sure it is a user page
    if (PTE_AP(*pte) != AP_KU) {
        return 0;
    }

    return (char*) p2v(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for user pages.
int copyout (pde_t *pgdir, uint va, void *p, uint len)
{
    char *buf, *pa0;
    uint n, va0;

    buf = (char*) p;

    while (len > 0) {
        va0 = align_dn(va, PTE_SZ);
        pa0 = uva2ka(pgdir, (char*) va0);

        if (pa0 == 0) {
            return -1;
        }

        n = PTE_SZ - (va - va0);

        if (n > len) {
            n = len;
        }

        memmove(pa0 + (va - va0), buf, n);

        len -= n;
        buf += n;
        va = va0 + PTE_SZ;
    }

    return 0;
}


// 1:1 map the memory [phy_low, phy_hi] in kernel. We need to
// use 2-level mapping for this block of memory. The rumor has
// it that ARMv6's small brain cannot handle the case that memory
// be mapped in both 1-level page table and 2-level page. For
// initial kernel, we use 1MB mapping, other memory needs to be
// mapped as 4KB pages
void paging_init (uint phy_low, uint phy_hi)
{
    mappages (P2V(&_kernel_pgtbl), P2V(phy_low), phy_hi - phy_low, phy_low, AP_KU);
    flush_tlb ();
}

void pgdump1(pde_t *pgdir)
{
    pte_t *pte;
    uint i;
    uint proc_size = proc->sz;
    cprintf("page_dump: starting for PID %d (size: 0x%x)\n", proc->pid, proc_size);
    cprintf("Top 10 pages:\n");
                //  proc_size // for full ptable
    for (i = 0; i < proc_size; i += PTE_SZ) {
        pte = walkpgdir(pgdir, (void *)i, 0);
        // PTE is valid and present
        if (pte && (*pte & PE_TYPES)) {
            cprintf("va 0x%x, pa 0x%x, flags 0x%x, valid: %x\n", i, PTE_ADDR(*pte), *pte & 0xFFF, (int)((*pte & PTE_V) != 0));
        }
    }

    cprintf("Bottom 10 pages (without exceeding process size):\n");
    uint start_va;
    // start_va is the top of the bottom ten pages in the page tabel.
    if (proc_size > 10 * PTE_SZ) {
        start_va = proc_size - (10 * PTE_SZ);
    } else {
        start_va = 0;
    }

    // align start_va to nearest page boundary
    start_va = align_dn(start_va, PTE_SZ);

    for (i = start_va; i < proc_size; i += PTE_SZ) {
        pte = walkpgdir(pgdir, (void *)i, 0);
        if (pte && (*pte & PE_TYPES)) {
            cprintf("va 0x%x, pa 0x%x, flags 0x%x\n", i, PTE_ADDR(*pte), *pte & 0xFFF);
        }

    }
    cprintf("page_dump: OK\n");
}

void kpgdump1(void)
{
    cprintf("kernel_page_dump: starting\n");
    cprintf("kpgdir at P:0x%x, V:0x%x\n", V2P(kpgdir), kpgdir);

    // Kernel entries start at KERNBASE.
    for (uint i = PDE_IDX(KERNBASE); i < 4096; i++) {
        pde_t pde = kpgdir[i];

        // Valid PDE: type!=0
        // for the kernel, it should be KPDE_TYPE 
        if (pde & KPDE_TYPE) {
            uint va = i * PDE_SZ; // VA for this PDE
            cprintf("PDE[%d] va 0x%x pa 0x%x flags 0x%x\n",
                    i, va, (pde & ~0xFFF), (pde & 0xFFF));
        }
    }
    cprintf("kernel_page_dump: OK\n");
}
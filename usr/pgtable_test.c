#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"
#include "mmu.h"

#define N (125 * (1 << 20)) + (1 << 19) // 8 MiB // max 125 MiB works fails otherwise
#define PGSIZE (1 << PTE_SHIFT)  // user pages are 4KB in size

void print_pt_test();
void ugetpid_test();
void print_kpt_test();
void superpg_test();
void err(char *why);

int main(int argc, char *argv[])
{
    printf(1, "page_table_test: starting\n\n");

    printf(1, "Initial Page Table\n");
    print_pt_test(); 

    printf(1, "\nAllocating 25 pages\n");
    int num_pages_to_alloc = 25;
    
    int size = num_pages_to_alloc * PGSIZE;
    char *mem = sbrk(size);
    if (mem == (char*)(-1)) {
        err("sbrk for 25 pages failed");
    }
    printf(1, "Success!\n");

    printf(1, "\nPage Table after allocating 25 pages\n");
    print_pt_test(); 

    sbrk(-size);
    printf(1, "Deallocated 25 pages\n");

    printf(1, "\nPage Table after deallocation\n");
    print_pt_test();

    printf(1, "\nOther tests in original test.c\n");
    ugetpid_test();
    superpg_test();
    print_kpt_test();
    printf(1, "\npage_table_test: all tests succeeded\n");
    exit(0);
}


char *testname = "";

void err(char *why)
{
    printf(2, "page_table_test: %s failed: %s, pid=%d\n", testname, why, getpid());
    exit(1);
}

void print_pt_test()
{
    testname = "print_pt_test";
    pgdump();
}

void ugetpid_test()
{
    int i;
    testname = "ugetpid_test";
    printf(1, "\nugetpid_test: starting\n");

    for (i = 0; i < 30; i++) {
        int pid = fork();
        if (pid < 0) {
            err("fork failed");
        }
        else if (pid == 0) {
          if (getpid() != ugetpid())
            err("missmatched PID");
          exit(0);
        } else {
          wait();
        }
    }
    printf(1, "ugetpid_test: OK\n");
}

void print_kpt_test()
{
    testname = "print_kpt_test";
    kpgdump();
}


// equivalent to the superpg_test() in original test.c
// try to allocate 8 MiB of data
void superpg_test()
{
    testname = "superpg_test";
    printf(1, "\nsuperpg_test: starting\n");

    char *mem = sbrk(N);
    if (mem == (char*)-1) {
        err("sbrk failed to allocate large memory");
    }
    // write test
    for (uint i = 0; i < N/2; i++) {
        mem[i] = (char)(i % 256);
        mem[N - 1 - i] = (char)(i % 256);
    }
    // read test
    for (uint i = 0; i < N/2; i++) {
        if (mem[i] != (char)(i % 256) || mem[N - 1 - i] != (char)(i % 256)) {
            err("memory content mismatch");
        }
    }

    sbrk(-N);

    printf(1, "superpg_test: OK\n");
}

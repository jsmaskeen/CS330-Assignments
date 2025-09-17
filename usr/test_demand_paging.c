#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"
#include "mmu.h"

#define N (8 * (1 << 20))       // 8 MiB
#define PGSIZE (1 << PTE_SHIFT) // user pages are 4KB in size

char *testname = "";

void print_pt_test()
{
    testname = "print_pt_test";
    pgdump(1);
}

void ugetpid_test();
void print_kpt_test();
void superpg_test();
void err(char *why);

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        printf(2, "Usage: test_demand_paging\n");
        exit(1);
    }

    printf(1, "test_demand_paging: starting\n\n");

    printf(1, "\nAllocating 25 pages\n");
    int num_pages_to_alloc = 25;

    int old_size = num_pages_to_alloc * PGSIZE;
    sbrk(old_size);

    printf(1, "Success!\n");

    printf(1, "\nPage Table after allocating 25 pages\n");
    print_pt_test();

    num_pages_to_alloc = 40000;
    int size = num_pages_to_alloc * PGSIZE;

    printf(1, "Size trying to alloc is: %d MB\n", (size / (1 << 20)));

    printf(1, "-----------------------------------\n\n\n\nAllocating %d pages\n", num_pages_to_alloc);

    sbrk(size);

    printf(1, "Allocated Successfully!\n Page Table after allocation:\n");
    print_pt_test();
    printf(1, "Test is done running now exiting\n");
    // sbrk(-size);
    // sbrk(-old_size);
    exit(0);
}
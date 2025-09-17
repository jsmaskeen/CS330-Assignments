#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"
#include "mmu.h"

#define N (8 * (1 << 20)) // 8 MiB 
#define PGSIZE (1 << PTE_SHIFT)  // user pages are 4KB in size

char* testname = "";

void print_pt_test()
{
    testname = "print_pt_test";
    pgdump();
}

void ugetpid_test();
void print_kpt_test();
void superpg_test();
void err(char *why);

int
main(int argc, char **argv)
{
    if (argc > 1){
        printf(2, "Usage: test_demand_paging\n");
        exit(1);
    }
    
    printf(1, "test_demand_paging: starting\n\n");

    printf(1, "\nAllocating 25 pages\n");
    int num_pages_to_alloc = 25;
    
    int size = num_pages_to_alloc * PGSIZE;
    sbrk(size);

    printf(1, "Success!\n");
    
    printf(1, "\nPage Table after allocating 25 pages\n");
    print_pt_test();
    
    num_pages_to_alloc = 3500;
    size = num_pages_to_alloc * PGSIZE;

    printf(1, "-----------------------------------\n\n\n\nAllocating %d pages\n", num_pages_to_alloc);
    
    sbrk(size);

    printf(1, "Allocated Successfully!\n Page Table after allocation:\n");
    print_pt_test();
 
    exit(0);
}

#include "types.h"
#include "user.h"

struct all{
    struct lock* l;
    int* x;
};

void* count(void* arg){
    struct all* curr = (struct all*) arg;
    for(int i=0; i<1000000; i++)
	{	
        acquireLock(curr->l);
		*(int*)(curr->x)= *(int*)(curr->x) + 1;
        releaseLock(curr->l);
	}
    thread_exit();
    return 0;
}

int main()
{
	// two threads increment the same counter 10000 times each
	uint tid1, tid2;
    struct all input;
    struct lock l;
    initiateLock(&l);
    int a;
    input.x = &a;
    input.l = &l;

    thread_create(&tid1, count, (void*)&input);
    thread_create(&tid2, count, (void*)&input);
    thread_join(tid1);
    thread_join(tid2);

    printf(1, "Final value of x: %d\n", a);
    exit(0);
}


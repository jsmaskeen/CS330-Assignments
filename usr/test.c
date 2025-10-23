#include "types.h"
#include "usr/user.h"

void* function(void* param) {
	// int a = 0;
	sleep(100);
	printf(1, "Here: %d\n", (int)param);
	printf(1, "Child PID: %d\n", (int)getpid());
	thread_exit(0);
	printf(1, "thread shouldn't print this\n");
	return NULL;
}

int main()
{
	int a = 10;
	int b = 20;

	pgdump(1);
	printf(1, "Address of a: %p and b: %p, and f: %p, and main: %p \n", &a, &b, &function, &main);

	uint tid;
	thread_create(&tid, &function, (void *) a);
	printf(1, "thread_id: %d\n", tid);

	thread_exit(0);
	thread_join(tid);
	
	printf(1, "Here2:\n");
	// sleep(1);
	printf(1, "Here2:\n");
	// sleep(100);

	// need to cleanup
	printf(1, "PID: %d\n", (int)getpid());

	exit(0);
}

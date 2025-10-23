#include "types.h"
#include "usr/user.h"

void* function(void* param) {
	// int a = 0;
	sleep(10);
	printf(1, "Here: %d\n", (int)param);
	exit(0);
}

int main()
{
	int a = 10;
	int b = 20;

	pgdump(1);
	printf(1, "Address of a: %p and b: %p, and f: %p, and main: %p \n", &a, &b, &function, &main);

	uint tid;
	thread_create(&tid, &function, (void *) a);

	sleep(100);

	exit(0);
}

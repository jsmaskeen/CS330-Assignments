#include "types.h"
#include "usr/user.h"

void* function() {
	// int a = 0;
	sleep(10);
	printf(1, "Here");
	exit(0);
}

int main()
{
	int a = 10;
	int b = 20;

	pgdump(1);
	printf(1, "Address of a: %p and b: %p, and f: %p, and main: %p \n", &a, &b, &function, &main);

	uint tid;
	int something;
	thread_create(&tid, &function, &something);
	sleep(100);

	exit(0);
}

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char **argv)
{
    // prints to the fd 1, i.e. stdout
	int* buf = (int *) malloc(sizeof(int) * 64);
	int status = proclist(buf);
    printf(1, "user space address: %p", buf);
	int i;

	for (i = 0; i <  64; i++) {
		printf(1, "%d\n", &buf[i]);
	}

	printf(1, "Status: %d", status);
    
    exit();
}

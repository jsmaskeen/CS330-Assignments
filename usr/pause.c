#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char **argv)
{
    if (argc < 2){
        printf(2, "Usage: pause <no. of seconds>\n");
        exit(1);
    }
    if (argv[1][0] == '-'){
        printf(2, "Number of seconds must be atleast 0.\n");
        exit(2);
    }
    int num_seconds = atoi(argv[1]);
    int num_ticks = num_seconds * clockfreq();
    printf(1, "Sleeping for %d seconds!\n", num_seconds);
    sleep(num_ticks);
    printf(1, "Done Sleeping.\n");
    exit(0);
}

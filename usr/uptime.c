#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char **argv)
{
    // prints to the fd 1, i.e. stdout
    int i = uptime();
    int hz = clockfreq();
    printf(1, "System uptime is: %d ticks (%d s)\n", i, i/hz);
    
    exit(0);
}

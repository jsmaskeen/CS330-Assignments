#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    printf(1,"In use: %d\n",get_inuse_blocks());
    exit(0);
}

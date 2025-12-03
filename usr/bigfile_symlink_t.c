#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "memlayout.h"

char buf[8192];

int main(int argc, char *argv[])
{
    int numblocks;

    if (argc != 2)
    {
        printf(1, "bigfile_t: need number of blocks, usage: bigfile_t <num_blocks>\n");
        exit(0);
    }

    numblocks = atoi(argv[1]);

    int i, fd, n;

    printf(1, "big files test\n");

    fd = open("big", O_CREATE | O_RDWR);
    if (fd < 0)
    {
        printf(1, "error: creat big failed!\n");
        exit(0);
    }

    // for(i = 0; i < MAXFILE; i++){
    for (i = 0; i < numblocks; i++)
    { // MAXFILE exceeds 1024 blocks !!! we need upper lim to be  > 11+128 to check for doubly indirect blocks.
        ((int *)buf)[0] = i;
        if (write(fd, buf, 512) != 512)
        {
            unlink("big");
            printf(1, "error: write big file failed\n", i);
            exit(0);
        }
    }
    printf(1, "INUSE BLOCKS: \n%d\n", get_inuse_blocks());
    close(fd);

    char* linkfile = "linkfile";

    symlink("big", linkfile);

    fd = open(linkfile, O_RDONLY);
    if (fd < 0)
    {
        unlink("linkfile");
        unlink("big");
        printf(1, "error: open big failed!\n");
        exit(0);
    }

    n = 0;
    for (;;)
    {
        i = read(fd, buf, 512);
        // printf(1,"\n3: %d %d\n",i,((int*)buf)[0]);
        if (i == 0)
        {
            // if(n == MAXFILE - 1){
            if (n == numblocks - 1)
            {
                unlink("linkfile");
                unlink("big");
                printf(1, "read only %d blocks from big", n);
                exit(0);
            }
            break;
        }
        else if (i != 512)
        {
            unlink("linkfile");
            unlink("big");
            printf(1, "read failed %d\n", i);
            exit(0);
        }

        // printf(1,"%d\n",((int*)buf)[0]);
        if (((int *)buf)[0] != n)
        {
            unlink("linkfile");
            unlink("big");
            printf(1, "Error: read content of block %d is %d\n",
                   n, ((int *)buf)[0]);
            exit(0);
        }
        n++;
    }
    close(fd);

    unlink(linkfile);

    if (unlink("big") < 0)
    {
        printf(1, "unlink big failed\n");
        exit(0);
    }
    printf(1, "big files ok\n");

    exit(0);
}

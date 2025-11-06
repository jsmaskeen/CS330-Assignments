#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

#define O_RDONLY        0x000
#define O_WRONLY        0x001
#define O_RDWR          0x002
#define O_CREATE        0x200

int
main(int argc, char *argv[])
{
    char *path = "myfile";
    int fd = open(path, O_CREATE | O_RDWR);
    if(fd < 0){
        printf(1, "open %s failed\n", path);
        exit(0);
    }

    char *msg = "Sayonara, -child killers\n";
    int len = strlen(msg);

    if(write(fd, msg, len + 1) != len + 1){
        printf(1, "write failed\n");
        close(fd);
        exit(0);
    }
    close(fd);

    fd = open(path, 0);
    if(fd < 0){
        printf(1, "open for read failed\n");
        exit(0);
    }

    char buf[128];
    int n = read(fd, buf, sizeof(buf));
    if(n < 0){
        printf(1, "read failed\n");
        close(fd);
        exit(0);
    }

    printf(1, "Read %d bytes directly using the path: %s", n, buf);
    close(fd);

    // we use the symlink now

    symlink(path, "abc");

    fd = open("abc", 0);
    if(fd < 0){
        printf(1, "open for read failed\n");
        exit(0);
    }

    n = read(fd, buf, sizeof(buf));
    if(n < 0){
        printf(1, "read failed\n");
        close(fd);
        exit(0);
    }

    printf(1, "Read %d bytes using symlink: %s", n, buf);
    close(fd);

    exit(0);
}
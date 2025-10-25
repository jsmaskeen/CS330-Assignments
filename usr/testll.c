#include "types.h"
#include "user.h"

struct lock l;
int counter = 0;

void* increment(void* arg) {
    for (int i = 0; i < 1000000; i++) {
        acquireLock(&l);
        counter++;
        releaseLock(&l);
    }
    thread_exit();
    return 0;
}

int main() {
    initiateLock(&l);
    
    uint tid;
    thread_create(&tid, increment, 0);
    
    for (int i = 0; i < 1000000; i++) {
        acquireLock(&l);
        counter++;
        releaseLock(&l);
    }
    
    thread_join(tid);
    printf(1, "Counter: %d (expected %d)\n", counter,2*1000000);
    exit(0);
}
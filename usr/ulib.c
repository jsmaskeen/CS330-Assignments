#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "usyscall.h"
#include "arm.h"

static inline int
xchg(volatile int *addr, int newval)
{
    int old;
    unsigned int tmp;

    do {
        __asm__ volatile(
                "ldrex %0, [%2]\n"
                "strex %1, %3, [%2]\n"
                : "=&r" (old), "=&r" (tmp)
                : "r" (addr), "r" (newval)
                : "cc", "memory"
                );
    } while (tmp != 0);

    return old;
}

char*
strcpy(char *s, char *t)
{
    char *os;
    
    os = s;
    while((*s++ = *t++) != 0)
        ;
    return os;
}

char*
strncpy(char *s, const char *t, int n)
{
  char *os;

  os = s;
  while(n-- > 0 && (*s++ = *t++) != 0)
    ;
  while(n-- > 0)
    *s++ = 0;
  return os;
}

int
strcmp(const char *p, const char *q)
{
    while(*p && *p == *q)
        p++, q++;
    return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
    int n;
    
    for(n = 0; s[n]; n++)
        ;
    return n;
}

void*
memset(void *dst, int v, uint n)
{
	uint8	*p;
	uint8	c;
	uint32	val;
	uint32	*p4;

	p   = dst;
	c   = v & 0xff;
	val = (c << 24) | (c << 16) | (c << 8) | c;

	// set bytes before whole uint32
	for (; (n > 0) && ((uint)p % 4); n--, p++){
		*p = c;
	}

	// set memory 4 bytes a time
	p4 = (uint*)p;

	for (; n >= 4; n -= 4, p4++) {
		*p4 = val;
	}

	// set leftover one byte a time
	p = (uint8*)p4;

	for (; n > 0; n--, p++) {
		*p = c;
	}

	return dst;
}

char*
strchr(const char *s, char c)
{
    for(; *s; s++)
        if(*s == c)
            return (char*)s;
    return 0;
}

char*
gets(char *buf, int max)
{
    int i, cc;
    char c;
    
    for(i=0; i+1 < max; ){
        cc = read(0, &c, 1);
        if(cc < 1)
            break;
        buf[i++] = c;
        if(c == '\n' || c == '\r' )
            break;
    }
    buf[i] = '\0';
    return buf;
}

int
stat(char *n, struct stat *st)
{
    int fd;
    int r;
    
    fd = open(n, O_RDONLY);
    if(fd < 0)
        return -1;
    r = fstat(fd, st);
    close(fd);
    return r;
}

int
atoi(const char *s)
{
    int n;
    
    n = 0;
    while('0' <= *s && *s <= '9')
        n = n*10 + *s++ - '0';
    return n;
}

void*
memmove(void *vdst, void *vsrc, int n)
{
    char *dst, *src;
    
    dst = vdst;
    src = vsrc;
    while(n-- > 0)
        *dst++ = *src++;
    return vdst;
}

int
ugetpid(void)
{
  struct usyscall *u = (struct usyscall *)USYSCALL;
  return u->pid;
  // return u->pid + 1; // @karan try uncommenting this see how pftable_test will fail at ugetpid
}

void initiateLock(struct lock* l) {
    l->isInitiated = 1;
    l->lockvar = 0;
}

void acquireLock(struct lock* l) {
    while(l -> isInitiated && xchg(&l->lockvar, 1) != 0) {
        // printf(1, "trying to acquire lock, val : %d \n", l->lockvar);
    }
    // printf(1, "acquired lock!");
}

void releaseLock(struct lock* l) {
    if (l -> isInitiated && l -> lockvar) xchg(&l->lockvar, 0); 
    // printf(1, "\n lock released, val : %d", l -> lockvar);
}

void initiateCondVar(struct condvar* cv) {
    int chan = getChannel();
    cv -> var = chan;
    cv -> isInitiated = 1;
}

void condWait(struct condvar* cv, struct lock* l) {
    if (cv -> isInitiated && l -> isInitiated)
    {
        // printf(1, "Condwait releasing lock");
        releaseLock(l);
        // printf(1, "Condwait released lock");
        sleepChan(cv -> var);

        // printf(1, "signal caught, reaquiring lock");
        acquireLock(l);
    }
}

void broadcast(struct condvar* cv) {
    if (cv -> isInitiated)
    {
        sigChan(cv -> var);
    }
}

void signal(struct condvar* cv) {
    if (cv -> isInitiated)
    {
        sigOneChan(cv -> var);
    }
}

void semInit(struct semaphore* s, int initVal) {
    initiateLock(&s->l);
    initiateCondVar(&s->cv);
    s->ctr = initVal;
    s->isInitiated = 1;
}

void semUp(struct semaphore* s) {
    acquireLock(&s->l);
    s->ctr ++;
    signal(&s->cv);
    releaseLock(&s->l);
}

void semDown(struct semaphore* s) {
    acquireLock(&s->l);
    s->ctr--;
    if (s->ctr < 0) condWait(&s->cv, &s->l);
    releaseLock(&s->l);
}

#include "../include/lwp.h"
#include <stdio.h>

#define NTHREADS 5

// int new_thread(void *arg){
// 	printf("Hello, this is the thread. Your word is %s\n", (char *)arg);
// 	return 123;
// }
//
// int main(){
// 	tid_t tid =  lwp_create(new_thread, "big mode");
// 	lwp_start();
// 	thread t = tid2thread(tid);
//
// 	return 0;
// }
/* Thread body: greet, yield once, then say goodbye */

static int
thread_fn(void *arg)
{
    int id = (int)(long)arg;
    printf("Greetings from Thread %d.  Yielding...\n", id);
    lwp_yield();  // give everyone else one turn
    printf("I (%d) am still alive.  Goodbye.\n", id);
    return 0;     // return → lwp_wrap will call lwp_exit(0)
}

int
main(void)
{
    int i, status;

    /* 1) Spawn N threads */
    for (i = 0; i < NTHREADS; i++) {
        if (lwp_create(thread_fn, (void *)(long)i) == NO_THREAD) {
            perror("lwp_create");
            return 1;
        }
    }

    /* 2) Start the LWP system (never returns until all threads exit) */
    lwp_start();

    /* 3) Reap each terminated thread (and drop its stack) */
    for (i = 0; i < NTHREADS; i++) {
        lwp_wait(&status);
    }

    printf("LWPs have ended.\n");
    return 0;
}

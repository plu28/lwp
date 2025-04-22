// light weight processes
#include "../include/lwp.h"

tid_t lwp_create(lwpfun func, void* arg) {}
void lwp_exit(int status) {}
tid_t lwp_gettid(void) {}
void lwp_yield(void) {}
void lwp_start(void) {}
tid_t lwp_wait(int* status) {}
void lwp_set_schedular(scheduler func) {}
scheduler lwp_get_scheduler(void) {}
thread tid2thread(tid_t tid) {}


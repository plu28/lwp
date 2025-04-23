// light weight processes
#include "../include/lwp.h"
#include "../include/rr.h"
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdlib.h>
// #define _GNU_SOURCE maybe include MAP_STACK in mmap if things are exploding running on server
#define DEFAULT_STACK_SIZE 8338608 // 8 MiB
#define BASE_LWP_SIZE 256

static tid_t tid_count = 1;

static thread* t_mem = NULL; // holds where the thread list is on the heap
static size_t t_len; // current size of thread list
static size_t t_cap; // maximum size of thread list
static struct scheduler curr_scheduler = {rr_init, rr_shutdown, rr_admit, rr_remove, rr_next, rr_qlen}; /* init or shutdown could be null. ours isn't */

static int initted = 0; // flag for seeing if our scheduler has been initted

static void lwp_wrap(lwpfun func, void* arg) {
	int rval = func(arg);
	lwp_exit(rval);
}

tid_t lwp_create(lwpfun func, void* arg) {

	if (!initted) {
		curr_scheduler.init();
		initted = 1;
	}

	// allocate thread data structure of not allocated already
	if (t_mem == NULL) {
		t_mem = (thread*)malloc(sizeof(thread) * BASE_LWP_SIZE);
		if (t_mem == NULL) {
			perror("malloc");
			return NO_THREAD;
		}
	}

	// check if thread data structure can hold the additional thread and realloc if it cant
	if (t_len == t_cap) {
		size_t new_t_cap = t_cap * 2; 
		thread* new_t_mem = realloc(t_mem, sizeof(thread) * new_t_cap);
		if (new_t_mem == NULL) {
			perror("realloc");
			return NO_THREAD;
		}
		t_cap = new_t_cap;
		t_mem = new_t_mem;
	}


	// allocate a stack for the thread using mmap
	long page_size = sysconf(_SC_PAGE_SIZE);
	if (page_size == -1) {
		perror("sysconf");
		return NO_THREAD;
	}

	size_t stack_size;
	struct rlimit rlim;
	int rlimit_ret = getrlimit(RLIMIT_STACK, &rlim);
	if (rlimit_ret == -1 | rlim.rlim_cur == RLIM_INFINITY) {
		// fall back to default size
		stack_size = DEFAULT_STACK_SIZE;
	} else {
		stack_size = rlim.rlim_cur;
	}

	// align stack size to page size
	if (stack_size % page_size != 0) {
		stack_size += page_size - (stack_size % page_size);
	}

	// TODO: Make sure memory allocated for the stack is also munmap'd at some point in the future (probably in exit or wait)
	void* lwp_stack = mmap(NULL, stack_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (lwp_stack == MAP_FAILED) {
		perror("mmap");
		return NO_THREAD;
	}

	// TODO: Configure reg_file properly for the new lwp
	
	unsigned long* top = (unsigned long*)lwp_stack + stack_size; // calculate top of stack
	top--; // top now points to the first addressable space 
	*top = 3; // some garbage value for the compiler
	*(top - 1) = (unsigned long)lwp_wrap;	
	
	rfile reg_file;
	reg_file.rbp = (unsigned long)(top - 2); 
	reg_file.rdi = (unsigned long)func;
	reg_file.rsi = (unsigned long)arg;
	reg_file.fxsave = FPU_INIT;

	// allocate thread on heap
	thread new_thread = (thread)malloc(sizeof(context));
	t_mem[t_len] = new_thread;

	new_thread->tid = tid_count++;
	new_thread->stack = (unsigned long*) lwp_stack;
	new_thread->stacksize = stack_size;
	new_thread->state = reg_file;
	new_thread->lwp_next = NULL; 
	if (t_len > 0) {
		new_thread->lwp_prev = t_mem[t_len - 1]; 
		new_thread->lwp_prev->lwp_next = new_thread;
	} else {
		new_thread->lwp_prev = NULL;
	}
	new_thread->sched_next = NULL;
	new_thread->sched_prev = NULL;

	// admit the thread into the scheduler
	curr_scheduler.admit(new_thread);

	return new_thread->tid;
}

void lwp_start(void) {
	// create a lwp out of the main thread
	thread new_thread = (thread)malloc(sizeof(context));
	t_mem[t_len] = new_thread;

	rfile reg_file;

	new_thread->tid = tid_count++;
	new_thread->stack = NULL; // main thread has its own stack
	new_thread->stacksize = 0;
	new_thread->state = reg_file;
	new_thread->lwp_next = NULL;
	if (t_len > 0) {
		new_thread->lwp_prev = t_mem[t_len - 1]; 
		new_thread->lwp_prev->lwp_next = new_thread;
	} else {
		new_thread->lwp_prev = NULL;
	}
	new_thread->sched_next = NULL;
	new_thread->sched_prev = NULL;

	// save the main threads state 
	swap_rfiles(&new_thread->state, NULL); 
	lwp_yield();
}

void lwp_exit(int status) {}
void lwp_yield(void) {

}
tid_t lwp_wait(int* status) {}

void lwp_set_schedular(scheduler func) {
	curr_scheduler = *func;
}

scheduler lwp_get_scheduler(void) {
	return &curr_scheduler;
}

tid_t lwp_gettid(void) {}
thread tid2thread(tid_t tid) {}

void swap_rfiles(rfile *old, rfile *new) {}


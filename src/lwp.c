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
static size_t t_len = 0; // current size of thread list
static size_t t_cap = BASE_LWP_SIZE; // maximum size of thread list

static thread* wait_list = NULL; // contains a list of any waiting threads waiting for a corresponding exit
static size_t wait_len = 0;

static thread* exit_list = NULL; // contains a list of any exited threads wiating for a corresponding wait
static size_t exit_len = 0;

static struct scheduler curr_scheduler = {rr_init, rr_shutdown, rr_admit, rr_remove, rr_next, rr_qlen}; /* init or shutdown could be null. ours isn't */
static thread curr_thread = NULL;

static int initted = 0; // flag for seeing if our scheduler has been initted

// NOTE: style guide for function documentation. delete once done documenting
/*
 * function - short descriptions
 *
 * @param parameter - short parameter description
 * @return return type - description of the return type
 *
 * */

/*
 * lwp_wrap - helper function from which thread functions are called from. necessary to guarantee return value is passed into lwp_exit
 *
 * @param lwpfun func - function being run by the thread
 * @return void* arg - argument to the function
 * */
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

	unsigned long* top = (unsigned long*)lwp_stack + (stack_size / (sizeof(unsigned long))); // calculate top of stack
	top--; // top now points to the first addressable space 
	*top = 3; // some garbage value for the compiler
	*(top - 1) = (unsigned long)lwp_wrap;	
	
	rfile reg_file;
	reg_file.rbp = (unsigned long)(top - 2); 
	reg_file.rdi = (unsigned long)func;
	reg_file.rsi = (unsigned long)arg;
	reg_file.fxsave = FPU_INIT;

	// allocate thread on heap
	thread main_thread = (thread)malloc(sizeof(context));
	t_mem[t_len] = main_thread;

	main_thread->tid = tid_count++;
	main_thread->stack = (unsigned long*) lwp_stack;
	main_thread->stacksize = stack_size;
	main_thread->state = reg_file;
	main_thread->lwp_next = NULL; 
	if (t_len > 0) {
		main_thread->lwp_prev = t_mem[t_len - 1]; 
		main_thread->lwp_prev->lwp_next = main_thread;
	} else {
		main_thread->lwp_prev = NULL;
	}
	main_thread->sched_next = NULL;
	main_thread->sched_prev = NULL;

	// admit the thread into the scheduler
	curr_scheduler.admit(main_thread);
	curr_thread = main_thread;

	return main_thread->tid;
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

tid_t lwp_wait(int* status) {
	if (exit_len == 0) {
		// no threads have exited. block
		curr_scheduler.remove(curr_thread);

		// add to the wait list
		wait_list[wait_len++] = curr_thread;

		if (t_len < 2) {
			// would block forever because there is no other thread to yield to
			return NO_THREAD;
		}
		lwp_yield(); // yield to the next thread
	} 
	// deallocate resources for the exited thread
}

void lwp_exit(int status) {
	if (wait_len == 0) {
		// no threads waiting
		exit_list[exit_len++] = curr_thread;

		// remove thread from the scheduler
		curr_scheduler.remove(curr_thread);
	}
}

void lwp_yield(void) {
	if (curr_thread == NULL) {
		perror("no current thread");
		return;
	}
	thread next = curr_scheduler.next();

	// if the scheduler says theres no next thread 
	// NOTE: Expect zombie threads if there are blocked threads
	if (next == NULL) {
		exit(curr_thread->status);
	}
	next = tid2thread(next->tid);
	thread temp = curr_thread;
	curr_thread = next;
	swap_rfiles(&temp->state, &next->state);
}


void lwp_set_scheduler(scheduler func) {
	struct scheduler round_robin = {rr_init, rr_shutdown, rr_admit, rr_remove, rr_next, rr_qlen};
	if (func == NULL) {
		curr_scheduler = round_robin;
		return;
	}

	// store a list of all the threads
	size_t temp_len = 0;
	thread* temp_mem = (thread*)malloc(sizeof(context) * t_len); // I think its fair to assume that you won't have more threads in the scheduler then you have in here
	
	thread temp;
	while ((temp = curr_scheduler.next()) != NULL) {
		temp_mem[temp_len++] = temp;
		curr_scheduler.remove(temp);
	}

	// check if we have to shut down the old scheduler
	if (curr_scheduler.shutdown != NULL) {
		curr_scheduler.shutdown();
	}

	// set the new scheduler
	curr_scheduler = *func;

	// check if we have to init the new scheduler
	if (curr_scheduler.init != NULL) {
		curr_scheduler.init();
	}

	// now, iterate over all the threads and admit them to the new 
	for (int i = 0; i < temp_len; i++) {
		curr_scheduler.admit(temp_mem[i]);
	}
}

scheduler lwp_get_scheduler(void) {
	return &curr_scheduler;
}

tid_t lwp_gettid(void) {
	if (curr_thread == NULL) {
		return NO_THREAD;
	}
	return curr_thread->tid;
}

thread tid2thread(tid_t tid) {
	// iterate over the thread list until a matching tid is found
	for (int i = 0; i < t_len; i++) {
		if (t_mem[i]->tid == tid) {
			return t_mem[i];
		}
	}
	return NULL;
}


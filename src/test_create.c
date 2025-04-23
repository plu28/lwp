#include "lwp.h"
#include <stdio.h>

int new_thread(void *arg){
	printf("Hello, this is the thread. Your word is %s\n", (char *)arg);
	return 123;
}

int main(){
	tid_t tid = lwp_create(new_thread, "big mode");

	if(tid == NO_THREAD){
		printf("Failed returning thread\n");
		return 1;
	}
	
	printf("Created thread with tid: %lu\n", tid);

	return 0;
}

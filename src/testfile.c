#include "../include/lwp.h"
#include <stdio.h>

int new_thread(void *arg){
	printf("Hello, this is the thread. Your word is %s\n", (char *)arg);
	return 123;
}

int main(){
	tid_t tid =  lwp_create(new_thread, "big mode");
	lwp_start();
	thread t = tid2thread(tid);

	return 0;
}

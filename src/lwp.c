#include "../include/lwp.h"
#include <stdlib.h>

typedef struct QNode {
	struct QNode* next;
	struct QNode* prev;
	thread tinfo;
} QNode;

static QNode* top;
static int qlen;


void init(void) {
	top = NULL;
}

void shutdown(void) {
}

void admit(thread new) {
	// first thread admitted
	if (top == NULL) {
		top->tinfo = new;
		top->next = top;
		top->prev = top;
	}

	// Create a QNode for the new thread
	QNode newq;
	newq.tinfo = new;
	newq.prev = top;

	// admit this guy to the back
	top->next->prev = &newq;
	newq.next = top->next;
	top->next = &newq;
}

void remove(thread victim) {

}

thread next(void) {
}

int qlen(void) {
}

// Round Robin scheduler
#include "../include/rr.h"
#include "../include/lwp.h"
#include <stdlib.h>
#include <stdio.h>
// NOTE: cant use stdio because remove is defined in stdio and we have our own
// remove defined. C does not support function overloading. #include <stdio.h>
// // required for perror

#define BASE_QUEUE_MEMBERS 64

// Queue data structure stored on heap
typedef struct QNode {
  struct QNode *next;
  struct QNode *prev;
  thread tinfo;
} QNode;

static QNode *top;    // holds the current node at the top of the queue
static size_t q_len;  // holds the length of the queue
static size_t q_cap;  // holds the current maximum capacity of the queue
static QNode **q_mem; // holds where the queue is stored on the heap

void rr_init(void) {
  top = NULL;
  q_cap = BASE_QUEUE_MEMBERS;
  q_len = 0;
}

// void print_Q() {
// 	QNode* curr = top;
// 	printf("top=%p total of %ld nodes\n", curr, q_len);
// 	curr = curr->prev;
// 	int i;
// 	for (i = 0; i < q_len; i++) {
// 		printf("curr->prev: %p\n", curr);
// 		curr = curr->prev;
// 	}
// }

void rr_shutdown(void) {
  // Free all memory in the data structure
  int i;
  for (i = 0; i < q_len; i++) {
    free(q_mem[i]);
  }
  free(q_mem);
}

void rr_admit(thread new) {

  // check if admitting the first thread
  if (top == NULL) {
    // initialize enough memory to start with
    q_mem = (QNode **)malloc(sizeof(QNode *) * q_cap);
    if (q_mem == NULL) {
      // perror("malloc");
      return;
    }

    top = (QNode *)malloc(sizeof(QNode));
    q_mem[0] = top;

    top->tinfo = new;
    top->next = top;
    top->prev = top;

    q_len++;
    return;
  }
  // Create a QNode for the new thread and add it into the data structure
  if (q_len == q_cap) {
    int new_cap = q_cap * 2; // doubling size of queue on each realloc
    QNode **new_mem = (QNode **)realloc(q_mem, sizeof(QNode *) * new_cap);
    if (new_mem == NULL) {
      // perror("realloc");
      return;
    }
    // only set these values if realloc succeeded
    q_cap = new_cap;
    q_mem = new_mem;
  }

  QNode *newq = (QNode *)malloc(sizeof(QNode));
  if (newq == NULL) {
    // perror("realloc");
    return;
  }
  q_mem[q_len] = newq;

  newq->tinfo = new;
  newq->prev = top;

  top->next->prev = newq;
  newq->next = top->next;
  top->next = newq;

  q_len++;
}

void rr_remove(thread victim) {
  // do a linear pass through the data structure
  int i;
  for (i = 0; i < q_len; i++) {
    QNode *curr_node = q_mem[i];
    if (curr_node->tinfo->tid == victim->tid) {
      // found thread to delete
      // adjust the linked list
      curr_node->next->prev = curr_node->prev;
      curr_node->prev->next = curr_node->next;
      curr_node->next = NULL;
      curr_node->prev = NULL;

      // adjust the arraylist
      free(curr_node);
      curr_node = NULL;
      int j;
      for (j = i; j < q_len; j++) {
        QNode *temp = curr_node;
        curr_node = q_mem[j + 1];
        q_mem[j + 1] = temp;
      }
      q_len--;
      return;
    }
  }
}

// get the next process in the queue
thread rr_next(void) {
	// print_Q();
  if (top == NULL) {
    return NULL;
  }
	thread temp = top->tinfo;
  top = top->prev;
  return temp;
}

int rr_qlen(void) { return q_len; }

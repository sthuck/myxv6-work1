
#include "types.h"
#include "defs.h"
#include "param.h"
struct queue{
	struct proc* arr[NPROC];
	int first;
	int last;
} ;
typedef struct queue queue;

int isEmpty(queue q);

void init_queue(queue myque);
struct proc* dequeue(queue q);
void enqueue(queue q,struct proc* proc);

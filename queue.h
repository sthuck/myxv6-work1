
#include "types.h"
#include "defs.h"
#include "param.h"
struct queue{
	struct proc* arr[NPROC];
	int first;
	int last;
} ;
typedef struct queue queue;

int isEmpty(queue* q);

void init_queue(queue* myque);
struct proc* dequeue(queue* q);
void enqueue(queue* q,struct proc* proc);

#if SCHED_FRR || SCHED_FCFS
extern queue ProcQue;
#endif

#if SCHED_3Q
extern queue ProcQueLow;
extern queue ProcQue;
extern queue ProcQueHigh;
extern queue* ProcQues[];
extern int numQue;
#endif
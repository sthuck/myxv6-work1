#include "queue.h"

void
init_queue(queue* myque) {
	myque->first=0;
	myque->last=0;
}

int 
isEmpty(queue* q) {
	return (q->first==q->last);
}


void enqueue(queue* q,struct proc* proc)
{
           q->arr[ q->last ] = proc;
           q->last = (q->last+1) % NPROC;
               
 
}

struct proc* dequeue(queue* q)
{
        struct proc* x;
        if (isEmpty(q)) 
       		return 0;

        else {
                x = q->arr[q->first ];
                q->first = (q->first+1) % NPROC;
        }

        return(x);
}


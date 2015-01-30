// DrawQueue function definitions
// Brandon Pollack
#include "DrawQueue.h"
#include "stdio.h" //for null pointer

void initQueue(DrawQueue* q) //initialize the queue
{
	q->head = q->tail = -1;
}
void clearQueue(DrawQueue* q) //clear the queue
{
	q->head = q->tail = -1;
}

int enqueue(DrawQueue* q, DrawableObject* obj) //put something in queue, 0 returns success, else fail
{
	if(isFull(q)) return FALSE;
	
	q->tail++;
	
	q->drawQueue[q->tail % QUEUE_MAX] = obj;
	return TRUE;
}


DrawableObject* dequeue(DrawQueue* q) //take from head of queue and return it, returns null if empty
{
	if(isEmpty(q)) return (void*) NULL;
	
	q->head++;
	
	return (q->drawQueue[q->head % QUEUE_MAX]);
}

int isEmpty(DrawQueue* q) //return true if empty
{
	if (q->head == q->tail) return TRUE;
	else return FALSE;
}

int isFull(DrawQueue* q) //retunr true if full
{
	return ((q->tail - QUEUE_MAX) == q->head);
}

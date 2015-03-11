// DrawQueue function definitions
// Brandon Pollack
#include "DrawQueue.h"
#include "stdio.h" //for null pointer

void initQueue(DrawQueue* q) //initialize the queue
{
	q->head = q->tail = -1;
	//TODO init all prio to 0
}
void clearQueue(DrawQueue* q) //clear the queue
{
	q->head = q->tail = -1;
}

uint32_t getSize(DrawQueue* q)
{
	return q->tail - q->head;
}
int enqueue(DrawQueue* q, DrawableObject* obj, uint8_t prio) //put something in queue, 0 returns success, else fail
{
	if(isFull(q)) return FALSE;
	
	obj->prio = prio;
	
	for(int i = q->head; i <= q->tail; ++i)
	{
		if(q->drawQueue[i]->prio < prio) //smaller numbers have lower priority, if we find one less, we found our spot
		{
			for(int j = q->tail; j > i; --j)
			{
				q->drawQueue[j+1 % QUEUE_MAX] = q->drawQueue[j % QUEUE_MAX]; //move everything over one
			}
			
			q->drawQueue[i+1] = obj;
			
			q->tail++;//one more added
			
			return TRUE;
		}
	}
		
	return FALSE; //shouldn't get here
}

int removeItem(DrawQueue* q, DrawableObject* obj)
{
	if(isEmpty(q)) return FALSE;
	
	for(int i = q->head; i <= q->tail; ++i)
	{
		if(q->drawQueue[i] == obj)
		{
			for(int j = i; j < q->tail; ++j)
			{
				q->drawQueue[j] = q->drawQueue[j+1];
			}
			
			q->tail--; //like we had one less thing
			
			return 0; //completed successfully
		}
	}
	
	return FALSE; //no such item in queue
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

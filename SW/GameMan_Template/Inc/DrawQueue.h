/* Queue Implementation using array nodes
** Brandon Pollack
** GameBro
*/
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef __QUEUE_H
#define __QUEUE_H

#include "DrawableObject.h"

#define QUEUE_MAX 255

typedef struct {DrawableObject* drawQueue[QUEUE_MAX]; int head,tail;} DrawQueue;

void initQueue(DrawQueue* q); //initialize the queue
void clearQueue(DrawQueue* q); //clear the queue
int enqueue(DrawQueue* q, DrawableObject* obj); //put something in queue, 0 returns success, else fail
DrawableObject* dequeue(DrawQueue* q); //take from head of queue and return it, returns null if empty
int isEmpty(DrawQueue* q); //return true if empty
int isFull(DrawQueue* q); //retunr true if full

#endif

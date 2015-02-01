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
#include "stdint.h"

#define QUEUE_MAX 255 //change head and tail type if bigger than 255

typedef struct {DrawableObject* drawQueue[QUEUE_MAX]; int8_t head,tail;} DrawQueue;

void initQueue(DrawQueue* q); //initialize the queue
void clearQueue(DrawQueue* q); //clear the queue
int enqueue(DrawQueue* q, DrawableObject* obj, uint8_t prio); //put something in queue, 0 returns success, else fail
int removeItem(DrawQueue* q, DrawableObject* obj); //remove an object (not pop)
DrawableObject* dequeue(DrawQueue* q); //take from head of queue and return it, returns null if empty
int isEmpty(DrawQueue* q); //return true if empty
int isFull(DrawQueue* q); //retunr true if full

#endif

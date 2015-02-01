/* Drawable Object structure definition
** Brandon Pollack
*/
#ifndef __DRAWABLE_OBJECT_H
#define __DRAWABLE_OBJECT_H
#include "stdint.h"

typedef struct
{
	void (*render)(void* obj); //render function pointer for a drawable object
	void* datas; //pointer to all logical game datas, including update function
	
	uint8_t prio; //draw priority (for queue insertion)
} DrawableObject;

#endif

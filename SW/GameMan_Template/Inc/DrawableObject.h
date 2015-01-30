/* Drawable Object structure definition
** Brandon Pollack
*/
#ifndef __DRAWABLE_OBJECT_H
#define __DRAWABLE_OBJECT_H

typedef struct
{
	void (*render)(); //render function pointer for a drawable object
	void* Datas; //pointer to all logical game datas
} DrawableObject;

#endif

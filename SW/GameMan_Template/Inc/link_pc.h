//Brandon Pollack
//Controllable Player character, link style
//Not so generic, this senior design project is supposed to be an example, so I am trying to keep it simple and get something done

#ifndef __LINK_PC_H
#define __LINK_PC_H

#include "stdint.h"
#include "DrawableObject.h"
#include "stdlib.h"
#include "TestGame.h"
#include "bitmap_tools.h"

//speed defines
#define LINK_MOVE_SPEED_INCH_PER_SECOND 1
#define LINK_MOVE_SPEED_PX_PER_SECOND LINK_MOVE_SPEED_INCH_PER_SECOND*PX_PER_INCH
#define LINK_MOVE_SPEED_PX_PER_FRAME LINK_MOVE_SPEED_PX_PER_SECOND/60
#define LINK_ANIMATION_FRAME_DURATION 400 //animation changes 2.5 times a second at 400 ms

//size defines
#define LINK_X 16
#define LINK_Y 16

//direction defines
#define LINK_DOWN 1
#define LINK_LEFT 2
#define LINK_UP 3
#define LINK_RIGHT 4 //count from 1 so we can use for math later

#define TRANSPARENT_COLOR 0xFFFD

//link struct
typedef struct
{
	uint16_t x_pos;
	uint8_t y_pos;
	
	uint8_t direction;
	
	uint8_t moving; //depending on this, resting link is different (l or r)
	
	uint16_t* sprite_set;
	
	uint8_t animation_frame; //keeps track of which of the 2 animations link could be (left step/rightstep/not moving)
} linkDObj;

DrawableObject* constructLink(uint16_t x_pos, uint8_t y_pos, uint8_t direction, uint16_t* sprite_set, RenderStyle mode);
void deconstructLink(DrawableObject* link);

uint16_t* loadLinkSpriteSet(void); //returns pointer to link sprite set, use different function to change images to load for link

void updateLink(void* dobj);
void renderLinkCPU(void* dobj);
void renderLinkDMA2D(void* dobj);
#endif
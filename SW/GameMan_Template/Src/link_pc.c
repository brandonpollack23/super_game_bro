//Brandon Pollack

#include "link_pc.h"

DrawableObject* constructLink(uint16_t x_pos, uint8_t y_pos, uint8_t direction, uint16_t* sprite_set, RenderStyle mode)
{
	DrawableObject* link = malloc(sizeof(DrawableObject));
	linkDObj* datas = malloc(sizeof(linkDObj));
	
	link->datas = datas;
	link->update = &updateLink;
	
	if(mode == CPU_RENDER)
	{
		link->render = &renderLinkCPU;
	}
	else
	{
		link->render = &renderLinkDMA2D;
	}
	
	datas->x_pos = x_pos;
	datas->y_pos = y_pos;
	datas->direction = direction;
	datas->sprite_set = sprite_set;
	datas->moving = 0;
	
	if(direction == LINK_LEFT || direction == LINK_UP)
	{
		datas->animation_frame = 1;
	}
	else
	{
		datas->animation_frame = 2; //neutral frame is different for different directions
	}
	
	return link;
}

void deconstructLink(DrawableObject* link)
{
	free(link->datas);
	free(link);
}

uint16_t* loadLinkSpriteSet(void)
{
	uint32_t error;
	uint16_t* sprite_set;
	
	sprite_set = (uint16_t*)simpleLoad("/sprites/link/link_d1_16.bmp", &loadImage, &error);
	simpleLoad("sprites/link/link_l1_16.bmp",&loadImage,&error);
	simpleLoad("sprites/link/link_u1_16.bmp",&loadImage,&error);
	simpleLoad("sprites/link/link_r1_16.bmp",&loadImage,&error);
	
	simpleLoad("/sprites/link/link_d2_16.bmp", &loadImage, &error);
	simpleLoad("sprites/link/link_l2_16.bmp",&loadImage,&error);
	simpleLoad("sprites/link/link_u2_16.bmp",&loadImage,&error);
	simpleLoad("sprites/link/link_r2_16.bmp",&loadImage,&error);
	
	return sprite_set;
}

void updateLink(void* dobj)
{
	linkDObj* link = ((linkDObj*) ((DrawableObject*) dobj)->datas);
	uint8_t new_y_pos;
	uint16_t new_x_pos;
	
	link->moving = 0;
	
	if(_BUTTON_PRESSED(BUTTON_UP)) //TODO add obstacle collision to all directions
	{
		link->moving = 1;
		if(link->direction != LINK_UP) //if not up before, switch to neutral stance and face up
		{
			link->direction = LINK_UP;
			link->animation_frame = 1;
		}
		
		new_y_pos = link->y_pos - LINK_MOVE_SPEED_PX_PER_FRAME;
		if(new_y_pos > 0) 
		{
			link->y_pos = new_y_pos;
		}
		else
		{
			link->y_pos = 0;
		}
	}
	else if(_BUTTON_PRESSED(BUTTON_DOWN))
	{
		link->moving = 1;
		if(link->direction != LINK_DOWN) //if not up before, switch to neutral stance and face up
		{
			link->direction = LINK_DOWN;
			link->animation_frame = 1; 
		}
		
		new_y_pos = link->y_pos + LINK_MOVE_SPEED_PX_PER_FRAME;
		if(new_y_pos < Y_RESOLUTION - LINK_Y)
		{
			link->y_pos = new_y_pos;
		}
		else
		{
			link->y_pos = Y_RESOLUTION - LINK_Y;
		}
	}
	else if(_BUTTON_PRESSED(BUTTON_LEFT))
	{
		link->moving = 1;
		if(link->direction != LINK_LEFT) //if not up before, switch to neutral stance and face up
		{
			link->direction = LINK_LEFT;
			link->animation_frame = 2; //default take a step first
		}
		
		new_x_pos = link->x_pos + LINK_MOVE_SPEED_PX_PER_FRAME;
		if(new_x_pos > 0)
		{
			link->x_pos = new_x_pos;
		}
		else
		{
			link->x_pos = 0;
		}
	}
	else if(_BUTTON_PRESSED(BUTTON_RIGHT))
	{
		link->moving = 1;
		if(link->direction != LINK_RIGHT) //if not up before, switch to neutral stance and face up
		{
			link->direction = LINK_RIGHT;
			link->animation_frame = 1; //default take step
		}
		
		new_x_pos = link->x_pos + LINK_MOVE_SPEED_PX_PER_FRAME;
		if(new_x_pos < X_RESOLUTION - LINK_X)
		{
			link->x_pos = new_x_pos;
		}
		else
		{
			link->x_pos = X_RESOLUTION - LINK_X;
		}
	}
	
	if(link->moving == 0)
	{
		if(link->direction == LINK_UP || link->direction == LINK_LEFT) //up/left not moving is 0
		{
			link->animation_frame = 0;
		}
		else //down right is 1
		{
			link->animation_frame = 1;
		}
	}
}

void renderLinkCPU(void* dobj)
{
	linkDObj* link = ((linkDObj*) ((DrawableObject*) dobj)->datas);
	uint16_t* sprite = link->sprite_set + LINK_X*LINK_Y*(link->direction)*(link->animation_frame); //animation frame is from link.bmp in resources, dlur and which frame
	
	for(int i = 0; i < LINK_Y; ++i)
	{
		for(int j = 0; i < LINK_X; ++j)
		{
			_FB_XY(framebuffer_0,(link->x_pos + j),(link->y_pos + i)) = *(sprite + j + i*LINK_X); //copy the sprite to the correct position in the FB
		}
	}
}

void renderLinkDMA2D(void* dobj)
{
	//TODO
}
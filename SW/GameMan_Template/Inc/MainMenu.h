// Main Menu, highlightable menu that selects between inputs
// CPU render first, DMA2D is a hassle with text (have to restart every char, current queue does one item at a time, but this is an item)
// DMA2D solution, while this was the item that caused DMA2D done int, on DMA2D interrupt queury this for next char and continue, if no more chars to print, continue with drawQueue
// My first DrawableObject, let's hope it doesnt explode!
// Brandon Pollack

#ifndef __MAIN_MENU_H
#define __MAIN_MENU_H
#include "DrawableObject.h"
#include "GameMan_RevA0.h"
#include "TestGame.h"
#include "stdlib.h"

#define MENU_UPDATE_SPEED 125

typedef struct
{
	uint16_t x_pos;
	uint8_t y_pos;
	
	char* header;
	char** options; //list array of options
	
	uint8_t highlighted_option; //defaults to 0
	uint8_t num_options; //number of options
	uint8_t option_selected; //Bool, means that we "clicked" an option, defaults to 0, checked by game to determine what to do next
	
	uint8_t currentChar; //which char we are drawing (DMA2D)
	uint8_t currentItem; //header is 0, options are 1-X (DMA2D)
	
	uint32_t lastActionTime; //time stamp of last thing to happen
	
	RenderStyle mode;
} menuDObj;

DrawableObject* constructMenu(uint16_t x_pos, uint8_t y_pos, char* header, char** options, uint8_t num_options, RenderStyle mode);
void deconstructMenu(DrawableObject* mainMenu);

void updateMenu(void* dobj); //updates menu based on inputs (down increments, up decrements highlighted_option mod number of options) and adds to drawqueue
void renderMenuCPU(void* dobj); //draws menu, highlighted option has blue background, menu is always drawn if it is alive
void renderMenuDMA2D(void* dobj);

#endif

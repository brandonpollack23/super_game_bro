// Main Menu, highlightable menu that selects between inputs
// CPU render first, DMA2D is a hassle with text (have to restart every char, current queue does one item at a time, but this is an item)
// DMA2D solution, while this was the item that caused DMA2D done int, on DMA2D interrupt queury this for next char and continue, if no more chars to print, continue with drawQueue
// My first DrawableObject, let's hope it doesnt explode!
// Brandon Pollack

#ifndef __MAIN_MENU_H
#define __MAIN_MENU_H
#include "DrawableObject.h"

extern const char menu_header[];
extern const char* menu_options[];

typedef struct
{
	uint16_t x_pos;
	uint8_t y_pos;
	
	char* header;
	char** options; //list array of options
	
	uint8_t highlighted_option; //defaults to 0
	uint8_t num_options; //number of options
	uint8_t option_selected; //defaults to 0, checked by game to determine what to do next
} menuDObj;

menuDObj createMenu(uint16_t x_pos, uint8_t y_pos,const char* header,const char** options);

void updateMenu(menuDObj* m); //updates menu based on inputs (down increments, up decrements highlighted_option mod number of options)
void renderMenu(menuDObj* m); //draws menu, highlighted option has blue background

#endif

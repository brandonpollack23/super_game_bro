// Test Game states/definitions
// Brandon Pollack
#ifndef __TESTGAME_H
#define __TESTGAME_H
//std includes
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"

//GameMan/DS Includes
#include "GameMan_RevA0.h"
#include "DrawQueue.h"

//Game Includes
#include "MainMenu.h"

typedef enum
{
	INIT,
	MENU,
	MARIO_TEST,
	SCROLLING_TEST,
	SPRITE_TEST,
	MP3_TEST
} GameState;

extern DrawQueue drawQueue; //queue of things to draw
extern DrawQueue drawableObjects; //all drawable objects, iterate through, update, readd (use special iterate with locally stored "head" so you don't actually remove from queue)

extern menuDObj* mainMenu; //the main menu

void runGame(void); //called from main
uint32_t updateObjects(void); //updates all active in game objects (in predefined array or malloc'd) and adds them to render queue if necessary
uint32_t updateGame(void); //updates game state based on certain in game objects

void writeToScreen(uint16_t x, uint8_t y, char* text); //function to write text to screen without gameobject

inline void swap(void); //if drawQueue is empty swap the framebuffer and tell the shadow registers to update at v_sync, update everything necessary (inputs, touch screen, etc)

#endif

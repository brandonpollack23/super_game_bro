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
#include "DrawableObject.h" //drawable objects
#include "MainMenu.h" //menu drawable object
#include "link_pc.h" //link game object

//game object type casts for easy access
#define MAIN_MENU ((menuDObj*)mainMenu->datas)
#define LINK ((linkDObj*)link->datas)

typedef enum //game state
{
	INIT,
	MENU,
	LINK_TEST,
	SCROLLING_TEST,
	SPRITE_TEST,
	MP3_TEST
} GameState;

typedef enum //game errors
{
	GAME_OK,
	ILLEGAL_GAME_STATE,
	NO_SUCH_MENU_OPTION
} GameError;

extern DrawQueue drawQueue; //queue of things to draw
extern DrawQueue drawableObjects; //all drawable objects, iterate through, update, readd (use special iterate with locally stored "head" so you don't actually remove from queue)

//Drawable Objects we need (sprites, bgs, menus, etc) we could devise a way to just add to queue and not need these references, but this is fine for the example
extern DrawableObject* mainMenu; //the main menu
extern DrawableObject* link;

/******Resources********/
extern uint8_t font12[];
#define FONT_HEIGHT 12 //each height is a byte, so 12 bytes per char
#define FONT_WIDTH 7

/***** Functions *******/

void runGame(void); //called from main

void switchToMainMenu(void);
void switchToLinkTest(void);

uint32_t updateObjects(void); //updates all active in game objects (in predefined array or malloc'd) and adds them to render queue if necessary
uint32_t updateGame(void); //updates game state based on certain in game objects
#endif

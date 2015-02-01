// Game Program and associated functions
// This contains the logical flow of the game logic and update of the objects within the game
// There should be no direct hardware access from the game functions in this file, it is all abstracted in TestGame_hw.c (swap, DMA2D stuff if necessary, etc)
// Brandon Pollack

#include "TestGame.h"

//Inputs
uint16_t buttonInput; //let the compiler reserve this location at the beginning so I can't fuck it up

//Game Globals
DrawQueue drawableObjects, drawQueue;
GameState gameState = INIT;
uint32_t delay_timestamp; //keeping track of time (delays) at the game logic level

//GameObjects
menuDObj* mainMenu;

void runGame(void)
{
	uint32_t game_err = 0;
	initQueue(&drawableObjects); //initialize the queue of drawable objects (starts with nothing in it)
	initQueue(&drawQueue);
	
	//TODO write welcome to screen press any key to continue
	
	while(1)
	{
		game_err = updateObjects();
		if(game_err != 0) break;
		game_err = updateGame();
		if(game_err != 0) break;
		
		swap();
	}
	
	#ifdef DEBUG_MSG_EN
	printf("Stupid BUGS!!! Error Code: %d",game_err);
	#endif
}

uint32_t updateObjects(void) //TODO
{
	uint8_t tempHead = drawableObjects.head; //iterate with this "head"
	
	return 0;
}

uint32_t updateGame(void) //TODO
{
	switch(gameState)
	{
		case INIT:
			if(buttonInput != 0)
			{
				gameState = MENU; //if any key is pressed, continue to the menu
				delay_timestamp = HAL_GetTick(); //take timestamp for delay to prevent double input
				
				mainMenu = malloc(sizeof(menuDObj));
				*mainMenu = createMenu(100,10,menu_header,menu_options);
			}
			break;
			
		case MENU:
			if(HAL_GetTick() - delay_timestamp > 1000 && mainMenu->option_selected != 0) //if it has been a second since we got to the menu
			{
				switch(mainMenu->highlighted_option)
				{
					free(mainMenu); //no longer need to store info about the menu
					
					case 0:
						gameState = MARIO_TEST;
						break;
					case 1:
						gameState = SCROLLING_TEST;
						break;
					case 2:
						gameState = SPRITE_TEST;
						break;
					case 3:
						gameState = MP3_TEST;
						break;
					default:
						return 2;
				}
			}
				
		case MARIO_TEST:
		case SCROLLING_TEST:
		case SPRITE_TEST:
		case MP3_TEST:
		default:
			return 1;
	}
	
	return 0;
}


// Game Program and associated functions
// This contains the logical flow of the game logic and update of the objects within the game
// There should be no direct hardware access from the game functions in this file, it is all abstracted in TestGame_hw.c (swap, DMA2D stuff if necessary, etc)
// Brandon Pollack

#include "TestGame.h"

//Game Globals
DrawQueue drawQueue;
GameState gameState = INIT;
uint32_t delay_timestamp; //keeping track of time (delays) at the game logic level

//GameObjects
DrawableObject* mainMenu;
const char main_menu_header[] = "Please Select One of the Following Tests:";
const char* main_menu_options[] = { "0) Test A Simple Mario Sprite", "1) Scrolling Background Test", "2) Sprite Render Test", "3) MP3 Playback Test" };

//RenderStyle
RenderStyle renderstyle = CPU_RENDER;

void runGame(void)
{
	uint32_t game_err = 0;
	initQueue(&drawQueue); //initialize queue of drawable objects
	
	clearFrameBuffer();
	writeStringToScreen(0,0,"Welcome! Press Any Key to Begin!",0xFFFF,0,3,2);
	swap();
	clearFrameBuffer();
	writeStringToScreen(0,0,"Welcome! Press Any Key to Begin!",0xFFFF,0,3,2); //write welcome screen on both frames
	
	_GET_NEXT_BUTTON_INPUT(); //throw out first input for some reason
	while(input_spi.Instance->SR == (input_spi.Instance->SR & SPI_SR_BSY)); //wait for busy flag
	
	while(1)
	{
		_GET_NEXT_BUTTON_INPUT();
		
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
	DrawableObject** draw = drawQueue.drawQueue;
	
	for(int i = (drawQueue.head + 1); i <= drawQueue.tail; ++i)
	{
		
		draw[i]->update((void*) draw[i]);
		draw[i]->render((void*) draw[i]);
	}
	
	return 0;
}

uint32_t updateGame(void) //TODO
{
	switch(gameState)
	{
		case INIT:
			if(_CURRENT_BUTTON_INPUT() != BUTTON_NONE)
			{
				gameState = MENU; //if any key is pressed, continue to the menu
				delay_timestamp = HAL_GetTick(); //take timestamp for delay to prevent double input
				
				mainMenu = constructMenu(0,0,(char*)main_menu_header,(char**)main_menu_options,sizeof(main_menu_options)/sizeof(char*), renderstyle);
				enqueue(&drawQueue,mainMenu,255);
				clearBothFrameBuffers();
			}
			break;
			
		case MENU:
			if(HAL_GetTick() - delay_timestamp > 500 && MAIN_MENU->option_selected != 0) //if it has been a half second since we got to the menu
			{
				switch(MAIN_MENU->highlighted_option)
				{
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
				removeItem(&drawQueue,mainMenu);
				deconstructMenu(mainMenu);
			}
			break;
				
		case MARIO_TEST:
		case SCROLLING_TEST:
		case SPRITE_TEST:
		case MP3_TEST:
		default:
			return 1;
	}
	
	return 0;
}

// Main Menu function implementations
// Brandon Pollack

#include "MainMenu.h"

DrawableObject* constructMenu(uint16_t x_pos, uint8_t y_pos, char* header, char** options, uint8_t num_options, RenderStyle mode)
{
	DrawableObject* menu = malloc(sizeof(DrawableObject));		 //allocate drawable object	
	menuDObj* datas = malloc(sizeof(menuDObj));
	
	menu->datas = datas;
	menu->update = &updateMenu;
	
	if(mode == CPU_RENDER)
	{
		menu->render = &renderMenuCPU;
	}
	else
	{
		menu->render = &renderMenuDMA2D;
	}
	
	datas->x_pos = x_pos;
	datas->y_pos = y_pos;
	datas->header = header;
	datas->options = options;
	datas->mode = mode;
	datas->num_options = num_options;
	datas->highlighted_option = 0;
	datas->option_selected = 0;
	datas->lastActionTime = HAL_GetTick();
	return menu;
}

void deconstructMenu(DrawableObject* menu)
{
	free(menu->datas);
	free(menu);
}

void updateMenu(void* dobj)
{
	menuDObj* m = (menuDObj *) ((DrawableObject *) dobj)->datas;
	uint32_t currTime = HAL_GetTick();
	
	if(currTime - m->lastActionTime > MENU_UPDATE_SPEED) //if it has been long enough
	{
		if(_BUTTON_PRESSED(BUTTON_DOWN) && _LAST_BUTTON_INPUT() != _CURRENT_BUTTON_INPUT()) //if down selected and not being held
		{
			if(m->highlighted_option < m->num_options - 1)
			{
				m->highlighted_option++;
				m->lastActionTime = currTime; //update time
			}
		}
		else if(_BUTTON_PRESSED(BUTTON_UP) && _LAST_BUTTON_INPUT() != _CURRENT_BUTTON_INPUT()) //same but up (to beginning)
		{
			if(m->highlighted_option > 0)
			{
				m->highlighted_option--;
				m->lastActionTime = currTime;
			}
		}
		else if(_BUTTON_PRESSED(BUTTON_A) && _LAST_BUTTON_INPUT() != _CURRENT_BUTTON_INPUT()) //select option
		{
			m->option_selected = 1;
			m->lastActionTime = currTime;
		}
	}
}

void renderMenuCPU(void* dobj)
{
	menuDObj* m = (menuDObj *) ((DrawableObject *) dobj)->datas;
	uint16_t bg = 0;
	
	writeStringToScreen(m->x_pos,m->y_pos,m->header,0xFFFF,0,1,1); //draw header
	
	for(int i = 0, current_y = (m->y_pos + 2*FONT_HEIGHT); i < m->num_options; ++i, current_y += FONT_HEIGHT)
	{
		if(m->highlighted_option == i) bg = 31; //pure blue
		writeStringToScreen(m->x_pos,current_y,m->options[i],0xFFFF,bg,1,1);
		bg = 0;
	}
}

void renderMenuDMA2D(void* dobj)
{
	//TODO
}

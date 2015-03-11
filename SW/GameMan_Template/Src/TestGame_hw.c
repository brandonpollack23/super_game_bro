//Hardware Based Routines for the game, such as swap for the FB
// Brandon Pollack

#include "TestGame.h"
#include "font12.h"

#define CHAR_OFFSET 32

//LCD
uint16_t* framebuffer_0 = FRAMEBUFFER_0; //framebuffer for writing to and displaying
uint16_t* framebuffer_1 = FRAMEBUFFER_1;

//TODO add scale
void writeCharToScreen(uint16_t x, uint8_t y, char c, uint16_t text_color, uint16_t bg_color) //default text white 0xFFFF, bg black 0
{
	for(;y < FONT_HEIGHT; ++y)
	{
		for(;x < FONT_WIDTH; ++x)
		{
			if((Font12_Table[(c - CHAR_OFFSET)*FONT_HEIGHT] & (1 << x)) == 1) //if the pixel is colored
			{
				*(framebuffer_0 + X_RESOLUTION*y + x) = text_color;
			}
			else
			{
				*(framebuffer_0 + X_RESOLUTION*y + x) = bg_color;
			}
		}
	}
}

//TODO add scale
void writeStringToScreen(uint16_t x, uint8_t y, char* text, uint16_t text_color, uint16_t bg_color)
{
	int index = 0;
	for(char* i = text; *i != '\0'; ++i)//for every char up to null in the string
	{
		if(*i == '\n') //if new line go to next fucking line
		{
			y += FONT_HEIGHT;
		} //PERHAPS check for line overflow? nahhh
		else
		{
			writeCharToScreen(x + index*FONT_WIDTH,y,*i, text_color, bg_color);
		}
		index++;
	}
}

inline void swap(void)
{
	uint16_t* holdmeplz = framebuffer_0;
	framebuffer_0 = framebuffer_1;
	framebuffer_1 = holdmeplz; //swap the FB pointers
	
	SPRITE_LAYER->CFBAR = (uint32_t)framebuffer_0; //set to the new swapped value
	hltdc.Instance->SRCR |= LTDC_SRCR_VBR;
	
	retrieveSPI(INPUT_SPI,&buttonInput);
	initiateSPIRx(INPUT_SPI);
	retrieveTouchI2C(&hi2c1,&touchInput);
	initiateTouchI2CRx(&hi2c1);
	//TODO anything else
}

uint32_t retrieveSPI(SPI_HandleTypeDef* handle, uint16_t* storageVar)
{
	*storageVar = handle->Instance->DR;
	return 0;
}

uint32_t initiateSPIRx(SPI_HandleTypeDef* handle)
{
	//TODO spi RX init
}

uint32_t retrieveTouchI2C(I2C_HandleTypeDef* handle,uint16_t* storageVar)
{
	//TODO I2C Rx
}

uint32_t initiateTouchI2CRx(I2C_HandleTypeDef* handle)
{
	//TODO initiate Rx init
}

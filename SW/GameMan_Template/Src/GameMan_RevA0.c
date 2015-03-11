//Hardware Based Routines for the game, such as swap for the FB
// Brandon Pollack

#include "GameMan_RevA0.h"
#include "TestGame.h"
#include "font12.h"

void Error_Handler(void)
{
	//TODO implement some prints or something for errors, maybe beep codes or light codes
	GameManLed_Write(LED0,GPIO_PIN_SET);
	while(1);
}

uint8_t* globalLoadPointer = (uint8_t*)EXTERNAL_SRAM_USABLE_START_ADDR;

void resetSimpleLoad(void)
{
	globalLoadPointer = (uint8_t*)EXTERNAL_SRAM_USABLE_START_ADDR;
}

uint8_t* simpleLoad(char* filepath, uint32_t (*loadFunc)(char* /*filepath*/, uint8_t* /*dest*/, uint32_t* /*bytes_loaded*/), uint32_t* error)
{
	uint32_t bytes_loaded;
	uint8_t* loaded_addr;
	loaded_addr = globalLoadPointer;
	
	*error = (*loadFunc)(filepath,loaded_addr,&bytes_loaded);
	
	globalLoadPointer += bytes_loaded;
	
	return loaded_addr;
}

/********************LCD********************/
uint16_t* framebuffer_0 = FRAMEBUFFER_0; //framebuffer for writing to and displaying
uint16_t* framebuffer_1 = FRAMEBUFFER_1;

void writeCharToScreen(uint16_t x, uint8_t y, char c, uint16_t text_color, uint16_t bg_color, uint8_t scale_n, uint8_t scale_d) //default text white 0xFFFF, bg black 0
{
	uint16_t temp_x;
	for(uint8_t y_char = 0; y_char <(scale_n*FONT_HEIGHT)/scale_d; ++y_char,++y)
	{
		uint8_t currentCharLine = Font12_Table[(c - CHAR_OFFSET)*FONT_HEIGHT + (scale_d*y_char)/scale_n];
		for(uint16_t x_char = 0, temp_x = x; x_char < (scale_n*FONT_WIDTH)/scale_d; ++x_char,++temp_x)
		{
			if((currentCharLine & (128 >> (scale_d*x_char)/scale_n)) > 0) //if the pixel is colored
			{
				_FB_XY(framebuffer_0,temp_x,y) = text_color;
			}
			else
			{
				_FB_XY(framebuffer_0,temp_x,y) = bg_color;
			}
		}
	}
}

void writeStringToScreen(uint16_t x, uint8_t y, char* text, uint16_t text_color, uint16_t bg_color, uint8_t scale_n, uint8_t scale_d)
{
	int index = 0;
	uint16_t temp_x;
	
	uint8_t offset = (scale_n*FONT_WIDTH/scale_d);
	
	for(char* i = text; *i != '\0'; ++i)//for every char up to null in the string
	{
		temp_x = x + index*offset;
		
		if(*i == '\n' || (temp_x + FONT_WIDTH) > X_RESOLUTION) //if new line go to next fucking line, if overflow go to next line
		{
			++y;
			x = 0;
		} //TODO check for line overflow? nahhh
		else
		{
			writeCharToScreen(temp_x, y, *i, text_color, bg_color,scale_n,scale_d);
		}
		index++;
	}
}

void swap(void)
{
	uint16_t* holdmeplz = framebuffer_0;
	framebuffer_0 = framebuffer_1;
	framebuffer_1 = holdmeplz; //swap the FB pointers
	
	SPRITE_LAYER->CFBAR = (uint32_t)framebuffer_1; //set to the new swapped value
	hltdc.Instance->SRCR |= LTDC_SRCR_VBR;
}

void clearFrameBuffer(void)
{
	for(int i = 0; i < FRAMEBUFFER_SIZE; ++i)
	{
		framebuffer_0[i] = 0;
	}
}

void clearBothFrameBuffers(void)
{
	for(int i = 0; i < FRAMEBUFFER_SIZE*2; ++i)
	{
		FRAMEBUFFER_0[i] = 0;
	}
}

void NV3035_init(void)
{
	HAL_Delay(1);
	NV3035_send_cmd(0,3); //reset
	
	HAL_Delay(1);
	NV3035_send_cmd(0x0E,0x6B);
	
	HAL_Delay(1);
	NV3035_send_cmd(0x0F,0x24);
	
	HAL_Delay(1);
	NV3035_send_cmd(1,2);
}

void NV3035_send_cmd(uint8_t addr, uint8_t data)
{
	uint8_t tx[2];
	HAL_GPIO_WritePin(GPIO_LCD_SPI_NEN,PIN_LCD_SPI_NEN,GPIO_PIN_RESET); //spi select low
	
	tx[0] = (addr << 2) | 2;
	tx[1] = 
	HAL_SPI_Transmit(&lcd_spi,tx,2,1000);
	
	HAL_GPIO_WritePin(GPIO_LCD_SPI_NEN,PIN_LCD_SPI_NEN,GPIO_PIN_SET);
}

void framebuffer_layer_init(LTDC_LayerCfgTypeDef* pLayerCfg, uint8_t layer, uint16_t* fb) //modify or mimic to change settings
{
	/* Layer1 Configuration ------------------------------------------------------*/
  
  /* Windowing configuration */ 
  pLayerCfg->WindowX0 = 0;
  pLayerCfg->WindowX1 = 320;
  pLayerCfg->WindowY0 = 0;
  pLayerCfg->WindowY1 = 240;
  
  /* Pixel Format configuration*/ 
  pLayerCfg->PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  
  /* Start Address configuration : frame buffer is located at esram memory */
  pLayerCfg->FBStartAdress = (uint32_t)fb;
  
  /* Alpha constant (255 totally opaque) */
  pLayerCfg->Alpha = 255;
  
  /* Default Color configuration (configure A,R,G,B component values) */
  pLayerCfg->Alpha0 = 255;
  pLayerCfg->Backcolor.Blue = 0;
  pLayerCfg->Backcolor.Green = 0;
  pLayerCfg->Backcolor.Red = 0;
  
  /* Configure blending factors */
  pLayerCfg->BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg->BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  
  /* Configure the number of lines and number of pixels per line */
  pLayerCfg->ImageWidth = 320;
  pLayerCfg->ImageHeight = 240;
	
	if(HAL_LTDC_ConfigLayer(&hltdc, pLayerCfg, layer) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler(); 
  }
}
/******************************LEDS************************/
void GameManLed_Init(void)
{
	GPIO_InitTypeDef gpio;
	
	__GPIOI_CLK_ENABLE();
	__GPIOC_CLK_ENABLE();
	
	gpio.Pin = PIN_LED0;
	
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FAST;
	
	HAL_GPIO_Init(GPIO_LED0,&gpio);
	
	gpio.Pin = PIN_LED1;
	
	HAL_GPIO_Init(GPIO_LED1,&gpio);
	
	HAL_GPIO_WritePin(GPIO_LED0, PIN_LED0, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIO_LED1, PIN_LED1, GPIO_PIN_RESET);
}

void GameManLed_Write(GameMan_LED l, GPIO_PinState s)
{
	if(l == LED0)
	{
		HAL_GPIO_WritePin(GPIO_LED0, PIN_LED0, s);
	}
	else
	{
		HAL_GPIO_WritePin(GPIO_LED1, PIN_LED1, s);
	}
}

void GameManLed_Toggle(GameMan_LED l)
{
	if(l == LED0)
	{
		HAL_GPIO_TogglePin(GPIO_LED0,PIN_LED0);
	}
	else
	{
		HAL_GPIO_TogglePin(GPIO_LED1,PIN_LED1);
	}
}

/******************************SPI*************************/
uint16_t retrieveSPI(SPI_HandleTypeDef* handle)
{
	SPI_TypeDef* spi = handle->Instance;
	while(spi->SR == (SPI_SR_RXNE & spi->SR));
	return spi->DR;
}

void initiateSPIRx(SPI_HandleTypeDef* handle)
{
	SPI_TypeDef* spi = handle->Instance;
	
	if(spi->SR != (SPI_SR_RXNE & spi->SR))
	{
		spi->CR1 |= SPI_CR1_SPE;
		spi->DR = 0; //send dummy data to recieve
	}
}

/**************************MP3***********************/
unsigned volatile int mp3_ptr = (unsigned int)MP3_BUFFER;
unsigned volatile int mp3_end_ptr = (unsigned int)MP3_BUFFER + (unsigned int)MP3_BUFFER_SIZE;
unsigned volatile int file_ptr;

void configureMp3(uint16_t addr)
{
	uint8_t x[2];
	uint8_t response;
	//TEST
	x[0] = 1;
	HAL_I2C_Master_Transmit(&hi2c1,addr,x,1,1000);
	HAL_I2C_Master_Receive(&hi2c1,addr,(uint8_t*)&response,1,1000);
	
	//first "apply patch"
	for(int i = 0; i < STA013_PATCH_SIZE; i+=2)
	{
		if(HAL_I2C_Master_Transmit(&hi2c1,addr,(uint8_t*)sta013_patch + i,2,1000) != HAL_OK)
		{
			Error_Handler();
		}
		if(*(sta013_patch + i) == 0x10)
		{
			HAL_Delay(1000);
		}
	}
	x[0] = 84;
	x[1] = 1;
	HAL_I2C_Master_Transmit(&hi2c1,addr,x,2,1000);
	x[0] = 85;
	x[1] = 33;
	HAL_I2C_Master_Transmit(&hi2c1,addr,x,2,1000);
	x[0] = 7;
	x[1] = 0;
	HAL_I2C_Master_Transmit(&hi2c1,addr,x,2,1000);
	x[0] = 6;
	x[1] = 12;
	HAL_I2C_Master_Transmit(&hi2c1,addr,x,2,1000);
	x[0] = 11;
	x[1] = 3;
	HAL_I2C_Master_Transmit(&hi2c1,addr,x,2,1000);
	x[0] = 80;
	x[1] = 16;
	HAL_I2C_Master_Transmit(&hi2c1,addr,x,2,1000);
	x[0] = 81;
	x[1] = 0;
	HAL_I2C_Master_Transmit(&hi2c1,addr,x,2,1000);
	x[0] = 82;
	x[1] = 4;
	HAL_I2C_Master_Transmit(&hi2c1,addr,x,2,1000);
	x[0] = 97;
	x[1] = 15;
	HAL_I2C_Master_Transmit(&hi2c1,addr,x,2,1000);
	x[0] = 100;
	x[1] = 85;
	HAL_I2C_Master_Transmit(&hi2c1,addr,x,2,1000);
	x[0] = 101;
	x[1] = 85;
	HAL_I2C_Master_Transmit(&hi2c1,addr,x,2,1000);
	
	x[0] = 24;
	x[1] = 4;
	if(HAL_I2C_Master_Transmit(&hi2c1,addr,x,2,1000) != HAL_OK)
	{
		Error_Handler();
	}
	x[0] = 5;
	x[1] = 161;
	HAL_I2C_Master_Transmit(&hi2c1,addr,x,2,1000);
}

void startPlayingMp3(void)
{
	uint8_t x[2];
	uint16_t* x_p = (uint16_t *)x;
	//start playing
	x[0] = 114;
	x[1] = 1;
	if(HAL_I2C_Master_Transmit(&hi2c1,STA013_ADDR,x,2,1000) != HAL_OK)
	{
		Error_Handler();
	}
	x[0] = 19;
	x[1] = 1;
	HAL_I2C_Master_Transmit(&hi2c1,STA013_ADDR,x,2,1000);
	
	//fill initial buffer
	/*for(int i = 0; i < (int)MP3_BUFFER_SIZE; ++i)
	{
		MP3_BUFFER[i] = *((uint16_t*) (kraidslair + (i << 1))); //read kraids lair as 16 bit numbers and index by 2x i
	}
	
	file_ptr = ((int)MP3_BUFFER_SIZE*2); //file pointer is 2*buffer size bytes into the file
	
	//send from buffer until DR goes low
	while((GPIO_DATA_REQ_MP3->IDR & PIN_DATA_REQ_MP3) == PIN_DATA_REQ_MP3)
	{
		*x_p = *(uint16_t* )mp3_ptr;
		HAL_SPI_Transmit(&mp3_spi,(uint8_t*)((uint32_t)x % (uint32_t)MP3_BUFFER_SIZE),2,1000);
		mp3_ptr += 2; //shouldnt need to check for buffer overflow this is initial send
	}*/
}

void MP3_DR_int_init(void)
{
	SYSCFG->EXTICR[0] = 8 << 0; //set EXTI0 to PortI pin 2
	EXTI->RTSR |= PIN_DATA_REQ_MP3; //set it as rising edge
	EXTI->IMR |= PIN_DATA_REQ_MP3; //unmask external interrupt 0
	HAL_NVIC_SetPriority(EXTI2_IRQn,2,0);
	HAL_NVIC_EnableIRQ(EXTI2_IRQn);
}

void stopPlayingMp3(void)
{
	
}

uint32_t selectSong(char* file)
{
	
}

/*******************************Inputs*******************/
uint16_t buttonInput[20];
uint16_t touchInput[20];
uint32_t input_ptr = -1;
uint16_t getInputs(void)
{
	HAL_GPIO_WritePin(INPUTLATCHPORT,INPUTLATCHPIN,GPIO_PIN_RESET);
	for(volatile int i = 0; i < 2; i++);
	HAL_GPIO_WritePin(INPUTLATCHPORT,INPUTLATCHPIN,GPIO_PIN_SET);
	
	initiateSPIRx(&input_spi);
	return retrieveSPI(&input_spi);
}

/********************************I2C**************************/
uint32_t retrieveTouchI2C(I2C_HandleTypeDef* handle,uint16_t* storageVar)
{
	//TODO I2C Rx
}

uint32_t initiateTouchI2CRx(I2C_HandleTypeDef* handle)
{
	//TODO initiate Rx init
}

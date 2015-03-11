/* GameMan Board Support Header
	 Brandon Pollack
*/

#ifndef __GAMEMAN_BSP
#define __GAMEMAN_BSP
#include "stm32f4xx_hal.h"
#include "stdint.h"
#include "sta013_patch.h"
#include "ff_gen_drv.h"

//comment out this line to remove debug prints
#define DEBUG_MSG_EN

//handlers
extern DAC_HandleTypeDef hdac;

extern DMA2D_HandleTypeDef hdma2d;

extern I2C_HandleTypeDef hi2c1;

extern LTDC_HandleTypeDef hltdc;

extern RNG_HandleTypeDef hrng;

extern SPI_HandleTypeDef input_spi;
extern SPI_HandleTypeDef mp3_spi;
extern SPI_HandleTypeDef lcd_spi;

extern UART_HandleTypeDef huart1;

extern SRAM_HandleTypeDef hsram1;

//Peripherals
#define DEBUG_UART &huart1
#define INPUT_SPI &hspi2
#define GPIO_LED0 GPIOI
#define GPIO_LED1 GPIOC
#define PIN_LED0 GPIO_PIN_8
#define PIN_LED1 GPIO_PIN_13
#define INPUTLATCHPORT GPIOG
#define INPUTLATCHPIN GPIO_PIN_8
#define STA013_ADDR ((uint16_t)(0x43 << 1))
#define PIN_DATA_REQ_MP3 GPIO_PIN_0
#define GPIO_DATA_REQ_MP3 GPIOI
#define PIN_LCD_SPI_NEN GPIO_PIN_8
#define GPIO_LCD_SPI_NEN GPIOF

//Input buttons
#define INPUT_HIST_SIZE 20
#define BUTTON_NONE 0xFFF0
#define BUTTON_A (1 << 15)
#define BUTTON_B (1 << 14)
#define BUTTON_X (1 << 13)
#define BUTTON_Y (1 << 12)
#define BUTTON_UP (1 << 11)
#define BUTTON_DOWN (1 << 10)
#define BUTTON_LEFT (1 << 9)
#define BUTTON_RIGHT (1 << 8)
#define BUTTON_LS (1 << 7)
#define BUTTON_RS (1 << 6)
#define BUTTON_START (1 << 5)
#define BUTTON_SELECT (1 << 4)

//External Memory Defines
#define EXTERNAL_SRAM_ADDRESS_BASE ((uint8_t*) 0x60000000)
#define EXTERNAL_SRAM_SIZE_HALFWORDS 1048576
#define EXTERNAL_SRAM_SIZE_BYTES 2*EXTERNAL_SRAM_SIZE_HALFWORDS

//LCD Defines -- change to change framebuffer bases, window sizes etc
#define X_RESOLUTION 320
#define Y_RESOLUTION 240
#define FRAMEBUFFER_SIZE (X_RESOLUTION * Y_RESOLUTION)
#define FRAMEBUFFER_0 ((uint16_t*) EXTERNAL_SRAM_ADDRESS_BASE)
#define FRAMEBUFFER_1 ((uint16_t*) FRAMEBUFFER_0 + FRAMEBUFFER_SIZE)
#define BACKGROUND_LAYER ((LTDC_Layer_TypeDef *)__HAL_LTDC_LAYER(&hltdc,0))
#define SPRITE_LAYER ((LTDC_Layer_TypeDef *)__HAL_LTDC_LAYER(&hltdc,1))
#define BACKGROUND_LAYER_NUM 0
#define SPRITE_LAYER_NUM 1

#define EXTERNAL_SRAM_USABLE_START_ADDR (FRAMEBUFFER_1 + FRAMEBUFFER_SIZE)

#define CHAR_OFFSET 32
#define _FB_XY(fb,x,y) *(fb + y*X_RESOLUTION + x)

//MP3 Defines, statically set MP3 buffer location
#define MP3_BUFFER (FRAMEBUFFER_1 + FRAMEBUFFER_SIZE)
#define MP3_BUFFER_SIZE ((uint16_t*)6000)

//global variables and functions
void Error_Handler(void);

//SD card/FATFS
extern FIL file;
void resetSimpleLoad(void); //resets global static pointer
uint8_t* simpleLoad(char* filepath, uint32_t (*loadFunc)(char* /*filepath*/, uint8_t* /*dest*/, uint32_t* /*bytes_loaded*/), uint32_t* error); //load address determined by static global who starts at available memory address; loadFunc is a function pointer to the specific kind of load we're doing

//LCD
extern uint16_t* framebuffer_0; //framebuffer for writing to and displaying
extern uint16_t* framebuffer_1;
extern void NV3035_init(void);
extern void NV3035_send_cmd(uint8_t addr, uint8_t data);
extern void framebuffer_layer_init(LTDC_LayerCfgTypeDef* pLayerCfg, uint8_t layer, uint16_t* fb);
extern void swap(void); //if drawQueue is empty swap the framebuffer and tell the shadow registers to update at v_sync, update everything necessary (inputs, touch screen, etc)
void clearFrameBuffer(void);
void clearBothFrameBuffers(void);

//Inputs
extern uint16_t buttonInput[INPUT_HIST_SIZE]; //let the compiler reserve this location at the beginning so I can't fuck it up
extern uint16_t touchInput[INPUT_HIST_SIZE];
extern uint32_t input_ptr;
uint16_t getInputs(void);
#define _GET_NEXT_BUTTON_INPUT() buttonInput[(++input_ptr % INPUT_HIST_SIZE)] = getInputs()
#define _CURRENT_BUTTON_INPUT() buttonInput[(input_ptr % INPUT_HIST_SIZE)]
#define _LAST_BUTTON_INPUT() buttonInput[(input_ptr-1) % INPUT_HIST_SIZE]
#define _BUTTON_PRESSED(btn) ((~(_CURRENT_BUTTON_INPUT()) & btn) == btn)


//Function Prototypes
//LEDs
typedef enum LED { LED0, LED1} GameMan_LED;

void GameManLed_Init(void); //initilizes PI8 and PC13 as outputs
void GameManLed_Write(GameMan_LED l, GPIO_PinState s);
void GameManLed_Toggle(GameMan_LED l);
	
//SPI
uint16_t retrieveSPI(SPI_HandleTypeDef* handle);
void initiateSPIRx(SPI_HandleTypeDef* handle);

//MP3
extern volatile unsigned int file_ptr;
extern volatile unsigned int mp3_ptr;
extern volatile unsigned int mp3_end_ptr;
void configureMp3(uint16_t addr);
void startPlayingMp3(void); //sends I2c commands to MP3 chip to begin
void MP3_DR_int_init(void); //sets up DR interrupt
void stopPlayingMp3(void); //disables DR interrupt and sends I2c commands to MP3 chip to stop
uint32_t selectSong(char* file); //selects file from SD card to play as mp3

//I2C
uint32_t retrieveTouchI2C(I2C_HandleTypeDef* handle,uint16_t* storageVar);
uint32_t initiateTouchI2CRx(I2C_HandleTypeDef* handle);

//Utilities
void writeCharToScreen(uint16_t x, uint8_t y, char c, uint16_t color, uint16_t bg_color, uint8_t scale_n, uint8_t scale_d); //write one character
void writeStringToScreen(uint16_t x, uint8_t y, char* text, uint16_t text_color, uint16_t bg_color, uint8_t scale_n, uint8_t scale_d); //function to write text to screen without gameobject
HAL_StatusTypeDef print_debug(uint8_t *pData, uint16_t Size, uint32_t Timeout);

#endif

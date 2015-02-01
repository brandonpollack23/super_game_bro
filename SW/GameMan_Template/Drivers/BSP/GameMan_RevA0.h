/* GameMan Board Support Header
	 Brandon Pollack
*/

#ifndef __GAMEMAN_BSP
#define __GAMEMAN_BSP
#include "stm32f4xx_hal.h"
#include "stdint.h"

//comment out this line to remove debug prints
#define DEBUG_MSG_EN

//Peripherals
#define DEBUG_UART &huart1

//External Memory Defines
#define EXTERNAL_SRAM_ADDRESS_BASE ((uint8_t*) FMC_Bank1)
#define EXTERNAL_NOR_ADDRESS_BASE ((uint8_t*) FMC_Bank1 + (0x4000000))

//LCD Defines -- change to change framebuffer bases, window sizes etc
#define X_RESOLUTION 320
#define Y_RESOLUTION 240
#define FRAMEBUFFER_SIZE (X_RESOLUTION * Y_RESOLUTION)
#define FRAMEBUFFER_0 ((uint16_t*) EXTERNAL_SRAM_ADDRESS_BASE)
#define FRAMEBUFFER_1 ((uint16_t*) FRAMEBUFFER_0 + FRAMEBUFFER_SIZE)

//global variables and functions
//LCD
extern uint16_t* framebuffer_0; //framebuffer for writing to and displaying
extern uint16_t* framebuffer_1;

//Inputs
extern uint16_t buttonInput; //let the compiler reserve this location at the beginning so I can't fuck it up

HAL_StatusTypeDef print_debug(uint8_t *pData, uint16_t Size, uint32_t Timeout);

#endif

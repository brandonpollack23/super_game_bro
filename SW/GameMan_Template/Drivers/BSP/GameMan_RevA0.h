/* GameMan Board Support Header
	 Brandon Pollack
*/

#ifndef __GAMEMAN_BSP
#define __GAMEMAN_BSP

//comment out this line to remove debug prints
#define DEBUG_MSG_EN

#define DEBUG_UART &huart1

//External Memory Defines
#define EXTERNAL_SRAM_ADDRESS_BASE ((size_t*) FMC_Bank1)
#define EXTERNAL_NOR_ADDRESS_BASE ((size_t*) FMC_Bank1 + (0x4000000))

//LCD Defines -- change to change framebuffer bases, window sizes etc
#define X_RESOLUTION 320
#define Y_RESOLUTION 240
#define FOREGROUND_FRAMEBUFFER_BASE EXTERNAL_SRAM_ADDRESS_BASE

HAL_StatusTypeDef print_debug(uint8_t *pData, uint16_t Size, uint32_t Timeout);

#endif

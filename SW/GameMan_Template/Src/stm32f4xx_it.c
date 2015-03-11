/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @date    04/03/2015 14:16:22
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2015 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"
/* USER CODE BEGIN 0 */
#include "GameMan_RevA0.h"
#include "kraidsfuckinglair.h"
/* USER CODE END 0 */
/* External variables --------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles System tick timer.
*/
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/* USER CODE BEGIN 1 */
void EXTI0_IRQHandler(void)
{
	//send from buffer until DR goes low
	while((GPIO_DATA_REQ_MP3->IDR & PIN_DATA_REQ_MP3) == PIN_DATA_REQ_MP3)
	{
		HAL_SPI_Transmit(&mp3_spi,(uint8_t*)((uint32_t)mp3_ptr % (uint32_t)MP3_BUFFER_SIZE),2,1000);
		mp3_ptr += 2; //increment mp3 buffer pointer by 2, we just sent 2 bytes
		mp3_ptr %= (int)MP3_BUFFER_SIZE;
		mp3_ptr += (int)MP3_BUFFER;
	}
	
	for(int i = mp3_end_ptr+1; i < mp3_ptr; i = ((i + 1) % (int)MP3_BUFFER_SIZE))
	{
		MP3_BUFFER[i] = *((uint16_t*)(kraidslair + (file_ptr)));
		file_ptr += 2; //we just filled 2 bytes into the buffer, inc file by 2
		file_ptr %= sizeof(kraidslair)/sizeof(uint8_t);
	}
	mp3_end_ptr = (int)MP3_BUFFER + ((mp3_ptr - 1) % (int)MP3_BUFFER_SIZE);
}
/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

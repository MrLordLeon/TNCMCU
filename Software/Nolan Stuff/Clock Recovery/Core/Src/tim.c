/**
  ******************************************************************************
  * File Name          : TIM.c
  * Description        : This file provides code for the configuration
  *                      of the TIM instances.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2020 STMicroelectronics
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
#include "tim.h"

#include "gpio.h"

/* USER CODE BEGIN 0 */
#include "stdbool.h"

#define RISING_EDGE	1
#define FALLING_EDGE	0

// NOTE: these symbol periods/margins are pre-calculated assuming:
	// SYSCLK = 80 MHz -> HCLK = 80 MHz -> APB1 timer clocks = 80 MHz
// should your values differ, you will need to re-calculate
// or, ideally, write code that calculates these values on the fly from the current APB1 timer clock value
// there are macros somewhere in the HAL libraries to get this value in runtime.

// these values are to be used with prescaler = 0
//const uint32_t SYMBOL_PERIOD = 66667;
//const uint32_t SYMBOL_MARGIN = 6667;

// this assumes that the timer is ticking at 40 MHz (80 MHz input with prescaler set to 1)
// these values to be used with prescaler = 1
const uint32_t SYMBOL_PERIOD = 33333;
const uint32_t SYMBOL_MARGIN = 3333;

// used to store the difference between transitions (in 40 MHz ticks)
// does not need to be global, but it is anyway
uint32_t capture_difference = 0;

// needs to be global, as it is used by both the input capture and output compare callbacks
uint32_t new_bit_period = 0;
// doesn't need to be global, but it is anyway
uint32_t next_capture_time = 0;
/* USER CODE END 0 */

TIM_HandleTypeDef htim2;

/* TIM2 init function */
void MX_TIM2_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_OC_InitTypeDef sConfigOC;
  TIM_IC_InitTypeDef sConfigIC;

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xffffffff;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_INDIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_BOTHEDGE;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  HAL_TIM_MspPostInit(&htim2);

}

void HAL_TIM_OC_MspInit(TIM_HandleTypeDef* tim_ocHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(tim_ocHandle->Instance==TIM2)
  {
  /* USER CODE BEGIN TIM2_MspInit 0 */

  /* USER CODE END TIM2_MspInit 0 */
    /* TIM2 clock enable */
    __HAL_RCC_TIM2_CLK_ENABLE();
  
    /**TIM2 GPIO Configuration    
    PA3     ------> TIM2_CH4 
    */
    GPIO_InitStruct.Pin = A2_IC_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(A2_IC_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM2_MspInit 1 */

  /* USER CODE END TIM2_MspInit 1 */
  }
}
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(timHandle->Instance==TIM2)
  {
  /* USER CODE BEGIN TIM2_MspPostInit 0 */

  /* USER CODE END TIM2_MspPostInit 0 */
  
    /**TIM2 GPIO Configuration    
    PA1     ------> TIM2_CH2 
    */
    GPIO_InitStruct.Pin = A1_OC_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(A1_OC_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM2_MspPostInit 1 */

  /* USER CODE END TIM2_MspPostInit 1 */
  }

}

void HAL_TIM_OC_MspDeInit(TIM_HandleTypeDef* tim_ocHandle)
{

  if(tim_ocHandle->Instance==TIM2)
  {
  /* USER CODE BEGIN TIM2_MspDeInit 0 */

  /* USER CODE END TIM2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM2_CLK_DISABLE();
  
    /**TIM2 GPIO Configuration    
    PA1     ------> TIM2_CH2
    PA3     ------> TIM2_CH4 
    */
    HAL_GPIO_DeInit(GPIOA, A1_OC_Pin|A2_IC_Pin);

    /* TIM2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(TIM2_IRQn);
  /* USER CODE BEGIN TIM2_MspDeInit 1 */

  /* USER CODE END TIM2_MspDeInit 1 */
  }
} 

/* USER CODE BEGIN 1 */

// this function takes a "difference", or number of clock ticks between transitions, as well as a bit number
// if the difference between transitions is within the valid band, the function returns true; otherwise, false.

// this allows for a margin to exist for clock jitter, drift, etc.
bool DifferenceWithinWindow(uint32_t difference, uint8_t bit_number)
{
	// does the margin get multiplied, too?

	//	uint32_t upper_bound = (((bit_number + 1) * SYMBOL_PERIOD) + SYMBOL_MARGIN);
	//	uint32_t lower_bound = (((bit_number + 1) * SYMBOL_PERIOD) - SYMBOL_MARGIN);

	uint32_t upper_bound = (((bit_number + 1) * (SYMBOL_PERIOD + SYMBOL_MARGIN)));
	uint32_t lower_bound = (((bit_number + 1) * (SYMBOL_PERIOD - SYMBOL_MARGIN)));

	if ((lower_bound < difference) && (difference < upper_bound))
	{
		return true;
	}
	return false;
}

// this function is automatically called whenever the input capture interrupt hits
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	static uint32_t rising_capture = 0;		// stores the timer tick value of the most recent rising edge
	static uint32_t falling_capture = 0;	// stores the timer tick value of the most recent falling edge

	static bool rise_captured = false;		// these are used to ensure that we aren't trying to compute the difference
	static bool fall_captured = false;		// before we have captured both a rising and falling edge

	static bool signal_edge = RISING_EDGE;	// so we know what edge we are looking for (really, the opposite of the edge that was captured last

	uint32_t this_capture = 0;		// simply stores either the rising or falling capture, based on which state we are in (avoids duplicate code)


	uint8_t bit_num = 0;		// loop counter for difference timing correlator

	if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
	{
		if (signal_edge == RISING_EDGE)
		{
			rising_capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4);		// read the captured value
			rise_captured = true;
			signal_edge = FALLING_EDGE;		// look for falling edge on next capture

			if (rise_captured && fall_captured)
			{
				capture_difference = rising_capture - falling_capture;		// calculate difference
				this_capture = rising_capture;		// set current sample to rising edge
			}
		}
		// falling edge
		else
		{
			falling_capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4);		// read the captured value
			fall_captured = true;
			signal_edge = RISING_EDGE;		// look for rising edge on next capture

			if (rise_captured && fall_captured)
			{
				capture_difference = falling_capture - rising_capture;		// calculate difference
				this_capture = falling_capture;
			}
		}

		if (rise_captured && fall_captured)
		{
			// this may need to be adjusted to account for HDLC limitation of valid data to 6 consecutive 1 bits (6 bit periods without a transition)
			// but works as is
			for (bit_num = 0; bit_num < 8; bit_num ++)
			{
				if (DifferenceWithinWindow(capture_difference, bit_num))		// iteratively check to see if the difference between the last and current transition falls within any acceptable bit window
				{
					new_bit_period = (capture_difference / (bit_num + 1));		// if correlated, we calculate when we expect the next transition to theoretically fall, if the next bit period were the same as the current one
																				// this configuration will generate a 600 Hz (1200 transition/second) clock on the output compare module/pin
					next_capture_time = this_capture + new_bit_period;
					__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, next_capture_time);
					// may need a break statement here?
					// but works without it...
				}
				// probably need to do something in the event that we did not correlate with any of the acceptable windows
			}
		}
	}

	return;
}

// this function is called automatically whenever the output compare interrupt hits
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
	{
		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, (__HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_2) + (new_bit_period)));		// if we have not received a transition to the input capture module, we want to refresh the output compare module with the last known bit period

		HAL_GPIO_TogglePin(A3_GPIO_Port, A3_Pin);		// here we toggle a pin just to let us know that this interrupt is firing. totally unnecessary, and really just matches the waveform that the output compare module generates anyway.
	}

	return;
}
/* USER CODE END 1 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/*
 * FreqIO.c
 *
 *  Created on: Jul 21, 2020
 *      Author: monke
 */
#include "FreqIO.h"

DAC_HandleTypeDef hdac;
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;

char uartData[3000];
uint32_t periodBuffer[BUFFERSIZE];
uint16_t buffLoadCount = 0;
bool first = false;
uint32_t period;


int freqtobit(uint32_t inputPeriod){
	int freq = PCONVERT / period;
	//return freq;
	if( (HIGHFREQ-FREQDEV < freq) && (freq < HIGHFREQ+FREQDEV) )
		return 1;
	if( (LOWFREQ-FREQDEV < freq) && (freq < LOWFREQ+FREQDEV) )
		return 0;
	else
		return -1;
}

void FreqCounterPinEXTI(){
	//HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_5);
	if(!first){
		htim2.Instance->CNT = 0;
		first = true;
	}
	else {
		period = htim2.Instance->CNT;
		first = false;
	}
}

void Tim3IT(){
	//HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_5);
	periodBuffer[buffLoadCount] = period;
	buffLoadCount++;
	if(buffLoadCount>=BUFFERSIZE)
		buffLoadCount = 0;
}

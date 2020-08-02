/*
 * FreqIO.c
 *
 *  Created on: Jul 21, 2020
 *      Author: monke
 */
#include "FreqIO.h"

//Needed uController Objects
//****************************************************************************************************************
DAC_HandleTypeDef hdac;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart2;
//Connectivity
//****************************************************************************************************************
char uartData[3000];

//General Program
//****************************************************************************************************************
bool mode = true;

void initController(){
	initOUTData();
	if(mode) {
		htim2.Instance->ARR = AUTORELOAD_TX;
	} else {
		htim2.Instance->ARR = AUTORELOAD_RX;
	}
	sprintf(uartData, "Init controller complete\r\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
}
void toggleMode() {
	mode = !mode;
	if (mode) {
		HAL_NVIC_DisableIRQ(EXTI0_IRQn);
		HAL_TIM_Base_Stop(&htim3);
		htim2.Instance->ARR = AUTORELOAD_TX;
	} else {
		HAL_NVIC_EnableIRQ(EXTI0_IRQn);
		HAL_TIM_Base_Start_IT(&htim3);
		htim2.Instance->ARR = AUTORELOAD_RX;
	}
	sprintf(uartData, "Set mode to %d\r\n",mode);
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
}
void tx_rx(){
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,mode);
	if(mode){
		bitToAudio(&bitStream,10);
	} else{
		sprintf(uartData, "periodBuffer[%d] = %d\r\n",0,pertobit(periodBuffer[0]));
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		HAL_Delay(500);
	}

}

//GENERATING FREQ
//****************************************************************************************************************
bool bitStream[10];

uint32_t sine_val[100];
uint32_t lowFrequency[LOWF];
uint32_t highFrequency[HIGHF];

void get_sineval() {
	for (int i = 0; i < 100; i++) {
		//formula in DAC Document
		sine_val[i] = ((sin(i * 2 * PI / 100) + 1) * (4096 / 2));
	}
}
void edit_sineval(uint32_t *sinArray, int arraySize) {
	for (int i = 0; i < arraySize; i++) {
		//formula in DAC Document
		sinArray[i] = ((sin((i - 45) * 2 * PI / arraySize) + 1.1) * (4096 / 4));
	}
}
void bitToAudio(bool *bitStream, int arraySize) {
	for (int i = 0; i < arraySize; i++) {
		if (bitStream[i])
			HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, lowFrequency, LOWF,
			DAC_ALIGN_12B_R);
		else
			HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, highFrequency, HIGHF,
			DAC_ALIGN_12B_R);
		HAL_Delay(0.5);
		HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
	}
}
void generateBitstream() {
	bitStream[0] = 1;
	bitStream[1] = 1;
	bitStream[2] = 1;
	bitStream[3] = 0;
	bitStream[4] = 0;
	bitStream[5] = 0;
	bitStream[6] = 1;
	bitStream[7] = 0;
	bitStream[8] = 0;
	bitStream[9] = 0;
}
void initOUTData() {
	get_sineval();
	edit_sineval(lowFrequency, LOWF);
	edit_sineval(highFrequency, HIGHF);
	generateBitstream();
}

//READING FREQ
//****************************************************************************************************************
uint32_t periodBuffer[BUFFERSIZE];
uint16_t buffLoadCount = 0;
bool first = false;
uint32_t period;

int pertobit(uint32_t inputPeriod) {
	int freq = PCONVERT / period;
	//return freq;
	if ((HIGHFREQ - FREQDEV < freq) && (freq < HIGHFREQ + FREQDEV))
		return 1;
	if ((LOWFREQ - FREQDEV < freq) && (freq < LOWFREQ + FREQDEV))
		return 0;
	else
		return -1;
}

void FreqCounterPinEXTI() {
	//HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_5);
	if (!first) {
		htim2.Instance->CNT = 0;
		first = true;
	} else {
		period = htim2.Instance->CNT;
		first = false;
	}
}

void Tim3IT() {
	//HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_5);
	periodBuffer[buffLoadCount] = period;
	buffLoadCount++;
	if (buffLoadCount >= BUFFERSIZE)
		buffLoadCount = 0;
}

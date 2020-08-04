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
DMA_HandleTypeDef hdma_dac1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart2;
//Connectivity
//****************************************************************************************************************
char uartData[3000];

//General Program
//****************************************************************************************************************
bool mode = false;
bool midbit = false;

void initController() {
	initOUTData();
	if (mode) {
		htim2.Instance->ARR = TIM2_AUTORELOAD_TX;
		htim3.Instance->ARR = TIM3_AUTORELOAD_TX;
	} else {
		htim2.Instance->ARR = TIM2_AUTORELOAD_RX;
		htim3.Instance->ARR = TIM3_AUTORELOAD_RX;
	}
}
void toggleMode() {
	mode = !mode;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, mode);
	htim3.Instance->CNT = 0;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0);

	if (mode) {
		//Shutdown RX
		HAL_NVIC_DisableIRQ(EXTI0_IRQn);
		HAL_TIM_Base_Stop(&htim3);

		//Init TX
		htim2.Instance->ARR = TIM2_AUTORELOAD_TX;
		htim3.Instance->ARR = TIM3_AUTORELOAD_TX;
	} else {
		//Shutdown TX
		midbit = false;
		HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
		HAL_TIM_Base_Stop(&htim3);

		//Init RX
		HAL_NVIC_EnableIRQ(EXTI0_IRQn);
		htim2.Instance->ARR = TIM2_AUTORELOAD_RX;
		htim3.Instance->ARR = TIM3_AUTORELOAD_TX;
		HAL_TIM_Base_Start_IT(&htim3);
	}
}
void tx_rx() {
	if (mode) {
		bitToAudio(&bitStream, 2);
	} else {
		sprintf(uartData, "periodBuffer[%d] = %d\r\n", 0,
				pertobit(periodBuffer[0]));
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		HAL_Delay(500);
	}

}

//GENERATING FREQ
//****************************************************************************************************************
bool bitStream[10];


uint32_t lowFrequency[2*LOWF_SAMP];
uint32_t highFrequency[2*HIGHF_SAMP];

void edit_sineval(uint32_t *sinArray, int arraySize, int waves) {
	for (int i = 0; i < arraySize; i++) {
		//formula in DAC Document
		sinArray[i] = ((sin((i - 45) * 2 * PI / arraySize * waves) + 1.1) * (OUT_AMPL/ 4));
	}
}
void bitToAudio(bool *bitStream, int arraySize) {
	for (int i = 0; i < arraySize; i++) {
		if (bitStream[i]){
			HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, highFrequency, HIGHF_SAMP,DAC_ALIGN_12B_R);

			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 1);
			htim3.Instance->CNT = 0;
			HAL_TIM_Base_Start_IT(&htim3);
			midbit = true;
		}
		else{
			HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, lowFrequency, LOWF_SAMP,DAC_ALIGN_12B_R);

			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0);
			htim3.Instance->CNT = 0;
			HAL_TIM_Base_Start_IT(&htim3);
			midbit = true;
		}

		while(midbit) __NOP();		//Just wait for timer3 IT to go off.
	}
	HAL_TIM_Base_Stop(&htim3);
}
void generateBitstream() {
	bitStream[0] = 1;
	bitStream[1] = 0;
	/*
	bitStream[2] = 1;
	bitStream[3] = 0;
	bitStream[4] = 0;
	bitStream[5] = 0;
	bitStream[6] = 1;
	bitStream[7] = 0;
	bitStream[8] = 0;
	bitStream[9] = 0;
	*/
}
void initOUTData() {
	edit_sineval(lowFrequency, 2*LOWF_SAMP,2);
	edit_sineval(highFrequency, 2*HIGHF_SAMP,2);
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
	if (!first) {
		htim2.Instance->CNT = 0;
		first = true;
	} else {
		period = htim2.Instance->CNT;
		first = false;
	}
}

void Tim3IT() {
	if (mode) {
		HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
		midbit = false;
	} else {
		//HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_5);
		periodBuffer[buffLoadCount] = period;
		buffLoadCount++;
		if (buffLoadCount >= BUFFERSIZE)
			buffLoadCount = 0;
	}
}

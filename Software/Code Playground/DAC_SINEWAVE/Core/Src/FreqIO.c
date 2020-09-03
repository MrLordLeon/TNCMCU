/*
 * FreqIO.c
 *
 *  Created on: Jul 21, 2020
 *      Author: monke
 */
#include "FreqIO.h"
#include "AX.25.h"

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

//GENERATING FREQ
//****************************************************************************************************************
bool bitStream[10];

uint32_t lowFrequency[2 * LOWF_SAMP];
uint32_t highFrequency[2 * HIGHF_SAMP];

void edit_sineval(uint32_t *sinArray, int arraySize, int waves) {
	for (int i = 0; i < arraySize; i++) {
		//formula in DAC Document
		sinArray[i] = (sin(
				(i + (.75 * arraySize * waves)) * 2 * PI / arraySize * waves)
				+ 1) * (OUT_AMPL / 4);
	}
}
void bitToAudio(bool *bitStream, int arraySize) {
	for (int i = 0; i < arraySize; i++) {
		if (bitStream[i]) {
			htim3.Instance->CNT = 0;
			HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, highFrequency, HIGHF_SAMP,
					DAC_ALIGN_12B_R);
			HAL_TIM_Base_Start_IT(&htim3);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 1);
			midbit = true;
		} else {
			htim3.Instance->CNT = 0;
			HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, lowFrequency, LOWF_SAMP,
					DAC_ALIGN_12B_R);
			HAL_TIM_Base_Start_IT(&htim3);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0);
			midbit = true;
		}

		while (midbit)
			__NOP();		//Just wait for timer3 IT to go off.
	}
	HAL_TIM_Base_Stop(&htim3);
}
void generateBitstream() {
	bitStream[0] = 1;
	bitStream[1] = 0;
	bitStream[2] = 1;
	bitStream[3] = 0;
	bitStream[4] = 0;
	bitStream[5] = 0;
	bitStream[6] = 1;
	bitStream[7] = 0;
	bitStream[8] = 1;
	bitStream[9] = 1;

}
void initOUTData() {
	edit_sineval(lowFrequency, 2 * LOWF_SAMP, 2);
	edit_sineval(highFrequency, 2 * HIGHF_SAMP, 2);
	generateBitstream();
}

//READING FREQ
//****************************************************************************************************************
uint32_t periodBuffer[RX_BUFFERSIZE];
uint32_t bitBuffer[RX_BUFFERSIZE];
uint8_t sampusecount = 0;
uint16_t periodSaveCount = 0;
uint16_t trackBit = 0;
uint16_t bitSaveCount = 0;

/*
 * 	Function to calculate bit value based on period. Returns the bit value
 */
int pertobit(uint32_t inputPeriod) {
	int freq = PCONVERT / inputPeriod;
	//return freq;
	if ((HIGHFREQ - FREQDEV < freq) && (freq < HIGHFREQ + FREQDEV))
		return 1;
	if ((LOWFREQ - FREQDEV < freq) && (freq < LOWFREQ + FREQDEV))
		return 0;
	else
		return -1;
}

/*
 *	Function to take period buffer values and loads the next bit into bit buffer.
 *	Also returns the determined bitvalue
 *	0 	= 1200Hz
 *	1  	= 2200Hz
 *	-1	= Invalid frequency
 */
int loadBit(){
	int currbit = 0;
	int nextbit = 0;

	currbit = pertobit(periodBuffer[trackBit]);

	//Low frequency should have 1 bit per baud
	if(currbit==0){
		bitBuffer[bitSaveCount] = 0;
	}

	//High frequency should have 2 high bits per baud
	else if(currbit==1){
		//Gather next bit
		//ternary assign: var = (cond)?if_true:if_false;
		nextbit = (trackBit!=RX_BUFFERSIZE-1)?pertobit(periodBuffer[trackBit+1]):pertobit(periodBuffer[0]);

		if(nextbit==1){
			//High frequency detected, skip next bit
			trackBit++;
			bitBuffer[bitSaveCount] = 1;
		}
	}
	//Invalid bit
	else{
		bitBuffer[bitSaveCount] = -1;
	}

	//Increment trackBit
	trackBit++;
	if (trackBit >= RX_BUFFERSIZE)
		trackBit = 0;

	//Increment bitSaveCount
	bitSaveCount++;
	if (bitSaveCount >= RX_BUFFERSIZE)
		bitSaveCount = 0;

	return currbit;
}

/*
 *	Function to iterate through period buffer and find a wave start signal. (0xC0=1100 0000)
 *	returns the index of wave start including 0xC0
 *
 */
int streamCheck() {
	int bit7,bit6,bit5,bit4,bit3,bit2,bit1,bit0;

	//Just do this unless we need to toggle
	while(!changeMode){
		//Slide bits
		bit7 = bit6;
		bit6 = bit5;
		bit5 = bit4;
		bit4 = bit3;
		bit3 = bit2;
		bit2 = bit1;
		bit1 = bit0;
		bit0 = loadBit();

		//Detect 0xC
		if((bit7==0)&&(bit6==1)&&(bit5==1)&&(bit4==1)){
			//Detect 0xC0
			if((bit3==1)&&(bit2==1)&&(bit1==1)&&(bit0==0)){
				AX25_flag = !AX25_flag;
				sprintf(uartData, "AX.25 Flag Detected\r\n");
			}
		}
		else {
			sprintf(uartData, "Bits detected: %d %d %d %d %d %d %d %d\r\n",bit7,bit6,bit5,bit4,bit3,bit2,bit1,bit0);
		}

		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	if(toggleMode)
		return -1;
}

//General Program
//****************************************************************************************************************
bool mode = true;
bool midbit = false;
bool changeMode = false;

void initProgram() {
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
	//Disable HW interrupt
	HAL_NVIC_DisableIRQ(EXTI0_IRQn);

	//Stop DAC
	HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);

	//Toggle mode
	mode = !mode;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, mode);
	midbit = false;

	//Stop timer and reset count
	HAL_TIM_Base_Stop(&htim3);
	htim3.Instance->CNT = 0;

	if (mode) {
		//Set Timer periods
		htim2.Instance->ARR = TIM2_AUTORELOAD_TX;
		htim3.Instance->ARR = TIM3_AUTORELOAD_TX;

	} else {
		//Set Timer Periods
		htim2.Instance->ARR = TIM2_AUTORELOAD_RX;
		htim3.Instance->ARR = TIM3_AUTORELOAD_RX;

		//Enable tim3 interrupt
		HAL_TIM_Base_Start_IT(&htim3);

		//Enable HW interrupt
		HAL_NVIC_EnableIRQ(EXTI0_IRQn);
	}

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0);
}
bool BuffRd = false;
int printCnt = 0;
void tx_rx() {
	if (changeMode) {
		changeMode = 0;
		toggleMode();
	}

	if (mode) {
		bitToAudio(&bitStream[0], 10);
	} else {
		streamCheck();

		/*
		for (int i = 0; i < RX_BUFFERSIZE; i++) {
			//sprintf(uartData, "periodBuffer[%d] period value = %d\r\n", i, periodBuffer[i]);
			sprintf(uartData, "periodBuffer[%d] bit value = %d\r\n", i, pertobit(periodBuffer[i]));
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
		*/
	}

}
void loadPeriodBuffer(int timerCnt) {
	periodBuffer[periodSaveCount] = timerCnt;
	periodSaveCount++;
	if (periodSaveCount >= RX_BUFFERSIZE) {
		periodSaveCount = 0;
	}
}
void Tim3IT() {
	if (mode) {
		HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
		midbit = false;
	} else {
		if(sampusecount>SAMP_PER_BAUD){
			loadPeriodBuffer(0);
		}
		sampusecount++;
	}
}
void FreqCounterPinEXTI() {
	loadPeriodBuffer(htim2.Instance->CNT);
	htim2.Instance->CNT = 0;
	sampusecount = 0;
}

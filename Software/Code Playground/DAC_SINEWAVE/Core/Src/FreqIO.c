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

//General Program
//****************************************************************************************************************
bool mode = false;
bool midbit = false;
bool changeMode = false;

void initProgram() {
	initOUTData();

	//Set hardware properly
	toggleMode();
	toggleMode();

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

//GENERATING FREQ
//****************************************************************************************************************
bool bitStream[10];

uint32_t lowFrequency[2 * LOWF_SAMP];
uint32_t highFrequency[2 * HIGHF_SAMP];

void edit_sineval(uint32_t *sinArray, int arraySize, int waves, float shiftPercent) {
	double ampl 		= OUT_AMPL / 2;						//Amplitude of wave
	double phaseShift 	= shiftPercent * 2 * PI;	//Desired phase shift
	double w 			= 2 * PI  * waves / arraySize;

	for (int i = 0; i < arraySize; i++) {
		//formula in DAC Document
		sinArray[i] = (sin((i * w) + phaseShift) + 1) * ampl;
	}
}
void bitToAudio(bool *bitStream, int arraySize, bool direction) {
	if(direction){//transmitting lsb first
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
	else{ //transmitting msb first
		for (int i = arraySize-1; i >= 0; i--) {
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
	edit_sineval(lowFrequency, 2 * LOWF_SAMP, 2, +0.90);
	edit_sineval(highFrequency, 2 * HIGHF_SAMP, 2, +0.99);
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
		else {
			bitBuffer[bitSaveCount] = -1;
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
int loadOctet(bool* bufferptr) {
	int bit;
	bool myPtr[8];
	bool isFlag = true;

	for (int i = 0; i < 8; i++) {
		bit = loadBit();
        if(bit < 0){
        	return -1;
        }
		myPtr[i] = bit;
        if(myPtr[i] != AX25TBYTE[i]){
        	isFlag = false;
        }
    }
	//If this is not a flag, copy the values into the buffer pointer
	if(!isFlag){
		sprintf(uartData, "Printing octet = ");
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		for(int i = 0;i<8;i++){
			bufferptr[7-i] = (myPtr[7-i]==1)?true:false;
			rxBit_count++;
			sprintf(uartData, " %d ",bufferptr[7-i]);
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
		sprintf(uartData, "\r\n");
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	return isFlag;
}
int streamGet() {
	struct PACKET_STRUCT* local_packet = &global_packet;

	int byteArray[8];
	int max_octets = (int)(AX25_PACKET_MAX)/8;
	int octet_count,good_octet;
	bool gotflag;

	//Just do this unless we need to toggle
	while(!changeMode){
		gotflag = false;

		//Slide bits
		for(int i = 0; i < 7; i++){
			byteArray[i] = byteArray[i+1];
		}
		byteArray[7] = loadBit();
		//sprintf(uartData, "Got bit %d\r\n",byteArray[7]);
		//HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		//Detect AX25 flag bytes
		for(int i = 0;i < 8; i++){
			//If the byte isn't lined up, break loop
			if(byteArray[i]!=AX25TBYTE[i]) {
				gotflag = false;
				break;
			}
			//If the loop makes it to the lowest bit, the flag should be lined up
			else if(i==7){
				gotflag = true;
			}
		}

		//Got flag
		if(gotflag){
			sprintf(uartData, "Start AX.25 Flag Detected\r\n");
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
			octet_count  = 0;

			//Until AX.25 buffer overflows, continue reading octets
			good_octet = 0;
			while( (good_octet==0) && (octet_count < max_octets) ){
				good_octet = loadOctet(&local_packet->AX25_temp_buffer[octet_count*8]);
				//sprintf(uartData, "Loaded octet %d out of %d\r\n",octet_count,max_octets);
				//sprintf(uartData, "good_octet: %d\r\n",good_octet);
				//HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

				octet_count+=1;
			}

			//If ax.25 buffer overflows or an octet was bad, this was a bad packet
			if((octet_count >= max_octets) || (good_octet!=1)){
				sprintf(uartData, "Bad packet! Not enough octets\r\n\n",octet_count);
				HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
			}
			//
			else if(octet_count == 1){
				sprintf(uartData, "Stop AX.25 Flag Detected\r\n");
				HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
				sprintf(uartData, "Bad packet! Not enough octetes.\r\n\n",octet_count);
				HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
			}
			//If ax.25 buffer does not overflow, this was a good packet
			else {
				sprintf(uartData, "Stop AX.25 Flag Detected\r\n\n");
				HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

				return 1;
			}
		}
		//Didn't get flag
		else {
			//sprintf(uartData, "Flag not detected\r\n");
			//HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
	}
	//Break if mode needs to change
	if(toggleMode)
		return -1;
}

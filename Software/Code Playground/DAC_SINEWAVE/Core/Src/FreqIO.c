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
bool mode;
bool midbit = false;
bool changeMode = false;

void initProgram(bool modeStart) {
	initOUTData();

	//Set hardware properly
	mode = modeStart;
	toggleMode();
	toggleMode();

	if (mode) {
		htim2.Instance->ARR = TIM2_AUTORELOAD_TX;
		htim3.Instance->ARR = TIM3_AUTORELOAD_TX;
	} else {
		htim2.Instance->ARR = TIM2_AUTORELOAD_RX;
		htim3.Instance->ARR = TIM3_AUTORELOAD_RX;
	}

	init_AX25();
}



void toggleMode() {
	//Disable HW interrupt
	HAL_NVIC_DisableIRQ(EXTI0_IRQn);

	//Toggle mode
	mode = !mode;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, mode);
	midbit = false;

	//Stop timer and reset count
	HAL_TIM_Base_Stop(&htim3);
	htim3.Instance->CNT = 0;

	HAL_TIM_Base_Stop(&htim4);
	htim4.Instance->CNT = 0;

	if (mode) {
		//Set Timer periods
		//htim2.Instance->ARR = TIM2_AUTORELOAD_TX; This is no longer used
		htim3.Instance->ARR = TIM3_AUTORELOAD_TX;

	} else {
		//Set Timer Periods
		htim2.Instance->ARR = TIM2_AUTORELOAD_RX;
		htim3.Instance->ARR = TIM3_AUTORELOAD_RX;
		HAL_TIM_Base_Start(&htim4);

		//Enable tim3 interrupt
		HAL_TIM_Base_Start_IT(&htim3);

		//Stop DAC
		HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);

		//Enable HW interrupt
		HAL_NVIC_EnableIRQ(EXTI0_IRQn);
	}

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0);
}
bool bufffull = false;
void loadPeriodBuffer(int timerCnt) {
	if(canWrite){
		periodBuffer[periodSaveCount] = timerCnt;
		periodSaveCount++;
		if (periodSaveCount >= RX_BUFFERSIZE) {
			periodSaveCount = 0;
		}

		//Buffer is full
		if(periodSaveCount == periodReadCount){
			canWrite = false;
		}
	} else {
		bufffull = true;
	}
	canRead = true;
}

int readPeriodBuffer(){
	int returnVal = -1;

	if(canRead){
		//Extract buffer value
		returnVal = periodBuffer[periodReadCount];

		if(returnVal == -1){
			sprintf(uartData, "End of incoming stream\n");
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}

		//Update period read
		periodReadCount++;
		if (periodReadCount >= RX_BUFFERSIZE) {
			periodReadCount = 0;
		}

		//Buffer is empty
		if(periodReadCount == periodSaveCount){
			canRead = false;
		}

	} else {
//		sprintf(uartData, "periodBuffer is empty\n");
//		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	if(bufffull){
		sprintf(uartData, "periodBuffer is full; periodSaveCount = %d\n",periodSaveCount);
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	bufffull = false;
	canWrite = true;
	return returnVal;
}
void Tim3IT() {
	if (mode) {
		midbit = false;
	} else {
		//If signal is determined to be invalid, load 0
		if(signal_detect_decay <= 0){
			if(signal_valid){
				loadPeriodBuffer(-1);
				HAL_TIM_Base_Stop(&htim4);
				htim4.Instance->CNT = 0;
			}
			signal_valid = false;
		}

		else {
			signal_detect_decay--;
		}
	}
}
int edges = 0;
int gotVal = 0;
int last_carrier_tone = 0;
int carrier_tone = 0;

void FreqCounterPinEXTI() {
	//Measure time since last measurement
	gotVal = htim2.Instance->CNT;
	int freq = PCONVERT / (gotVal);
	loadPeriodBuffer(gotVal);

	last_carrier_tone = carrier_tone;

	//2200Hz detected
	if ( ((HIGHFREQ - FREQDEV) < freq) && (freq < (HIGHFREQ + FREQDEV)) ){
		carrier_tone = HIGHFREQ;
	}
	//1200Hz detected
	else if ( ((LOWFREQ - FREQDEV) < freq) && (freq < (LOWFREQ + FREQDEV)) ){
		carrier_tone = LOWFREQ;
	}
	//Invalid freq
	else{
		carrier_tone = -1;
//		sprintf(uartData, "bad frequency detected, frequency was %d\n",freq);
//		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}

	if(carrier_tone!=last_carrier_tone || carrier_tone == -1){
		edges = 0;
		htim4.Instance->CNT = 0;
	} else {
		edges++;
	}

	signal_valid = true;
	signal_detect_decay = DECAY_TIME;
	htim2.Instance->CNT = 0;
}

//GENERATING FREQ
//****************************************************************************************************************
bool bitStream[10];
bool freqSelect = false;

void edit_sineval(uint32_t *sinArray, int arraySize, int waves, float shiftPercent) {
	double ampl 		= OUT_AMPL / 2;						//Amplitude of wave
	double phaseShift 	= shiftPercent * 2 * PI;	//Desired phase shift
	double w 			= 2 * PI  * waves / arraySize;

	for (int i = 0; i < arraySize; i++) {
		//formula in DAC Document
		sinArray[i] = (sin((i * w) + phaseShift) + 1) * ampl;
		sprintf(uartData, "sinArray[%d] = %d\n",i,sinArray[i]);
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
}

int bitToAudio(bool *bitStream, int arraySize, bool direction,int wave_start) {
	bool changeFreq;
	int waveoffset = wave_start;
	for (int i = 0; i < arraySize; i++) {
		//Check if freq needs to be changed for NRZI
		if(direction){
			changeFreq = bitStream[i];
		} else {
			changeFreq = bitStream[arraySize - i - 1];
		}

		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, changeFreq);
		//freqSelect = changeFreq;
		freqSelect = (changeFreq) ? freqSelect : !freqSelect;

		if (freqSelect) {
			htim2.Instance->ARR = 14;
			waveoffset = (1.0 * FREQ_SAMP) * (1.0 * HIGHF) / (1.0 * LOWF);
		}
		else {
			htim2.Instance->ARR = 27;
			waveoffset = (1.0 * FREQ_SAMP) * (1.0 * LOWF) / (1.0 * LOWF);
		}

		//htim2.Instance->CNT = 0;
		//HAL_TIM_Base_Stop(&htim2);
		HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (wave+wave_start), FREQ_SAMP, DAC_ALIGN_12B_R);
		htim3.Instance->CNT = 0;
		HAL_TIM_Base_Start_IT(&htim3);

		//Calculate ending point for wave
		wave_start = (wave_start+waveoffset+1)%FREQ_SAMP;

		midbit = true;
		while (midbit){
			//In the future this leaves the CPU free for scheduling or something
			__NOP();
		}

	}

	HAL_TIM_Base_Stop(&htim3);
	return wave_start;
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
	bitStream[8] = 1;
	bitStream[9] = 0;

}
void initOUTData() {
	//edit_sineval(lowFrequency, 2 * LOWF_SAMP, 2, +0.995);
	//edit_sineval(highFrequency, 2 * HIGHF_SAMP, 2, +0.99);
	generateBitstream();
}

//READING FREQ
//****************************************************************************************************************
uint32_t periodBuffer[RX_BUFFERSIZE];
uint32_t bitBuffer[RX_BUFFERSIZE];
bool		canWrite = true;
bool		canRead  = false;
uint16_t periodSaveCount = 0;
uint16_t periodReadCount = 0;
uint16_t	signal_detect_decay = 0;			//Pseudo timer to detect if value is valid
bool		signal_valid = false;					//Determines if frequency being read is a valid bit
uint16_t trackBit = 0;
uint16_t bitSaveCount = 0;

int pertobit(uint32_t inputPeriod) {
	int freq = PCONVERT / inputPeriod;

//	sprintf(uartData, "Recieved frequency = %d\r\n",freq);
//	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	//return freq;
	if ((HIGHFREQ - FREQDEV < freq) && (freq < HIGHFREQ + FREQDEV)){
		sprintf(uartData, "Recieved frequency = %d\r\n",freq);
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		return 1;
	}
	else if ((LOWFREQ - FREQDEV < freq) && (freq < LOWFREQ + FREQDEV)){
		sprintf(uartData, "Recieved frequency = %d\r\n",freq);
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		return 0;
	}
	else
//		sprintf(uartData, "Recieved frequency = %d\r\n",freq);
//		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		return -1;
}
int loadBit(){
	int startbit;
	int currbit = -1;
	int loopCount = 0;
	int checkCount;
	bool goodbit = false;

	startbit = pertobit(periodBuffer[trackBit]);
//	sprintf(uartData, "startbit = %d\n",startbit);
//	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	//Increment trackBit
	trackBit++;
	if (trackBit >= RX_BUFFERSIZE)
		trackBit = 0;

	if(startbit==1){
		checkCount = 3;
	}
	else if(startbit==0){
		checkCount = 1;
	}
	else {
		checkCount = 0;
	}

	//Valiate startbit value
	while(loopCount<checkCount){
		currbit = pertobit(periodBuffer[trackBit]);

		//Good bit
		if(startbit==currbit){
			goodbit = true;
		}
		//Bad bit
		else {
			currbit = -1;
			goodbit = false;
			break;
		}

		//Increment trackBit
		trackBit++;
		if (trackBit >= RX_BUFFERSIZE)
			trackBit = 0;
		loopCount++;
	}

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

		sprintf(uartData, "bit[%d] = %d \n",i,bit);
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

        if(bit < 0){
    		sprintf(uartData, "bit %d was bad\n",i);
    		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
        	return -1;
        }
		myPtr[i] = bit;
        if(myPtr[i] != AX25TBYTE[i]){
        	isFlag = false;
        }
    }
	//If this is not a flag, copy the values into the buffer pointer
	if(!isFlag){
		sprintf(uartData, "Printing octet [MSB:LSB]= ");
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
//		if(byteArray[7]>=0){
//			sprintf(uartData, "Got bit %d\r\n",byteArray[7]);
//			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
//		}
//		sprintf(uartData, "Current octet:");
//		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		//Detect AX25 flag bytes
		for(int i = 0;i < 8; i++){
//			sprintf(uartData, " %d ",byteArray[i]);
//			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

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
//		sprintf(uartData, "\n");
//		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);


		//Got flag
		if(gotflag){
			sprintf(uartData, "Start AX.25 Flag Detected\r\n");
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
			octet_count  = 0;

			//Until AX.25 buffer overflows, continue reading octets
			good_octet = 0;
			while( (good_octet==0) && (octet_count < max_octets) ){
				good_octet = loadOctet(&local_packet->AX25_PACKET[octet_count*8]);
				sprintf(uartData, "Loaded octet %d out of %d\r\n",octet_count,max_octets);
				//sprintf(uartData, "good_octet: %d\r\n",good_octet);
				HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

				octet_count+=1;
			}
			//If an octet was bad, this was a bad packet
			if(good_octet!=1){
				sprintf(uartData, "Bad packet! Detected bad signal.\n\n",octet_count);
				HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
				//for(int i = 0;i<)
				gotflag = false;
			}
			//If ax.25 buffer overflows
			else if(octet_count >= max_octets){
				sprintf(uartData, "Bad packet! Not enough octets\r\n\n",octet_count);
				HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
				gotflag = false;
			}
			//
			else if(octet_count == 1){
				sprintf(uartData, "Stop AX.25 Flag Detected\r\n");
				HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
				sprintf(uartData, "Bad packet! Not enough octetes.\r\n\n",octet_count);
				HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
				gotflag = false;
			}
			//If ax.25 buffer does not overflow, this was a good packet
			else {
				sprintf(uartData, "Stop AX.25 Flag Detected\r\n\n");
				HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
				gotflag = false;
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

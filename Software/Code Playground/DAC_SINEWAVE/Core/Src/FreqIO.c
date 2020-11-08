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
TIM_HandleTypeDef htim5;
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

	//Set hardware properly
	mode = modeStart;
	toggleMode();
	toggleMode();

	init_UART();
}

void toggleMode() {

	//Toggle mode
	mode = !mode;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, mode);

	//Stop DAC
	HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
	midbit = false;

	//Stop Timers the Correct Way
	HAL_TIM_OC_Stop_IT(&htim2, TIM_CHANNEL_1);
	HAL_TIM_Base_Stop(&htim3);
	HAL_TIM_IC_Stop_IT(&htim5, TIM_CHANNEL_1);

	//Zero Timers
	htim2.Instance->CNT = 0;
	htim3.Instance->CNT = 0;
	htim5.Instance->CNT = 0;

	//Transmission Mode
	if (mode) {

		//Set Timer Auto Reload Settings
		htim2.Instance->ARR = TIM2_AUTORELOAD_TX_LOW;
		htim3.Instance->ARR = TIM3_AUTORELOAD_TX;
		htim5.Instance->ARR = TIM5_AUTORELOAD_TX;

		//Start Timers the Correct Way
		//Nothing to do here
	}

	//Receiving Mode
	else {

		//Set Timer Auto Reload Settings
		htim2.Instance->ARR = TIM2_AUTORELOAD_TX_LOW;
		htim3.Instance->ARR = TIM3_AUTORELOAD_TX;
		htim5.Instance->ARR = TIM5_AUTORELOAD_TX;

//		//Start Timers the Correct Way
		HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1);
		HAL_TIM_IC_Start_IT(&htim5, TIM_CHANNEL_1);
	}
}

/*
 * Transmitting	: 				0x73 = 0111 0011 -> sent in this order [firstbit:lastbit] -> 11001110
 * Receiving[firstrx:lastrx] :	11001110 -> reverse it -> [MSB:LSB] 0111 0011
 *
 * Shifting method
 * 01234567
 * --------
 * 00000000
 * 10000000
 * 11000000
 * 01100000
 * 00110000
 * 10011000
 * 11001100
 * 11100110
 * 01110011
 *
 * Incrementing method
 * 01234567
 * --------
 * 00000000
 * 10000000
 * 11000000
 * 11000000
 * 11000000
 * 11001000
 * 11001100
 * 11001110
 * 11001110
 */

bool bufffull = false;
int loadBitBuffer(bool bit_val) {
	if(canWrite){
		bitBuffer[bitSaveCount] = bit_val;
		bitSaveCount++;
		if (bitSaveCount >= RX_BUFFERSIZE) {
			canWrite = false;
		}

		//Buffer is full
		if(bitSaveCount == bitReadCount){
			canWrite = false;
		}
	} else {
		bufffull = true;
	}
	canRead = true;
	return bitSaveCount;
}

int readBitBuffer(){
	int returnVal = -1;

	if(canRead){
		//Extract buffer value
		returnVal = bitBuffer[bitReadCount];

		if(returnVal == -1){
			sprintf(uartData, "End of incoming stream\n");
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}

		//Update period read
		bitReadCount++;
		if (bitReadCount >= RX_BUFFERSIZE) {
			canRead = false;
		}

		//Buffer is empty
		if(bitReadCount == bitSaveCount){
			canRead = false;
		}

	} else {
//		sprintf(uartData, "periodBuffer is empty\n");
//		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	if(bufffull){
		sprintf(uartData, "bitBuffer is full; bitSaveCount = %d\n",bitSaveCount);
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	bufffull = false;
	canWrite = true;
	return returnVal;
}
void resetBitBuffer(){
	bitReadCount = 0;
	bitSaveCount = 0;

	canRead  = false;
	canWrite = true;

	bufffull = false;
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
			htim2.Instance->ARR = TIM2_AUTORELOAD_TX_HIGH;
			waveoffset = (1.0 * FREQ_SAMP) * (1.0 * HIGHF) / (1.0 * LOWF);
		}
		else {
			htim2.Instance->ARR = TIM2_AUTORELOAD_TX_LOW;
			waveoffset = (1.0 * FREQ_SAMP) * (1.0 * LOWF) / (1.0 * LOWF);
		}

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

//READING FREQ
//****************************************************************************************************************
bool bitBuffer[RX_BUFFERSIZE];
bool		canWrite = true;
bool		canRead  = false;
uint16_t bitSaveCount = 0;
uint16_t bitReadCount = 0;
uint16_t	signal_detect_decay = 0;			//Pseudo timer to detect if value is valid
bool		signal_valid = false;					//Determines if frequency being read is a valid bit
uint16_t trackBit = 0;

//int loadOctet(bool* bufferptr) {
//	int bit;
//	bool myPtr[8];
//	bool isFlag = true;
//
//	for (int i = 0; i < 8; i++) {
//		bit = loadBit();
//
//		sprintf(uartData, "bit[%d] = %d \n",i,bit);
//		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
//
//        if(bit < 0){
//    		sprintf(uartData, "bit %d was bad\n",i);
//    		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
//        	return -1;
//        }
//		myPtr[i] = bit;
//        if(myPtr[i] != AX25TBYTE[i]){
//        	isFlag = false;
//        }
//    }
//	//If this is not a flag, copy the values into the buffer pointer
//	if(!isFlag){
//		sprintf(uartData, "Printing octet [MSB:LSB]= ");
//		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
//
//		for(int i = 0;i<8;i++){
//			bufferptr[7-i] = (myPtr[7-i]==1)?true:false;
//			rxBit_count++;
//			sprintf(uartData, " %d ",bufferptr[7-i]);
//			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
//		}
//		sprintf(uartData, "\r\n");
//		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
//	}
//	return isFlag;
//}

int streamGet() {
	struct PACKET_STRUCT* local_packet = &global_packet;

	int byteArray[8];
	int max_octets = (int)(AX25_PACKET_MAX)/8;
	int octet_count,good_octet;
	bool gotflag = false;//rxBit_count

	//Search for end of packet
	for(int i = rxBit_count-1;i > 0; i--){
		byteArray[i] = local_packet->AX25_PACKET[i];
	}
	//Read through AX.25 array
	for(int i = 0;i < rxBit_count; i++){
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

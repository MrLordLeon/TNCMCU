/*
 * FreqIO.h
 *
 *  Created on: Jul 21, 2020
 *      Author: monke
 *
 *      This file is dedicated to functions for creating and reading frequencies.
 */

#ifndef SRC_FREQIO_H_
#define SRC_FREQIO_H_
#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "math.h"
#include "sine.h"
#include "AX.25.h"
#include "interrupt_services.h"
#include "FreqIO.h"

//Needed uController Objects
//****************************************************************************************************************
extern DAC_HandleTypeDef hdac;
extern DMA_HandleTypeDef hdma_dac1;
//Connectivity
//****************************************************************************************************************
extern char uartData[3000];

//General Program
//****************************************************************************************************************
/*
 * 	Bool used to determine if in TX or RX mode.
 * 	TX = 1
 * 	RX = 0
 *
 */
extern bool mode;

void initProgram(bool modeStart);
void toggleMode();

int loadBitBuffer(bool bit_val);
int readBitBuffer();
void resetBitBuffer();

//GENERATING FREQ
//****************************************************************************************************************
#define TIM2_AUTORELOAD_TX_LOW	108			//Timer2 period, used to control DAC and generate 1200Hz, assuming 40Mhz clk
#define TIM2_AUTORELOAD_TX_HIGH	56			//Timer2 period, used to control DAC and generate 2200Hz, assuming 40Mhz clk
#define PI 					3.1415926
#define OUT_AMPL			4096
#define LOWF 				1200 		//This is the sample count for the low frequency , as configured maps to 1200Hz
#define HIGHF				2200		//This is the sample count for the high frequency, as configured maps to 2200Hz

extern bool 	midbit;
extern bool		changeMode;
extern bool		freqSelect;						//Tracks lasts state of output freq for NRZI encoding

void edit_sineval(uint32_t *sinArray, int arraySize, int waves, float shiftPercent);
int bitToAudio(bool *bitStream, int arraySize, bool direction,int wave_start);
void generateBitstream();
void initOUTData();

//READING FREQ
//****************************************************************************************************************
#define	BUFFER_SCALE		1024			//Scalar for buffer base
#define BIT_BUFF_BASE		16			//Base amount of bits to store
#define RX_BUFFERSIZE		2655

extern bool bitBuffer[RX_BUFFERSIZE];		//Stores bitstream values
extern bool		canWrite;
extern bool		canRead;
extern bool 	bufffull;
extern uint16_t bitSaveCount;				//Keep track of index to save period
extern uint16_t bitReadCount;				//Keep track of index to read period
extern uint16_t	signal_detect_decay;			//Pseudo timer to detect if value is valid
extern bool		signal_valid;					//Determines if frequency being read is a valid bit
extern uint16_t trackBit;						//Tracks bits being loaded into bit buffer
extern uint16_t bitSaveCount;					//Tracks bits being saved into bit buffer
/*
 *	Function to convert freq to bitstream:
 *		returns 1 if detect higher freq within threshold
 *		returns 0 if detect lower freq within threshold
 *		return if frequency is outside afsk set range
 */
int pertobit(uint32_t inputPeriod);
/*
 *	Function to take period buffer values and loads the next bit into bit buffer.
 *	Also returns the determined bitvalue
 *	0 	= 1200Hz
 *	1  	= 2200Hz
 *	-1	= Invalid frequency
 */
int loadBit();
/*
 *	Loads an octet at a time.
 *	Returns -1 if an invalid frequency is detected
 *	Returns 0 if the octet is valid and not an ax.25 flag
 *	Returns 1 if the octet is the ax.25 flag
 */
int loadOctet(bool* bufferptr);
/*
 *	Fills the ax.25 buffer with octets excluding the flags
 *	Returns -1 if need to change mode
 *	Returns 1 if the packet was valid frequencies
 */
int streamGet();


#endif /* SRC_FREQIO_H_ */
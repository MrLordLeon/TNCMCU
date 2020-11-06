/*
 * interrupt_services.c
 *
 *  Created on: Nov 5, 2020
 *      Author: monke
 */

#include "interrupt_services.h"
#include "FreqIO.h"
#include "AX.25.h"

uint8_t captured_bits_count = 0;	//How many captured bits since last clk sync
bool clk_sync = false;			//Are we synced with clock

bool freq_pin_state_curr = false;
bool freq_pin_state_last = false;

bool hold_state;
bool NRZI;

//Timer 2 Output Compare Callback
void Tim2_OC_Callback(){
	freq_pin_state_last = hold_state;

	//Check if this is valid data
	if(clk_sync){
		uint32_t this_capture = __HAL_TIM_GET_COMPARE(&htim2, TIM_CHANNEL_1);
		uint32_t next_sampl = this_capture + bit_sample_period;
		NRZI = (freq_pin_state_curr==freq_pin_state_last) ? 1 : 0;

		HAL_GPIO_TogglePin(GPIOA,D0_Pin);//Interpreted clock

		HAL_GPIO_WritePin(GPIOA,D1_Pin,freq_pin_state_curr);
		HAL_GPIO_WritePin(GPIOB,D2_Pin,freq_pin_state_last);
		HAL_GPIO_WritePin(GPIOB,D3_Pin,NRZI);

		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1,next_sampl); // if we have not received a transition to the input capture module, we want to refresh the output compare module with the last known bit period
	}
	//Clock not syncd
	else
	{
		//Turn off OC until syncd
		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1,0);
	}

	//Inc number of bits since last clock sync
	captured_bits_count++;
	if(captured_bits_count >= samp_per_bit * no_clk_max_cnt){
		clk_sync = false;	//Clock is no longer sync
	}

	hold_state = freq_pin_state_curr;

	return;
}
void Tim3_IT_Callback() {
	if (mode) {
		midbit = false;
	}
	//Timer 3 does nothing in RX
	else {}
}
//Timer 5 Input Capture Callback
void Tim5_IC_Callback(){
	static uint32_t rising_capture = 0;		// stores the timer tick value of the most recent rising edge
	static uint32_t falling_capture = 0;	// stores the timer tick value of the most recent falling edge
	static uint32_t capture_difference = 0;
	static bool rise_captured = false;		// these are used to ensure that we aren't trying to compute the difference
	static bool fall_captured = false;		// before we have captured both a rising and falling edge
	static bool signal_edge = RISING_EDGE;	// so we know what edge we are looking for (really, the opposite of the edge that was captured last

	uint32_t this_capture = 0;		// simply stores either the rising or falling capture, based on which state we are in (avoids duplicate code)
	//Grap pin state for OC timer
	freq_pin_state_curr = signal_edge;

	//Rising Edge
	if (signal_edge)
	{
		rising_capture = HAL_TIM_ReadCapturedValue(&htim5, TIM_CHANNEL_1); //Time-stamp interrupt
		signal_edge = FALLING_EDGE;		// look for falling edge on next capture
		rise_captured = true;

		if (rise_captured && fall_captured)
		{
			capture_difference = rising_capture - falling_capture;		// calculate difference
			this_capture = rising_capture;		// set current sample to rising edge
		}
	}

	//Falling edge
	else
	{
		falling_capture = HAL_TIM_ReadCapturedValue(&htim5, TIM_CHANNEL_1);		//Time-stamp interrupt
		fall_captured = true;
		signal_edge = RISING_EDGE;		// look for rising edge on next capture

		if (rise_captured && fall_captured)
		{
			capture_difference = falling_capture - rising_capture;		// calculate difference
			this_capture = falling_capture;
		}
	}

	//Have now captured the transition period
	//Can use this to align sampling clock
	if (rise_captured && fall_captured)
	{
		//Check if the transition was a valid transition period to use
		if(SYMBOL_PERIOD-SYMBOL_MARGIN < capture_difference && capture_difference < SYMBOL_PERIOD+SYMBOL_MARGIN){

			//Predict clock
			uint32_t next_sampl;

			if(!clk_sync){
				next_sampl = this_capture + SYMBOL_PERIOD;
				__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, next_sampl);
			}
			else {
				next_sampl = this_capture + bit_sample_period;
				__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, next_sampl);
			}
			//Reset roll-over value
			captured_bits_count = 0;

			//Have now synced with clock
			clk_sync = true;
		}
	}
}

void init_UART(){
	HAL_UART_Receive_IT(&huart2, &(UART_packet.input), UART_RX_IT_CNT);
	UART_packet.flags = 0;
	UART_packet.got_packet = false;
	UART_packet.rx_cnt = 0;
	UART_packet.received_byte_cnt = 0;
}
void UART2_Exception_Callback(){
	HAL_UART_Receive_IT(&huart2, &(UART_packet.input), UART_RX_IT_CNT);//Reset
	UART_packet.got_packet = false;

	  if(UART_packet.input==0xc0){
		  UART_packet.flags++;
	  }

	  *(UART_packet.HEX_KISS_PACKET+UART_packet.rx_cnt) = UART_packet.input;
	  UART_packet.rx_cnt++;

	  if(UART_packet.flags>=2){
		  if(!mode){
			  changeMode = true;
		  }
		  UART_packet.flags = 0;
		  UART_packet.got_packet = true;
		  UART_packet.received_byte_cnt = UART_packet.rx_cnt;
		  UART_packet.rx_cnt=0;

	  }
}

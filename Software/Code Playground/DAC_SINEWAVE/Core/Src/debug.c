/*
 * debug.c
 *
 *  Created on: Nov 1, 2020
 *      Author: monke
 */
#include "debug.h"

void print_AX25(){
	struct PACKET_STRUCT* local_packet = &global_packet;
	int bytecnt = local_packet->byte_cnt;
	bool *curr_mem;
	sprintf(uartData, "\nPrinting AX25_PACKET... All fields printed [MSB:LSB]\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	//Print Address Field
	curr_mem = (local_packet->AX25_PACKET) + address_len - 1;
	for(int i = 0;i<address_len/8;i++){
		sprintf(uartData, "Address Field %d =",i+1);
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		for(int j = 0;j<8;j++){
			sprintf(uartData, " %d ",*(curr_mem-j));
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
		curr_mem -= 8;
		sprintf(uartData, "\n");
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}

	//Print Control Field
	curr_mem += address_len;//Subtract 8 to start at the flag start
	sprintf(uartData, "Control Field   =");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	for(int i = 0;i<8;i++){
		sprintf(uartData, " %d ",*(curr_mem+8-i-1));
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	sprintf(uartData, "\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	//PID
	curr_mem += control_len;//Subtract 8 to start at the flag start
	sprintf(uartData, "PID Field       =");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	for(int i = 0;i<8;i++){
		sprintf(uartData, " %d ",*(curr_mem+8-i-1));
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	sprintf(uartData, "\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	curr_mem += PID_len;

	//Print Info Field
	curr_mem += local_packet->Info_Len - 1;
	for(int i = 0;i<(local_packet->Info_Len/8);i++){
		sprintf(uartData, "Info Field %d    =",i+1)	;
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		for(int j = 0;j<8;j++){
			sprintf(uartData, " %d ",*(curr_mem-j));
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
		curr_mem -= 8;
		sprintf(uartData, "\n");
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	curr_mem += local_packet->Info_Len;

	curr_mem += FCS_len - 8;
	for(int i = 0;i<(FCS_len/8);i++){
		sprintf(uartData, "FCS Field %d     =",i+1)	;
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		for(int j = 0;j<8;j++){
			sprintf(uartData, " %d ",*(curr_mem+8-j-1));
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
		curr_mem -= 8;
		sprintf(uartData, "\n");
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
}

void print_outAX25(){
	struct PACKET_STRUCT* local_packet = &global_packet;
	int bytecnt = local_packet->byte_cnt;
	bool *curr_mem;
	sprintf(uartData, "\nPrinting AX25_PACKET being sent to radio\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	//Print Address Field
	curr_mem = local_packet->address;
	for(int i = 0;i<address_len/8;i++){
		sprintf(uartData, "Address Field %d =",i+1);
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		for(int j = 0;j<8;j++){
			sprintf(uartData, " %d ",*(curr_mem+j));
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
		curr_mem += 8;
		sprintf(uartData, "\n");
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}

	//if address was bitstuffed then print rest of address field
	sprintf(uartData, "Address Field extra = ");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	curr_mem += address_len;
	if(local_packet->stuffed_address > 0){
		for(int i = 0; i < local_packet->stuffed_address; i++){
			sprintf(uartData, " %d ",*(curr_mem-i));
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
	}
	sprintf(uartData, "\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	//Print Control Field
	curr_mem = local_packet->control;//Subtract 8 to start at the flag start
	sprintf(uartData, "Control Field   =");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	for(int i = 0;i<control_len + local_packet->stuffed_control;i++){
		sprintf(uartData, " %d ",*(curr_mem+i));
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	sprintf(uartData, "\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	//PID
	curr_mem = local_packet->PID;//Subtract 8 to start at the flag start
	sprintf(uartData, "PID Field       =");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	for(int i = 0;i<PID_len + local_packet->stuffed_PID;i++){
		sprintf(uartData, " %d ",*(curr_mem+i));
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	sprintf(uartData, "\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	//Print Info Field
	curr_mem = local_packet->Info;
	for(int i = 0;i<(local_packet->Info_Len/8);i++){
		sprintf(uartData, "Info Field %d    =",i+1)	;
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		for(int j = 0;j<8;j++){
			sprintf(uartData, " %d ",*(curr_mem+j));
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
		curr_mem += 8;
		sprintf(uartData, "\n");
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}

	//if Info was bitstuffed then print rest of address field
	sprintf(uartData, "Info Field extra = ");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	curr_mem += local_packet->Info_Len;
	if(local_packet->stuffed_Info > 0){
		for(int i = 0; i < local_packet->stuffed_Info; i++){
			sprintf(uartData, " %d ",*(curr_mem+i));
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
	}
	sprintf(uartData, "\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	curr_mem = local_packet->FCS;
	sprintf(uartData, "FCS Field     =")	;
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	for(int i = 0;i<FCS_len+local_packet->stuffed_FCS;i++){
		sprintf(uartData, " %d ",*(curr_mem+i));
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}

	sprintf(uartData, "\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);



	//reset bitstuff members
	local_packet->stuffed_address = 0;
	local_packet->stuffed_control = 0;
	local_packet->stuffed_PID = 0;
	local_packet->stuffed_Info = 0;
	local_packet->stuffed_FCS = 0;
	local_packet->bit_stuffed_zeros = 0;
}

void print_KISS(){
	struct PACKET_STRUCT* local_packet = &global_packet;
	int bytecnt = local_packet->byte_cnt;
	bool *curr_mem;
	sprintf(uartData, "\nPrinting KISS_PACKET... All fields printed [MSB:LSB]\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	//Print Start Flag
	curr_mem = (local_packet->address + address_len + 16 - 1);//start at the flag start
	sprintf(uartData, "Start flag      =");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	for(int i = 0;i<8;i++){
		sprintf(uartData, " %d ",*(curr_mem-i));
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	sprintf(uartData, "\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	curr_mem = (local_packet->address) + address_len - 1;
	for(int i = 0;i<address_len/8;i++){
		sprintf(uartData, "Address Field %d =",i+1);
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		for(int j = 0;j<8;j++){
			sprintf(uartData, " %d ",*(curr_mem-j));
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
		curr_mem -= 8;
		sprintf(uartData, "\n");
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}

	//Print Control Field
	curr_mem = (local_packet->control);//Subtract 8 to start at the flag start
	sprintf(uartData, "Control Field   =");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	for(int i = 0;i<8;i++){
		sprintf(uartData, " %d ",*(curr_mem+8-i-1));
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	sprintf(uartData, "\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	//PID
	curr_mem = (local_packet->PID);//Subtract 8 to start at the flag start
	sprintf(uartData, "PID Field       =");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	for(int i = 0;i<8;i++){
		sprintf(uartData, " %d ",*(curr_mem+8-i-1));
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	sprintf(uartData, "\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	//Print Info Field
	curr_mem = (local_packet->Info) + local_packet->Info_Len - 1;
	for(int i = 0;i<(local_packet->Info_Len/8);i++){
		sprintf(uartData, "Info Field %d    =",i+1)	;
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		for(int j = 0;j<8;j++){
			sprintf(uartData, " %d ",*(curr_mem-j));
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
		curr_mem -= 8;
		sprintf(uartData, "\n");
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}

	//Print Stop Flag
	curr_mem = local_packet->KISS_PACKET;
	sprintf(uartData, "Stop flag       =");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	for(int i = 0;i<8;i++){
		sprintf(uartData, " %d ",*(curr_mem+8-i-1));
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	sprintf(uartData, "\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

}
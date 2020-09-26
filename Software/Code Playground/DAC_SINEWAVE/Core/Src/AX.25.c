/*
 * AX.25.c
 *
 *  Created on: Sep 2, 2020
 *      Author: Kobe
 */
#include "AX.25.h"
#include "FreqIO.h"


//*************** variables for detecting and validating  AX.25  ******************************************************
bool AX25_flag = false; 						//indicates whether the TNC started reading for packets
int rxBit_count = 0; 							//keeps count of the temp buffer index

bool AX25TBYTE[FLAG_SIZE] = { 0, 1, 1, 1, 1, 1, 1, 0 };

//NEED TO SET LOCAL_ADDRESS
bool local_address[address_len/2];
//*********************************************************************************************************************

//**************** KISS *************************************************************************************************************
bool KISS_FLAG[FLAG_SIZE] = { 1, 1, 0, 0, 0, 0, 0, 0 };
//*************************************************************************************************************************************

//*************** AX.25 Fields ******************************************************************************************

//***********************************************************************************************************************

//General Program
//****************************************************************************************************************
void tx_rx() {
	if (changeMode) {
		changeMode = 0;
		toggleMode();
	}

	if (mode) {
		bitToAudio(&bitStream[0], 10,1);
	} else {
		for(int i = 0;i<10;i++){
			sprintf(uartData, "Running streamGet() %d time\r\n",i);
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

			streamGet();
		}
		HAL_Delay(1000);
	}
}

//************* Handle bits received from Radio *************************************************************************
/**
  * @brief  discards bit stuffed 0 and slide remaining bits over bits over
  *
  */

void receiving_AX25(){
	struct PACKET_STRUCT* local_packet = &global_packet;

	int packet_status;
	packet_status = streamGet();
	if(packet_status == 1){
		remove_bit_stuffing();
		bool AX25_IsValid = Packet_Validate();
		memset(local_packet->AX25_temp_buffer,0,AX25_PACKET_MAX);
		if(AX25_IsValid){
			generate_KISS();
		}
		else{
			receiving_AX25();
		}
	}
	else{
		toggleMode();
	}
}

void transmitting_AX25(){
	HAL_GPIO_WritePin(PTT_GPIO_Port, PTT_Pin, GPIO_PIN_SET); //START PTT

	//Remember to format for KISS TO AX.25

	output_AX25();

	HAL_GPIO_WritePin(PTT_GPIO_Port, PTT_Pin, GPIO_PIN_RESET); //stop transmitting
}

void ouput_AX25(){
	struct PACKET_STRUCT* local_packet = &global_packet;

	bitToAudio(AX25TBYTE, FLAG_SIZE,1); //start flag

	bitToAudio(local_packet->address, address_len,0); //lsb first
	bitToAudio(local_packet->control,control_len,0);	//lsb first
	bitToAudio(local_packet->PID,PID_len,0);			//lsb first
	bitToAudio(local_packet->Info,local_packet->Info_Len,0);		//lsb first
	bitToAudio(local_packet->FCS,FCS_len,1);			//msb first

	bitToAudio(AX25TBYTE, FLAG_SIZE,1);//stop flag
}

void slide_bits(bool* array,int bits_left){
	memcpy(array,array+1,bits_left*bool_size);
}

void remove_bit_stuffing(){
	struct PACKET_STRUCT* local_packet = &global_packet;

	int one_count = 0;
	int shift_count = 0; //reduces loop count by number of bit stuffed zeros removed
	bool curr;
	for(int i = 0;i < KISS_SIZE-shift_count;i++){
		curr = local_packet->AX25_temp_buffer[i]; //iterate through all data received before seperating into subfields
		if(curr){ //current bit is a 1
			one_count++;
		}
		else{
			if(one_count >= 5){
				slide_bits(&local_packet->AX25_temp_buffer[i],rxBit_count-i);
				shift_count++;
				rxBit_count--;
			}
			one_count = 0;
		}
	}
	//transmit kiss
}

//NEED TO CREATE AN AX25_PACKET MEMBER FOR STRUCT
//COMPLETE MEMCOPY INSIDE GENERATE AX_25()
void generate_AX25(){
	struct PACKET_STRUCT* local_packet = &global_packet;

	bool *curr_mem = &local_packet->KISS_PACKET; //keep track of what address to copy from
	//this is assuming that the packet has all the subfields full

	sprintf(uartData,"Good Packet!");

	local_packet->address = curr_mem;
	if(!compare_address(local_packet->address)){
		return false; //discard
	}
	curr_mem += address_len;

	local_packet->control = curr_mem;
	curr_mem += control_len;

	//SHOULD CONSIDER A VAR IN STRUCT TO INDICATE THAT A PID FIELD IS PRESENT OR THAT THIS IS AN I FRAME
	if(local_packet->control[0] == 0){ // 0 = I frame, 01 = S frame, 11 = U Frame
		local_packet->PID = curr_mem;
		curr_mem += PID_len;
	}

	local_packet->Info = curr_mem;

	//USE CRC HERE TO GENERATE FCS FIELD
	//local_packet->FCS = curr_mem;

	return true; //valid packet
}

void generate_KISS(){
	struct PACKET_STRUCT* local_packet = &global_packet;

	bool *curr_mem = &local_packet->KISS_PACKET;

	memcpy(curr_mem,KISS_FLAG,FLAG_SIZE*bool_size);
	curr_mem += FLAG_SIZE;
	memcpy(curr_mem,local_packet->address,address_len*bool_size);
	curr_mem += address_len;
	memcpy(curr_mem,local_packet->control,control_len*bool_size);
	curr_mem += control_len;

	if(local_packet->control[0] == 0){ //information type packet only
		memcpy(curr_mem,local_packet->PID,PID_len*bool_size);
		curr_mem += PID_len;
	}

	memcpy(curr_mem,local_packet->Info,local_packet->Info_Len*bool_size);
	curr_mem += local_packet->Info_Len;
	memcpy(curr_mem,KISS_FLAG,FLAG_SIZE*bool_size);

	//remove bit stuffed zeros
	remove_bit_stuffing();
}

void generate_address(){
	for(int i = 0; i < address_len/2;i++){
		local_address[i] = (i%2) ? true : false;
	}
}

bool compare_address(bool *addr){
	generate_address();
	for(int i = 0;i < address_len/2;i++){
		if(addr[i] != local_address[i]){
			return false;
		}
	}
	return true;
}


//check if we keep the packet or remove it
bool Packet_Validate(){
	struct PACKET_STRUCT* local_packet = &global_packet;

	bool *curr_mem = &local_packet->AX25_temp_buffer; //keep track of what address to copy from

	if(rxBit_count < 120){ //invalid if packet is less than 136 bits - 2*8 bits (per flag)
		sprintf(uartData,"Trash Packet");
		return false;
	}
	else if((rxBit_count+1)%8 != 0){ //invalid if packet is not octect aligned (divisible by 8)
		sprintf(uartData,"Trash Packet");
		return false;
	}
	else{
		//this is assuming that the packet has all the subfields full
		int not_info = FCS_len; //number of bits in packet that aren't part of info field
		sprintf(uartData,"Good Packet!");

		local_packet->address = curr_mem;
		if(!compare_address(local_packet->address)){
			return false; //discard
		}
		curr_mem += address_len;
		not_info += address_len;

		local_packet->control = curr_mem;
		curr_mem += control_len;
		not_info += control_len;

		if(local_packet->control[0] == 0){ // 0 = I frame, 01 = S frame, 11 = U Frame
			local_packet->PID = curr_mem;
			curr_mem += PID_len;
			not_info += PID_len;
		}


		local_packet->Info_Len = rxBit_count - not_info;
		local_packet->Info = curr_mem;
		curr_mem += local_packet->Info_Len;

		local_packet->FCS = curr_mem;
	}
	return true; //valid packet
}

//---------------------- FCS Generation -----------------------------------------------------------------------------------------------
int crc = 0xFFFF; //initial crc value

//REMOVE THIS FLAG - KOBE
bool end_flag = false; //inidicates when to stop crc and perform one's compliment

//store bits in FCS field
void hex_to_bin(){
	struct PACKET_STRUCT* local_packet = &global_packet;

    int temp;
    for(int i = 0; i < 16; i++){ //stores in bits into fcs subfield
        temp = crc >> i;
        local_packet->FCS[i] = temp%2;
    }

    int time = htim2.Instance->CNT;
    sprintf(uartData,"FCS = %x\n",crc);
    HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
    sprintf(uartData,"\nExecution time = %d\n",time);
    HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
    crc = 0xFFFF; //reinitialize
    end_flag = 0;//reinitialize

}

//CRC Calculations
void crc_calc(int in_bit){
	int out_bit;
    int poly = 0x8408;             //reverse order of 0x1021
    if(!end_flag){
        out_bit = in_bit ^ (crc%2); //xor lsb of current crc with input bit
        crc >>= 1;                       //right shift by 1
        poly = (out_bit == 1) ? 0x8408 : 0;
        crc ^= poly;
    }
    else{
        crc ^= 0xFFFF; //one's compliment
        hex_to_bin();
    }
}

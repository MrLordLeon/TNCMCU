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
bool AX25_temp_buffer[AX25_PACKET_MAX]; 	//temperary stores bits received from radio, before formatting into AX.25 format
int packet_count = 0; 							//keeps count of the temp buffer index
bool buffer_rdy = true;
bool local_address[address_len/2];							//address set to this TNC

bool AX25TBYTE[FLAG_SIZE] = { 0, 1, 1, 1, 1, 1, 1, 0 };
//*********************************************************************************************************************

//**************** KISS *************************************************************************************************************
bool KISS_FLAG[FLAG_SIZE] = { 1, 1, 0, 0, 0, 0, 0, 0 };
bool KISS_PACKET[KISS_SIZE];
//*************************************************************************************************************************************

//*************** AX.25 Fields ******************************************************************************************
bool address[address_len];
bool control[control_len];
bool PID[PID_len]; //only for information subfield
bool Info[Info_len]; //only for information subfield
bool FCS[FCS_len];
//***********************************************************************************************************************

//General Program
//****************************************************************************************************************


//************* Handle bits received from Radio *************************************************************************
/**
  * @brief  discards bit stuffed 0 and slide remaining bits over bits over
  *
  */
void slide_bits(bool* array,int bits_left){
	memcpy(array,array+1,bits_left*bool_size);
}

void remove_bit_stuffing(){
	int one_count = 0;
	int shift_count = 0;
	bool curr;
	for(int i = 0;i < KISS_SIZE-shift_count;i++){
		curr = KISS_PACKET[i];
		if(curr){ //current bit is a 1
			one_count++;
		}
		else{
			if(one_count >= 5){
				slide_bits(&KISS_PACKET[i],KISS_SIZE-i);
				shift_count++;
			}
			one_count = 0;
		}
	}
	//transmit kiss
}

void generate_KISS(){
	bool *curr_mem = &KISS_PACKET;
	memcpy(curr_mem,KISS_FLAG,FLAG_SIZE*bool_size);
	curr_mem += FLAG_SIZE;
	memcpy(curr_mem,address,address_len*bool_size);
	curr_mem += address_len;
	memcpy(curr_mem,control,control_len*bool_size);
	curr_mem += control_len;

	if(packet_count > AX25_PACKET_MAX - PID_len){ //information type packet only
		memcpy(curr_mem,PID,PID_len*bool_size);
		curr_mem += PID_len;
	}

	memcpy(curr_mem,Info,Info_len*bool_size);
	curr_mem += Info_len;
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
void packetBit(){

}
//stores bits into buffer
void loadPacket(){

	//if(AX25)
	//AX25_temp_buffer[packet_count] = bit;
	if(packet_count < AX25_PACKET_MAX){
		packet_count++;
	}
	else{
		packet_count = 0;
	}
}

//check if we keep the packet or remove it
bool Packet_Validate(){
	bool *curr_mem = &AX25_temp_buffer; //keep track of what address to copy from

	if(packet_count < 120){ //invalid if packet is less than 136 bits - 2*8 bits (per flag)
		sprintf(uartData,"Trash Packet");
		return false;
	}
	else if((packet_count+1)%8 != 0){ //invalid if packet is not octect aligned (divisible by 8)
		sprintf(uartData,"Trash Packet");
		return false;
	}
	else{
		//this is assuming that the packet has all the subfields full
		sprintf(uartData,"Good Packet!");
		memcpy(address,curr_mem,address_len*bool_size);
		if(!compare_address(address)){
			return false; //discard
		}

		curr_mem += address_len;
		memcpy(control,curr_mem,control_len*bool_size);
		curr_mem += control_len;
		if(packet_count > AX25_PACKET_MAX - PID_len){ //information type packet only
			memcpy(PID,curr_mem,PID_len*bool_size);
			curr_mem += PID_len;
		}
		memcpy(Info,curr_mem,Info_len*bool_size);
		curr_mem += Info_len;
		memcpy(FCS,curr_mem,FCS_len*bool_size);
	}
	return true; //valid packet
}

//FCS Generation
unsigned long crc = 0;			//crc value after calculating data from PC
unsigned long poly = 0x11021;	//polynomial for crc calc
bool end_flag = 0;				//flag to indicate the end of a KISS Packet
int bit_count = 0;				//keeps count of how many bits to indicate when to start performing crc calc

//crc calculation process
void CRC_gen(){
    if(bit_count > 16 && !end_flag){
        poly = (crc > 0x10000) ? 0x11021 : 0;
        crc ^= poly;
    }
    else if(end_flag) {
        for(int i = 0;i < 16;i++){
            crc <<= 1;
            poly = (crc > 0x10000) ? 0x11021 : 0;
            crc ^= poly;
        }
    }

}

//converts bits to hex and runs crc calc while reading in bits simultaneously
void data_to_hex(unsigned int input){
    crc <<= 1;
    crc += input;
    bit_count++;
    CRC_gen();
}

//store bits in FCS field
void hex_to_bin(){
    int temp;
    for(int i = 0; i < 16; i++){
        temp = crc >> i;
        FCS[i] = temp%2;
    }
    int time = htim2.Instance->CNT;
    sprintf(uartData,"FCS = %x\n",crc);
    HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
    for(int j = 15; j >= 0; j--){
        sprintf(uartData,"%d",FCS[j]);
        HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
    }
    sprintf(uartData,"\ntime = %d\n",time);
    HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
    crc = 0;			//crc value after calculating data from PC
    end_flag = 0;				//flag to indicate the end of a KISS Packet
    bit_count = 0;

}

/*
 * AX.25.c
 *
 *  Created on: Sep 2, 2020
 *      Author: Kobe
 */
#include "AX.25.h"
#include "FreqIO.h"

//*************** variables for detecting and validating  AX.25  ******************************************************
bool AX25_flag = false; //indicates whether the TNC started reading for packets
int AX25_temp_buffer[max_packet_count]; //temperary stores bits received from radio, before formatting into AX.25 format
int packet_count = 0; //keeps count of the temp buffer index
//*********************************************************************************************************************

//*************** AX.25 Fields ******************************************************************************************
int start_flag[flag_len];
int address[address_len];
int control[control_len];
int PID[PID_len]; //only for information subfield
int Info[Info_len]; //only for information subfield
int FCS[FCS_len];
int end_flag[flag_len];
//***********************************************************************************************************************


//************* Handle bits received from Radio *************************************************************************
//while the AX25_flag is true, start storing bits in temp buffer
void store_radio_bits(){
	int bit7,bit6,bit5,bit4,bit3,bit2,bit1,bit0;

	//keep checking for ending flag
	while(AX25_flag && packet_count < max_packet_count && !changeMode){
		//Slide bits
		bit7 = bit6;
		bit6 = bit5;
		bit5 = bit4;
		bit4 = bit3;
		bit3 = bit2;
		bit2 = bit1;
		bit1 = bit0;
		bit0 = loadBit();
		AX25_temp_buffer[packet_count++] = bit0; //store bit into AX25 buffer

		//Detect 0x7
		if((bit7==0)&&(bit6==1)&&(bit5==1)&&(bit4==1)){
			//Detect 0x7E
			if((bit3==1)&&(bit2==1)&&(bit1==1)&&(bit0==0)){
				AX25_flag = !AX25_flag;
				packet_count = 0;
				sprintf(uartData, "AX.25 Flag Detected\r\n");
			}
		}
		else {
			sprintf(uartData, "Bits detected: %d %d %d %d %d %d %d %d\r\n",bit7,bit6,bit5,bit4,bit3,bit2,bit1,bit0);
		}

		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
}

//check if we keep the packet or remove it
bool Packet_Validate(){
	bool is_valid;
	if(packet_count < 120){ //invalid if packet is less than 136 bits - 2*8 bits (per flag)
		sprintf(uartData,"Trash Packet");
		is_valid = false;
	}
	else if((packet_count+1)%8 != 0){ //invalid if packet is not octect aligned (divisible by 8)
		sprintf(uartData,"Trash Packet");
		is_valid = false;
	}
	else{
		//this is assuming that the packet has all the subfields full
		sprintf(uartData,"Good Packet!");
		int *curr_mem = &AX25_temp_buffer; //keep track of what address to copy from
		memcpy(address,curr_mem,address_len*sizeof(int));
		curr_mem += address_len;
		memcpy(control,curr_mem,control_len*sizeof(int));
		curr_mem += control_len;
		if(packet_count > max_packet_count - PID_len){ //information type packet
			memcpy(PID,curr_mem,PID_len*sizeof(int));
			curr_mem += PID_len;
		}
		memcpy(Info,curr_mem,Info_len*sizeof(int));
		curr_mem += Info_len;
		memcpy(FCS,curr_mem,FCS_len*sizeof(int));
		is_valid = true;
	}

	//reset buffer and bit counter
	packet_count = 0;
	memset(AX25_temp_buffer,0,max_packet_count);
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	return is_valid;
=======

//*************** variables for detecting and validating  AX.25  ******************************************************
bool AX25_flag = false; 						//indicates whether the TNC started reading for packets
int rxBit_count = 0; 							//keeps count of the temp buffer index

bool AX25TBYTE[FLAG_SIZE] = { 0, 1, 1, 1, 1, 1, 1, 0 };

//NEED TO SET LOCAL_ADDRESS
bool local_address[address_len/2];
//*********************************************************************************************************************

//**************** KISS *************************************************************************************************************
bool KISS_FLAG[FLAG_SIZE] = { 1, 1, 0, 0, 0, 0, 0, 0 };

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

void transmitting_AX25(){
	HAL_GPIO_WritePin(PTT_GPIO_Port, PTT_Pin, GPIO_PIN_SET); //START PTT

	//Remember to format for KISS TO AX.25

	output_AX25();

	HAL_GPIO_WritePin(PTT_GPIO_Port, PTT_Pin, GPIO_PIN_RESET); //stop transmitting
}

void output_AX25(){
	struct PACKET_STRUCT* local_packet = &global_packet;

	bitToAudio(AX25TBYTE, FLAG_SIZE,1); //start flag

	bitToAudio(local_packet->address, address_len,0); //lsb first
	bitToAudio(local_packet->control,control_len,0);	//lsb first
	bitToAudio(local_packet->PID,PID_len,0);			//lsb first
	bitToAudio(local_packet->Info,local_packet->Info_Len + local_packet->stuffed_notFCS,0);		//lsb first
	bitToAudio(local_packet->FCS,FCS_len + local_packet->stuffed_FCS,1);			//msb first

	bitToAudio(AX25TBYTE, FLAG_SIZE,1);//stop flag
}

void transmitting_KISS(){
	//do stuff to send kiss... LOOKING AT YOU KALEB
}

//AX.25 to KISS data flow
//****************************************************************************************************************
void receiving_AX25(){
	struct PACKET_STRUCT* local_packet = &global_packet;

	int packet_status;
	packet_status = streamGet();
	if(packet_status == 1){

		//Remove the bit stuffed zeros from received packet and reset packet type
		remove_bit_stuffing();
		local_packet->i_frame_packet = false;

		//Validate packet
		bool AX25_IsValid = AX25_Packet_Validate();

		if(AX25_IsValid){

			//Put data into KISS format and buffer
			AX25_TO_KISS();

			//Clear AX.25 buffer
			memset(local_packet->AX25_PACKET,0,AX25_PACKET_MAX);
		}
		else{
			receiving_AX25();
		}
	}
	else{
		toggleMode();
	}
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
		curr = local_packet->AX25_PACKET[i]; //iterate through all data received before seperating into subfields
		if(curr){ //current bit is a 1
			one_count++;
		}
		else{
			if(one_count >= 5){
				slide_bits(&local_packet->AX25_PACKET[i],rxBit_count-i);
				shift_count++;
				rxBit_count--;
			}
			one_count = 0;
		}
	}
	//transmit kiss
}

bool AX25_Packet_Validate(){
	struct PACKET_STRUCT* local_packet = &global_packet;
	int fcs_val = 0;

	if(rxBit_count < 120){ //invalid if packet is less than 136 bits - 2*8 bits (per flag)
		sprintf(uartData,"Trash Packet");
		return false;
	}
	else if((rxBit_count+1)%8 != 0){ //invalid if packet is not octect aligned (divisible by 8)
		sprintf(uartData,"Trash Packet");
		return false;
	}

	//SHOULD BE VALID PACKET, JUST NEED TO C0MPARE CALCULATED CRC TO RECIEVED FCS
	else{
		//Set packet pointers for AX25 to KISS operation
		set_packet_pointer_AX25();
		return crc_check();

//		//FOR FCS ONLY, highest index == LSB
//		//FOR FCS ONLY, lowest index  == MSB
//		for(int i = FCS_len-1; i >= 0;i--){
//			//Convert FCS to decimal value (DOES NOT INCLUDE DECIMAL VALUES)
//			fcs_val += (local_packet->FCS[i]) ? pow(2,FCS_len-1-i) :0;
//		}
//
//		//generate crc
//		crc_generate();
//
//		//compare crc
//		if(local_packet->crc!=fcs_val){
//			return false;
//		}
	}

//	return true; //valid packet
}
void set_packet_pointer_AX25(){
	struct PACKET_STRUCT* local_packet = &global_packet;
	bool *curr_mem = &local_packet->AX25_PACKET; //keep track of what address to copy from
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

	if(local_packet->control[0] == 0){ // 0 == I frame, 01 == S frame, 11 == U Frame
		local_packet->i_frame_packet = true;//Signal flag is of type i-frame
		local_packet->PID = curr_mem;
		curr_mem += PID_len;
		not_info += PID_len;
	}


	local_packet->Info_Len = rxBit_count - not_info;
	local_packet->Info = curr_mem;
	curr_mem += local_packet->Info_Len;

	local_packet->FCS = curr_mem;
}

void AX25_TO_KISS(){
	struct PACKET_STRUCT* local_packet = &global_packet;

	bool *curr_mem = &local_packet->KISS_PACKET;

	/*
	 * 	NEED TO SET global_packet PACKET POINTERS IN HERE AS WELL
	 * 	DAVID WAS FEELING LAZY AND DID NOT DO IT
	 */

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

}

//****************************************************************************************************************
//END OF AX.25 to KISS data flow

//KISS to AX.25 data flow
//****************************************************************************************************************
void KISS_TO_AX25(){
	struct PACKET_STRUCT* local_packet = &global_packet;

	bool *curr_mem = &local_packet->KISS_PACKET; //keep track of what address to copy from
	//this is assuming that the packet has all the subfields full

	local_packet->Info_Len = rxBit_count - (address_len + control_len);

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
		local_packet->Info_Len -= PID_len;
	}

	local_packet->Info = curr_mem;
	curr_mem += local_packet->Info_Len;
	//find the length of info field for crc calculation


	//USE CRC HERE TO GENERATE FCS FIELD
	local_packet->FCS = curr_mem;
	local_packet->check_crc = false;
	crc_generate();

	//BIT STUFFING NEEDED
	bitstuffing(local_packet);

	return true; //valid packet
}

void set_packet_pointer_KISS(){

}

void bit_shift(bool* array,int bits_left){
	memcpy(array+2,array+1,bits_left*bool_size);
}

void bitstuffing(struct PACKET_STRUCT* packet){
	packet->stuffed_notFCS = 0;
	packet->stuffed_FCS = 0;

	int ones_count = 0;
	int bits_left = rxBit_count + FCS_len; //keeps track of how many bits have been iterated through in the AX.25 packet
	int bit_shifts = 0;			 //keeps track of how many bitstuffed zeros have been added

	//bit stuff fields for AX25 excluding FCS
	for(int i = 0; i < rxBit_count+bit_shifts;i++){
		bits_left--;
		if(packet->KISS_PACKET[i]){
			ones_count++;
			//add bitstuffed zeros after 5 contigious 1's
			if(ones_count == 5){
				bitshift(&(packet->KISS_PACKET[i]),bits_left);
				packet->KISS_PACKET[i+1] = false;
				bits_left++; //bitstuffed zero added
				bit_shifts++;
				ones_count = 0;
				packet->stuffed_notFCS++;
			}
		}
	}

	//bit stuff FCS field
	for(int i = 0; i < FCS_len;i++){
		bits_left--;
		if(packet->FCS[i]){
			ones_count++;
			//add bitstuffed zeros after 5 contigious 1's
			if(ones_count == 5){
				bitshift(&(packet->FCS[i]),bits_left);
				packet->FCS[i+1] = false;
				bits_left++; //bitstuffed zero added
				bit_shifts++;
				ones_count = 0;
				packet->stuffed_FCS++;
			}
		}
	}
}
//****************************************************************************************************************
//END OF KISS to AX.25 data flow

//---------------------- FCS Generation -----------------------------------------------------------------------------------------------

//store bits in FCS field
void hex_to_bin(){
	struct PACKET_STRUCT* local_packet = &global_packet;

    int temp;
    for(int i = 0; i < FCS_len; i++){ //stores in bits into fcs subfield
        temp = local_packet->crc >> i;
        local_packet->FCS[i] = temp%2;
    }

    //int time = htim2.Instance->CNT;

    //sprintf(uartData,"FCS = %x\n",local_packet->crc);
    //HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
    //sprintf(uartData,"\nExecution time = %d\n",time);
    //HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

    local_packet->crc = 0xFFFF; //reinitialize
}

//CRC Calculations
void crc_calc(int in_bit, int * crc_ptr_in, int * crc_count_ptr_in){
	struct PACKET_STRUCT* local_packet = &global_packet;
	int out_bit;
    int poly = 0x8408;             			//reverse order of 0x1021

    out_bit = in_bit ^ (*crc_ptr_in%2); 		//xor lsb of current crc with input bit
	*crc_ptr_in >>= 1;               	//right shift by 1
	poly = (out_bit == 1) ? 0x8408 : 0;
	*crc_ptr_in ^= poly;
	*crc_count_ptr_in+=1;//Increment count

    //End condition
	if(*crc_count_ptr_in >= rxBit_count){
    	*crc_ptr_in ^= 0xFFFF;//Complete CRC by XOR with all ones (one's complement)
    	if(local_packet->check_crc == false){
    		hex_to_bin();
    	}
    }
}

void crc_generate(){
	struct PACKET_STRUCT* local_packet = &global_packet;
	int * crc_ptr = &local_packet->crc;
	int * crc_count_ptr = &local_packet->crc_count;

	*crc_ptr = 0xFFFF;
	*crc_count_ptr = 0;

	//Generate CRC from packet pointers of current packet type

	//have to be inserted in reverse order
	//Calculate CRC for info
	for(int i = local_packet->Info_Len-1; i >= 0;i--){
		//Call crc_calc per bit
		crc_calc((int)local_packet->Info[i],crc_ptr,crc_count_ptr);
	}

	//Calculate CRC for PID (if packet is of type i-frame)
	if(local_packet->i_frame_packet){
		for(int i = PID_len-1; i >= 0; i--){
			//Call crc_calc per bit
			crc_calc((int)local_packet->PID[i],crc_ptr,crc_count_ptr);
		}
	}

	//Calculate CRC for control
	for(int i = control_len-1; i >= 0;i--){
		//Call crc_calc per bit
		crc_calc((int)local_packet->control[i],crc_ptr,crc_count_ptr);
	}

		//Calculate CRC for address
	for(int i = address_len-1; i >= 0;i--){
		//Call crc_calc per bit
		crc_calc((int)local_packet->address[i],crc_ptr,crc_count_ptr);
	}
}

bool crc_check(){
	struct PACKET_STRUCT* local_packet = &global_packet;
	local_packet->check_crc = true;
	int fcs_val = 0;
	bool valid_crc = false;

	//FOR FCS ONLY, highest index == LSB
	//FOR FCS ONLY, lowest index  == MSB
	for(int i = FCS_len-1; i >= 0;i--){
		//Convert FCS to decimal value (DOES NOT INCLUDE DECIMAL VALUES)
		fcs_val += (local_packet->FCS[i]) ? pow(2,FCS_len-1-i) :0;
	}

	//generate crc
	crc_generate();

	//compare crc
	valid_crc = (local_packet->crc==fcs_val) ? true : false;
	return valid_crc;
}

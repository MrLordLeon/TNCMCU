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

//Conversion functions
void conv_HEX_to_BIN(uint8_t hex_byte_in, bool *bin_byte_out){
    int temp;

    //sprintf(uartData, "\nByte value            = %d\nBinary value[LSB:MSB] =",hex_byte_in);
	//HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

    for(int i = 0; i < 8; i++){
        temp = hex_byte_in >> i;
        temp = temp%2;

        //sprintf(uartData, " %d ",temp);
		//HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

        *(bin_byte_out+i) = temp;
    }

    //sprintf(uartData, "\n");
	//HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
}
uint8_t conv_BIN_to_HEX(bool *bin_byte_in){
	uint8_t acc = 0;
	for(int i = 0; i < 8; i++){
		acc += ( *(bin_byte_in+i) )? pow(2,8-i-1) : 0;
	}
	return acc;
}

//General Program
//****************************************************************************************************************
void init_AX25(){
	HAL_UART_Receive_IT(&huart2, &(UART_packet.input), UART_RX_IT_CNT);
	UART_packet.flags = 0;
	UART_packet.got_packet = false;
	UART_packet.rx_cnt = 0;
	UART_packet.received_byte_cnt = 0;
}

void tx_rx() {
	if (changeMode) {
		changeMode = 0;
		toggleMode();
	}

	if (mode) {

		receiving_KISS();
		//bitToAudio(&bitStream[0], 10,1);
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
	// IT IS HERE

	struct UART_INPUT* local_UART_packet = &global_packet;

	HAL_UART_Transmit(&huart2, local_UART_packet->HEX_KISS_PACKET, KISS_SIZE, 10);
}

void print_KISS(){
	struct PACKET_STRUCT* local_packet = &global_packet;
	int bytecnt = local_packet->byte_cnt;
	bool *curr_mem;
	sprintf(uartData, "\nPrinting KISS_PACKET... All fields printed [MSB:LSB]\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	//Print Start Flag
	curr_mem = (local_packet->address-16);//Subtract 16 to start at the flag start
	sprintf(uartData, "Start flag      =");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	for(int i = 0;i<8;i++){
		sprintf(uartData, " %d ",*(curr_mem+8-i-1));
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	sprintf(uartData, "\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	//Print Address Field
	curr_mem = (local_packet->address);
	for(int i = 0;i<address_len/8;i++){
		sprintf(uartData, "Address Field %d =",i+1);
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		for(int j = 0;j<8;j++){
			sprintf(uartData, " %d ",*(curr_mem+8-j-1));
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
		curr_mem += 8;
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
	curr_mem = (local_packet->Info);
	for(int i = 0;i<(local_packet->Info_Len/8)-1;i++){
		sprintf(uartData, "Info Field %d    =",i+1)	;
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		for(int j = 0;j<8;j++){
			sprintf(uartData, " %d ",*(curr_mem+8-j-1));
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		}
		curr_mem += 8;
		sprintf(uartData, "\n");
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}

	//Print Stop Flag
	curr_mem = (local_packet->KISS_PACKET+(8*(local_packet->byte_cnt-1)));
	sprintf(uartData, "Stop flag       =");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

	for(int i = 0;i<8;i++){
		sprintf(uartData, " %d ",*(curr_mem+8-i-1));
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
	sprintf(uartData, "\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

}
//UART Handling data flow
//****************************************************************************************************************
void UART2_EXCEPTION_CALLBACK(){
	HAL_UART_Receive_IT(&huart2, &(UART_packet.input), UART_RX_IT_CNT);//Reset
	UART_packet.got_packet = false;

	  if(UART_packet.input==0xc0){
		  UART_packet.flags++;
	  }

	  *(UART_packet.HEX_KISS_PACKET+UART_packet.rx_cnt) = UART_packet.input;
	  UART_packet.rx_cnt++;

	  if(UART_packet.flags>=2){
		  UART_packet.flags = 0;
		  UART_packet.got_packet = true;
		  UART_packet.received_byte_cnt = UART_packet.rx_cnt;
		  UART_packet.rx_cnt=0;
	  }
}


//****************************************************************************************************************
//END OF UART Handling data flow

//AX.25 to KISS data flow
//****************************************************************************************************************
void receiving_AX25(){
	sprintf(uartData, "\nreceiving_AX25() start\n");
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	struct PACKET_STRUCT* local_packet = &global_packet;

	int packet_status;
	packet_status = streamGet();

	//Valid packet received
	if(packet_status == 1){
		sprintf(uartData, "Removing bit stuffed zeros\n");
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		//Remove the bit stuffed zeros from received packet and reset packet type
		remove_bit_stuffing();
		local_packet->i_frame_packet = false;

		//Validate packet
		bool AX25_IsValid = AX25_Packet_Validate();

		sprintf(uartData, "AX.25 frame valid check returned: %d\n",AX25_IsValid);
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		if(AX25_IsValid){
			//Put data into KISS format and buffer
			AX25_TO_KISS();
			sprintf(uartData, "AX.25 frame was converted to KISS frame\n");
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

			//Transmit KISS Packet that has been generated
			transmitting_KISS();
			sprintf(uartData, "KISS packeted sent via UART\n");
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

			//Clear AX.25 buffer
			memset(local_packet->AX25_PACKET,0,AX25_PACKET_MAX);

			//Loop back and begin receiving another message
			receiving_AX25();
		}
		else{
			sprintf(uartData, "Packet was not valid, restarting\n");
			HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
			receiving_AX25();
		}
	}
	//Return code for toggleMode
	else if(packet_status == -1){
		sprintf(uartData, "Need to change mode\n");
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		toggleMode();
	}
	//Weird case of unknown return code toggles mode
	else{
		sprintf(uartData, "Packet status was unknown, restarting\n");
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
		receiving_AX25();
	}
}

void slide_bits(bool* array,int bits_left){
	memcpy(array,array+1,bits_left*bool_size);
}

void remove_bit_stuffing(){
	struct PACKET_STRUCT* local_packet = &global_packet;

	int one_count = 0;
	bool curr;
	for(int i = 0;i < rxBit_count;i++){
		curr = local_packet->AX25_PACKET[i]; //iterate through all data received before seperating into subfields
		if(curr){ //current bit is a 1
			one_count++;
		}
		else{
			if(one_count >= 5){
				slide_bits(&local_packet->AX25_PACKET[i],rxBit_count-i);
				i--;
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
	else if((rxBit_count)%8 != 0){ //invalid if packet is not octect aligned (divisible by 8)
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

	KISS_TO_HEX();


}

//****************************************************************************************************************
//END OF AX.25 to KISS data flow

//KISS to AX.25 data flow
//****************************************************************************************************************
bool receiving_KISS(){
	struct UART_INPUT* local_UART_packet = &UART_packet;
	struct PACKET_STRUCT* local_packet = &global_packet;

	//Got a packet bounded by c0 over uart
	if(local_UART_packet->got_packet){
		sprintf(uartData, "\nGot a packet via UART of size %d, printing now...\n",local_UART_packet->received_byte_cnt);
		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

		for(int i = 0;i<local_UART_packet->received_byte_cnt;i++){
			//Hex value from UART
			uint8_t hex_byte_val=local_UART_packet->HEX_KISS_PACKET[i];

			//Bool pointer for KISS array
			bool *bin_byte_ptr = &local_packet->KISS_PACKET[i*8];

			//sprintf(uartData, "Byte[%d] = %d\n",i,hex_byte_val);
			//HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);

			conv_HEX_to_BIN(hex_byte_val, bin_byte_ptr);
		}
		local_packet->byte_cnt = local_UART_packet->received_byte_cnt;
		local_packet->Info_Len = (local_packet->byte_cnt-INFO_offset)*8;

		//Convert KISS packet to AX.25 packet
		KISS_TO_AX25();
		//Upon exit, have a perfectly good AX.25 packet

		//Output AFSK waveform for radio
		output_AX25();

		return true;
	}
	return false;
}

void set_packet_pointer_KISS(){
	struct PACKET_STRUCT* local_packet = &global_packet;

	bool *curr_mem = local_packet->KISS_PACKET+16;//+8 is to skip the flag since it does not have a pointer

	local_packet->address = curr_mem;
	curr_mem += address_len;

	local_packet->control = curr_mem;
	curr_mem += control_len;

	local_packet->PID = curr_mem;
	curr_mem += PID_len;

	local_packet->Info = curr_mem;
}

void KISS_TO_AX25(){
	struct PACKET_STRUCT* local_packet = &global_packet;

	//Update packet pointers to point to KISS members
	set_packet_pointer_KISS();

	//Print the kiss packet
	print_KISS();//Verify packet look correct

	if(!compare_address(local_packet->address)){
		return false; //discard
	}

	/*
	//SHOULD CONSIDER A VAR IN STRUCT TO INDICATE THAT A PID FIELD IS PRESENT OR THAT THIS IS AN I FRAME
	if(local_packet->control[0] == 0){ // 0 = I frame, 01 = S frame, 11 = U Frame
		local_packet->PID = curr_mem;
		curr_mem += PID_len;
		local_packet->Info_Len -= PID_len;
	}
	*/

	set_packet_pointer_AX25();

	//USE CRC HERE TO GENERATE FCS FIELD
	//local_packet->check_crc = false;
	crc_generate();

	//BIT STUFFING NEEDED
	bitstuffing(local_packet);

	return true; //valid packet
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
				bit_shift(&(packet->KISS_PACKET[i]),bits_left);
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
				bit_shift(&(packet->FCS[i]),bits_left);
				packet->FCS[i+1] = false;
				bits_left++; //bitstuffed zero added
				bit_shifts++;
				ones_count = 0;
				packet->stuffed_FCS++;
			}
		}
	}
}

void KISS_TO_HEX(){
	struct PACKET_STRUCT* local_packet = &global_packet;
	struct UART_INPUT* local_UART_packet = &UART_packet;

    for(int i = 0; i < KISS_SIZE; i+=8){
        bool *curr_mem = (local_packet->KISS_PACKET+i);
        local_UART_packet->HEX_KISS_PACKET[i] = conv_BIN_to_HEX(curr_mem);
    }
}
//****************************************************************************************************************
//END OF KISS to AX.25 data flow

//---------------------- FCS Generation -----------------------------------------------------------------------------------------------

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

    		//REMEBER TO CHECK THIS CRC conversion FOR ACCURACY LATER
    		conv_HEX_to_BIN(local_packet->crc,&(local_packet->FCS));
    		local_packet->crc = 0xFFFF;
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

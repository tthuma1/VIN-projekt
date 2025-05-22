/*
 * PN532.c
 *
 *  Created on: Apr 22, 2025
 *      Author: patriciobulic
 */


#include "PN532.h"

#include <stdint.h>
#include <string.h>



uint8_t packet_buffer[MAX_FRAME_LENGTH];      // packet buffer for data exchange

//uint8_t command;                 // variable to hold command sent
const uint8_t ACK_frame[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};



/****************************************************
 *                                                  *
 *                   SPI Functions                  *
 *                                                  *
 ****************************************************/



/*
 * This is just a wrapper around HAL_GPIO_WritePin used to set the pin for Slave Select
 */
static inline void SS_HIGH (void) {
	HAL_GPIO_WritePin(PN532_SS_GPIO_Port, PN532_SS_Pin, GPIO_PIN_SET);
}


/*
 * This is just a wrapper around HAL_GPIO_WritePin used to reset the pin for Slave Select
 */
static inline void SS_LOW (void) {
	HAL_GPIO_WritePin(PN532_SS_GPIO_Port, PN532_SS_Pin, GPIO_PIN_RESET);
}


/*
 * SPI_read : it receives one byte over SPI in blocking mode
 */
static uint8_t SPI_read(SPI_HandleTypeDef *hspi){
	uint8_t RxData[1];
	HAL_StatusTypeDef status;

	status = HAL_SPI_Receive(hspi, RxData, 1, HAL_MAX_DELAY);
	return RxData[0];
}

/*
 * SPI_write : it sends one byte over SPI in blocking mode
 */
static void SPI_write(SPI_HandleTypeDef *hspi, uint8_t data){
	uint8_t TxData[1];
	HAL_StatusTypeDef status;

	TxData[0] = data;
	status = HAL_SPI_Transmit(hspi, TxData, 1, HAL_MAX_DELAY);
}



 /****************************************************
  *                                                  *
  *       Internal Helper Functions                  *
  *                                                  *
  ****************************************************/

 /*
  * The SPI interface includes a specific register allowing the host
  * controller to know if the PN532 is ready to receive or to send data back.
  *
  * isReadyToSend: Sends a status read command and checks if the PN532 is ready to send data back.
  * It is a helper function used within waitToBeReady()
  *
  * @ return: STATUS_532_OK if the PN532 has a frame available to be transferred to the host controller.
  *
  */
 static StatusCode532_t isReadyToSend(void) {
     SS_LOW();

     SPI_write(&SPI_HANDLER, PN532_SPI_STATREAD);           	// write SPI STATUS READ command to PN532 module
     uint8_t status = SPI_read(&SPI_HANDLER);             		// and read response from PN532

     SS_HIGH();

     if (status == PN532_SPI_READY){ 	// check if PN532 is ready (LSB=1) and return the result
     	return STATUS_532_OK;
     }
     else return STATUS_532_ERROR;
 }

 /*
  * waitToBeReady:
  * Waits for the PN532 to become ready, up to a timeout in milliseconds.
  * It is a helper function used within sendCommand() and readResponse
  *
  * @return: STATUS_532_OK if ready, STATUS_532_ERROR if timeout
  */
 static StatusCode532_t waitToBeReady(uint16_t wait_time) {
     while (isReadyToSend() != STATUS_532_OK) {
         HAL_Delay(1);
         wait_time--;
         if (wait_time == 0) return STATUS_532_ERROR;
     }
     return STATUS_532_OK;
 }



 /*
  * readACK:
  * Reads the 6-byte ACK frame from the PN532 and checks if it matches the expected ACK values.
  * It is used within the sendCommand()
  * The expected ACK frame is 00 00 FF 00 FF 00
  *
  */
 static StatusCode532_t readACK(void) {
     uint8_t ACK_buffer[6];
     SS_LOW();

     SPI_write(&SPI_HANDLER, PN532_SPI_DATAREAD);  			// send PN532 the DATA READ SPI byte indicating the SPI READ operation
     for (int i = 0; i < 6; i++)
         ACK_buffer[i] = SPI_read(&SPI_HANDLER); 			// read 6-byte ACK frame

     SS_HIGH();

     // compare the received response to ACK frame
     for (int i=0; i<6; i++) {
    	 if (ACK_buffer[i] != ACK_frame[i]) return STATUS_532_INVALID_ACK;
     }
     return STATUS_532_OK;
 }



 /*
  * sendFrame:
  * Constructs and sends a frame over SPI, including SPI operation, preamble, start codes, length, checksums, TFI, and postamble.
  * It is a helper function udsed within sendCommand()
  *
  * Normal frame:
  * 00 00 FF LEN LCS TFI PD0 ... PDn DCS 00
  */
 static void sendFrame(uint8_t *cmd, uint8_t cmd_length) {
     SS_LOW();

     SPI_write(&SPI_HANDLER, PN532_SPI_DATAWRITE);				// send PN532 the DATAWRITE SPI operation
     SPI_write(&SPI_HANDLER, PN532_PREAMBLE);					// write PREAMBLE
     SPI_write(&SPI_HANDLER, PN532_STARTCODE1);					// write the first byte of START CODE
     SPI_write(&SPI_HANDLER, PN532_STARTCODE2);					// write the second byte of START CODE

     cmd_length++;												// length of data field: TFI + DATA
     SPI_write(&SPI_HANDLER, cmd_length);						// write command length to LEN
     SPI_write(&SPI_HANDLER, (~cmd_length + 1));				// write the 2's complement of command length to LCS
     SPI_write(&SPI_HANDLER, PN532_HOSTTOPN532);				// TFI: a frame from the host controller to the PN532

     uint8_t DCS = PN532_HOSTTOPN532;							// data checksum, see the datasheet (TFI + PD0 + .. +PDn + DCS) = 0x00

     for (uint8_t i = 0; i < cmd_length - 1; i++) {
         SPI_write(&SPI_HANDLER, cmd[i]);						// write data bytes
         DCS += cmd[i];											// accumulate data checksum
     }

     SPI_write(&SPI_HANDLER, (~DCS + 1));						// write 2's complement of DCS
     SPI_write(&SPI_HANDLER, PN532_POSTAMBLE);					// write POSTAMBLE

     HAL_Delay(1);
     SS_HIGH();
 }


 /*
  * sendCommand:
  * Sends a command to the PN532, waits for PN532 to be ready and checks for a proper ACK response.
  *
  * @return: STATUS_532_OK if ready and ACK received
  */
 static StatusCode532_t sendCommand(uint8_t *cmd, uint8_t cmd_length) {
	 int status;

     //command = cmd[0];                                  // record command for response verification
     sendFrame(cmd, cmd_length);                        // write command
     status = waitToBeReady(15);
     if (status != STATUS_532_OK) return STATUS_532_NOTREADY;
     StatusCode532_t ack = readACK(); // read ACK, returns 0 if OK
     if (ack != STATUS_532_OK) return STATUS_532_INVALID_ACK;
     return STATUS_532_OK;
 }

 /**
  * readResponseToCommand:
  * Reads a full response frame from the PN532 to a command, checks headers, length, command ID, and validates checksum.
  *
  * Return: number of bytes (>0) received if success, error code otherwise
  */
 static int16_t readResponseToCommand(
		 	 	 	 uint8_t command,
		 	 	 	 uint8_t *data_buffer,
					 uint8_t data_length, 						// length of a data buffer used to receive the response
					 uint16_t wait_time
 					) {


     if (waitToBeReady(wait_time) != STATUS_532_OK)
         return STATUS_532_TIMEOUT;        						// return time out error if not ready

     SS_LOW();

     SPI_write(&SPI_HANDLER, PN532_SPI_DATAREAD);         		// send PN532 DATAREAD SPI operation

     // read 1st to 3rd bytes and check if this is a valid frame (preamble + start codes)
     if (SPI_read(&SPI_HANDLER) != PN532_PREAMBLE   ||  		// first byte should be PREAMBLE
         SPI_read(&SPI_HANDLER) != PN532_STARTCODE1 ||  		// second byte should be STARTCODE1
         SPI_read(&SPI_HANDLER) != PN532_STARTCODE2     		// third byte should be STARTCODE2
         ) {
         SS_HIGH();                                          	// pull SS high since we are exiting
         return STATUS_532_INVALID_FRAME;   					// return invalid frame code as a result
     }

     /* read 4th and 5th bytes */
     uint8_t LEN = SPI_read(&SPI_HANDLER);              		// LEN: number of bytes in the data field
     if (LEN == 0) {
    	 SS_HIGH();                                          	// pull SS high since we are exiting
    	 return STATUS_532_NODATA;  							// return STATUS_532_NODATA
     }
     uint8_t LCS = SPI_read(&SPI_HANDLER);              		// LCS: Packet Length Checksum
     if ((uint8_t)(LEN + LCS) != 0x00 ) {						// LEN + LCS should be 0x00
         SS_HIGH();                                          	// pull SS high since we are exiting
         return STATUS_532_INVALID_LCS;  						// return invalid LCS error as a result
     }

     /* read 6th and 7th bytes */
     uint8_t PD0 = command + 1;                 				// PD0 is command code
     if (PN532_PN532TOHOST != SPI_read(&SPI_HANDLER) || PD0 != SPI_read(&SPI_HANDLER)) {
         SS_HIGH();                           					// pull SS high since we are exiting
         return STATUS_532_INVALID_FRAME;      					// return invalid frame code as result
     }

     /* check data buffer size before read actual data */
     LEN -= 2;                         							// subtract TFI and PD0(command) from DATA length
     if (LEN > data_length) {          							// if no enough space, just dummy read bytes for synchronization
         for (uint8_t i = 0; i < LEN; i++) SPI_read(&SPI_HANDLER);      	// dump data
         SPI_read(&SPI_HANDLER);                                        	// dump DCS
         SPI_read(&SPI_HANDLER);                                        	// dump POSTAMBLE
         SS_HIGH();                                          	// pull SS high since we are exiting
         return STATUS_532_NO_SPACE;                             // return (buffer) no space error code as result
     }

     /* read actual data */
     uint8_t SUM = PN532_PN532TOHOST + PD0;                  // SUM: TFI + DATA, DATA = PD0 + PD1 + ... + PDn
     for (uint8_t i = 0; i < LEN; i++) {
         data_buffer[i] = SPI_read(&SPI_HANDLER);       	 // get data
         SUM += data_buffer[i];                              // accumulate SUM
     }

     /* read data checksum byte */
     uint8_t DCS = SPI_read(&SPI_HANDLER);
     if ((uint8_t)(SUM + DCS) != 0) {
         SS_HIGH();                           				// pull SS high since we are exiting
         return STATUS_532_INVALID_DCS;      				// proper frame should result in SUM + DCS = 0
     }

     /* read POSTAMBLE */
     SPI_read(&SPI_HANDLER);                 				// dump postamble

     SS_HIGH();
     return (int16_t)LEN;									// number of bytes (LEN>0) received
 }





 /*****************************************************
  *                                                   *
  *   E X P O R T E D  P N 532 F U N C T I O N S      *
  *                                           		  *
  *                                                   *
  *****************************************************/


 /*
  * Wake-up PN532 module
  * The Adafruit PN532 module uses power-saving modes
  *    and the chip starts in Low Power or HSU (High-Speed UART) mode
  * We're not getting any response until wake-up
  * A common trick is to reset SS for a few ms (and maybe also RST pin)
  */
  void PN532_SPI_Init(void) {

      /*-- Wake Up PN532 --*/
      SS_LOW();
      HAL_Delay(50);
      SS_HIGH();
  }

 /*
  * PN532_getFirmwareVersion
  *  Retrieves the 4-byte firmware version from the PN532.
  *
  * @return: 32-bit firmware version number for PN532 or STATUS_532_ERROR for an error.
  *
  * Version:
  * 1st byte: Version of the IC. For PN532, the contain of this byte is 0x32
  * 2nd byte: Version of the firmware.
  * 3rd byte: Revision of the firmware.
  * 4th byte: 3 LS bits indicate which are the functionalities supported (ISO18092,ISO14443-B,ISO14443-A).
  *
  * In the case of the PN532: 0x32010607
  *
  */
 uint32_t PN532_getFirmwareVersion(void) {
     packet_buffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;
     StatusCode532_t status;


     status = sendCommand(packet_buffer, 1);
     if (status != STATUS_532_OK) return STATUS_532_ERROR;     // write command to PN532, return 0 means write fail
     status = readResponseToCommand(PN532_COMMAND_GETFIRMWAREVERSION,packet_buffer, 12, PN532_WAITTIME);
     if (status < 0) return STATUS_532_ERROR; // read response from PN532, return 0 for error

     // store the result into an unsigned 32 bit integer
     uint32_t response;
     response = packet_buffer[0];
     response <<= 8;
     response |= packet_buffer[1];
     response <<= 8;
     response |= packet_buffer[2];
     response <<= 8;
     response |= packet_buffer[3];

     return response;
 }


 /*
  * PN532_SAMConfiguration
  *
  * The Security Access Module (SAM) Configuration command configures how the PN532 operates.
  * It sets the PN532 to normal operation mode and initializes the RF interface.
  * Without it, the chip may not behave as expected — it may not respond to InListPassiveTarget properly,
  * or you might not receive tag UIDs.
  *
  * If SAMConfiguration succeed, PN532 returns respond to a command without any data
  *
  *
  * @return STATUS_532_OK if success, STATUS_532_ERROR if error.
  *
  *
  */
 StatusCode532_t PN532_SAMConfiguration(void) {
     /*-- prepare command --*/
     packet_buffer[0] = PN532_COMMAND_SAMCONFIGURATION;
     packet_buffer[1] = 0x01;                           	// normal operation mode, the SAM is not used; this is the default mode;
     packet_buffer[2] = 0x14;                           	// The timeout field in the SAMConfiguration command in Normal Mode determines how long the PN532
															// will wait for a passive card (e.g., MIFARE tag) during operations like
															// InListPassiveTarget.
															// When PN532 is in Normal Mode, this timeout is used for:
															//  1. How long the chip waits when you perform commands like InListPassiveTarget
															// 		and no card is present.
															//  2. After this time, the command will timeout and return a response
															//      indicating no card was found.
															// This parameter indicates the timeout value with a LSB of 50ms.
															// There is no timeout control if the value is null (Timeout = 0).
															// The maximum value for the timeout is 12.75 sec (Timeout = 0xFF).
															// timeout 50ms * 20 = 1 second
     packet_buffer[3] = 0x01;                           	// use IRQ pin!

     /*-- write command and read response --*/
     if (sendCommand(packet_buffer, 4) != STATUS_532_OK) return STATUS_532_ERROR;
     if (readResponseToCommand(PN532_COMMAND_SAMCONFIGURATION, packet_buffer, sizeof(packet_buffer), PN532_WAITTIME) < 0) return STATUS_532_ERROR;
     return  STATUS_532_OK;  // return response status code
 }




 /*
  * InListPassiveTarget
  * Sends a command to detect ISO14443A targets. If found, returns the card's UID and its length.
  *
  * @args:
  *    uid           Pointer to the array that will be populated with the card's UID (up to 7 bytes).
  *    uidLength     Pointer to the variable that will hold the length of the card's UID.
  *
  * @return STATUS_532_OK if success, STATUS_532_ERROR for an error.
  *
  */
 StatusCode532_t InListPassiveTarget (uint8_t *uid, uint8_t *uid_length) {
     /*-- prepare command --*/
     packet_buffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
     packet_buffer[1] = 0x01;                  // (MaxTg) - max 1 card
     packet_buffer[2] = 0x00;                  // (BrTy) - read datasheet to see card baudrate setting - 0x00: 106 kbps type A (ISO/IEC14443 Type A),

     /*-- write command and read response --*/
     if (sendCommand(packet_buffer, 3) != STATUS_532_OK) return STATUS_532_ERROR;     // return  error
     if (readResponseToCommand(PN532_COMMAND_INLISTPASSIVETARGET,packet_buffer, sizeof(packet_buffer), 15) < 0) return STATUS_532_ERROR;

     /*
      *         ISO14443A Card Response Format
      *  -------------------------------------
      *   byte          |   Description
      *  -------------  |  -------------------
      *   b0            |   NbTg: Number of targets found (usually 1)
      *   b1            |   Target number (used in further commands like InDataExchange)
      *   b2..3         |   SENS_RES: ATQA (Answer to Request A): card type information.
      *   b4            |   SEL_RES: SAK (Select Acknowledge): indicates card capabilities.
      *   b5            |   NFCID Length: Length of the UID (typically 4, 7, or 10 bytes).
      *   b6..NFCIDLen  |   NFCID: The unique identifier (UID) of the card/tag.
      */

     /*-- authenticate and save data --*/
     /* byte 0 */
     if (packet_buffer[0] != 1)
    	 return STATUS_532_NOTAG;  						// return STATUS_532_NOTAG, if no tags found

     /* byte 5 */
     *uid_length = packet_buffer[5];                    // save uid length

     /* UID */
     for (uint8_t i = 0; i < packet_buffer[5]; i++)
         uid[i] = packet_buffer[6 + i];                 // save uid bytes

     return STATUS_532_OK;
 }




 /*
  * oscilloscopeLoop
  * host endlessly asks the PN532 if it is ready to send.
  * The function is used to view SPI communication on the oscilloscope
  */

 void oscilloscopeLoop(void) {
	 uint8_t cmd[1] = {PN532_COMMAND_GETFIRMWAREVERSION};

	 sendFrame(cmd, 1);
	 //HAL_Delay(500);

	 while(1) {
		 isReadyToSend();
		 HAL_Delay(1);
	 }
 }



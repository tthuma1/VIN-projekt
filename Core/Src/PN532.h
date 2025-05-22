/*
 * PN532.h
 *
 *  Created on: Apr 22, 2025
 *      Author: patriciobulic
 */

#ifndef SRC_PN532_H_
#define SRC_PN532_H_


#include "stm32h7xx_hal.h"

/*********************************************************
 * Define names for the used SPI handler and GPIO ports
 * Pa3cio, UL FRI, 2025
 *********************************************************/

extern SPI_HandleTypeDef hspi2;
#define SPI_HANDLER hspi2

#define PN532_SS_GPIO_Port	GPIOB
#define PN532_SS_Pin		GPIO_PIN_4


/*********************************************************
 * Define names for some useful constants
 * Pa3cio, UL FRI, 2025
 *********************************************************/

#define MAX_FRAME_LENGTH					(255) // as the LEN field in the frame is 8-bit, it cannot be larger than 255

/* Wait Time */
#define PN532_WAITTIME                      (1000)
#define PN532_ACK_WAIT_TIME                 (200)


/*********************************************************
 * Define names for constant frame bytes (e.g. preamble etc.)
 * Pa3cio, UL FRI, 2025
 *********************************************************/
#define PN532_PREAMBLE                      (0x00)
#define PN532_STARTCODE1                    (0x00)
#define PN532_STARTCODE2                    (0xFF)
#define PN532_POSTAMBLE                     (0x00)
#define PN532_HOSTTOPN532                   (0xD4)
#define PN532_PN532TOHOST                   (0xD5)


/*********************************************************
 * Define SPI operations used in PN532
 * Pa3cio, UL FRI, 2025
 *********************************************************/
#define PN532_SPI_STATREAD                  (0x02)
#define PN532_SPI_DATAWRITE                 (0x01)
#define PN532_SPI_DATAREAD                  (0x03)





/*********************************************************
 * Define PN532 commands
 * Pa3cio, UL FRI, 2025
 *********************************************************/
#define PN532_COMMAND_DIAGNOSE              (0x00)
#define PN532_COMMAND_GETFIRMWAREVERSION    (0x02)
#define PN532_COMMAND_GETGENERALSTATUS      (0x04)
#define PN532_COMMAND_READREGISTER          (0x06)
#define PN532_COMMAND_WRITEREGISTER         (0x08)
#define PN532_COMMAND_READGPIO              (0x0C)
#define PN532_COMMAND_WRITEGPIO             (0x0E)
#define PN532_COMMAND_SETSERIALBAUDRATE     (0x10)
#define PN532_COMMAND_SETPARAMETERS         (0x12)
#define PN532_COMMAND_SAMCONFIGURATION      (0x14)
#define PN532_COMMAND_POWERDOWN             (0x16)
#define PN532_COMMAND_RFCONFIGURATION       (0x32)
#define PN532_COMMAND_RFREGULATIONTEST      (0x58)
#define PN532_COMMAND_INJUMPFORDEP          (0x56)
#define PN532_COMMAND_INJUMPFORPSL          (0x46)
#define PN532_COMMAND_INLISTPASSIVETARGET   (0x4A)
#define PN532_COMMAND_INATR                 (0x50)
#define PN532_COMMAND_INPSL                 (0x4E)
#define PN532_COMMAND_INDATAEXCHANGE        (0x40)
#define PN532_COMMAND_INCOMMUNICATETHRU     (0x42)
#define PN532_COMMAND_INDESELECT            (0x44)
#define PN532_COMMAND_INRELEASE             (0x52)
#define PN532_COMMAND_INSELECT              (0x54)
#define PN532_COMMAND_INAUTOPOLL            (0x60)
#define PN532_COMMAND_TGINITASTARGET        (0x8C)
#define PN532_COMMAND_TGSETGENERALBYTES     (0x92)
#define PN532_COMMAND_TGGETDATA             (0x86)
#define PN532_COMMAND_TGSETDATA             (0x8E)
#define PN532_COMMAND_TGSETMETADATA         (0x94)
#define PN532_COMMAND_TGGETINITIATORCOMMAND (0x88)
#define PN532_COMMAND_TGRESPONSETOINITIATOR (0x90)
#define PN532_COMMAND_TGGETTARGETSTATUS     (0x8A)





/* Status ready: */
#define PN532_SPI_READY                     (0x01)

/*********************************************************
 * Define a type for status returned from PN532 functions
 * Pa3cio, UL FRI, 2025
 *********************************************************/
typedef enum StatusCode532 {
	STATUS_532_OK				=  0,	// Success
	STATUS_532_ERROR			= -1,	// Error in communication
	STATUS_532_INVALID_ACK		= -2,
	STATUS_532_TIMEOUT			= -3,	// Timeout in communication.
	STATUS_532_INVALID_FRAME 	= -4,
	STATUS_532_INVALID_LCS		= -5,
	STATUS_532_NO_SPACE			= -6,
	STATUS_532_INVALID_DCS	    = -7,
	STATUS_532_NOTREADY			= -8,
	STATUS_532_NODATA			= -9,	// if length of data in read operation equals to 0
	STATUS_532_NOTAG			= -10
} StatusCode532_t;





/*********************************************************
 * Exported PN532 functions
 * Pa3cio, UL FRI, 2025
 *********************************************************/
void PN532_SPI_Init(void);
uint32_t PN532_getFirmwareVersion(void);
StatusCode532_t InListPassiveTarget (uint8_t *uid, uint8_t *uid_length);
StatusCode532_t PN532_SAMConfiguration(void);

void oscilloscopeLoop(void);



#endif /* SRC_PN532_H_ */

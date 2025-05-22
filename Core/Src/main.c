/**
  ******************************************************************************
  * @file    BSP/Src/main.c
  * @author  MCD Application Team
  * @brief   This example code shows how to use the STM32H750B_DISCOVERY BSP Drivers
  *          This is the main program.   
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdbool.h>
#include <math.h>

#include "PN532.h"
#include "spi_pattern.h"

/** @addtogroup STM32H7xx_HAL_Examples
  * @{
  */

/** @addtogroup BSP
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define MAX_COMMAND_LEN 100
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c4;
UART_HandleTypeDef huart3;
SPI_HandleTypeDef hspi2;
RTC_HandleTypeDef hrtc;

uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
uint8_t tagUid[] = { 0x93, 0x4D, 0x8, 0x14 };

uint32_t firmwareVersion = 0;

StatusCode532_t statusCode;

uint8_t txFWversion[128];
uint8_t txUIDLength[128];
uint8_t txUID[32];
uint8_t txData[256];

unsigned long textColor = UTIL_LCD_COLOR_BLACK;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);
//static void MPU_Config(void);
static void MX_USART3_UART_Init(void);
static void MX_GPIO_Init(void);
static void MX_I2C4_Init(void);
static void MX_SPI2_Init(void);
static void MX_RTC_Init(void);

float Read_HTU21D_Temperature(void);
float calculate_power(float, float);
void Start_UART_Receive_IT();
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);



/* Private functions ---------------------------------------------------------*/
float Read_HTU21D_Temperature(void) {
  uint8_t write_address = 0x80;
  uint8_t command[] = {0xF3};

  // trigger temperature measurement
  HAL_I2C_Master_Transmit(&hi2c4, write_address, command, sizeof(command), HAL_MAX_DELAY);
  HAL_Delay(50);

  uint8_t read_address = 0x81;
  uint8_t sensor_data[3];
  HAL_I2C_Master_Receive(&hi2c4, read_address, sensor_data, sizeof(sensor_data), HAL_MAX_DELAY);

  uint16_t meritev =  (sensor_data[0] << 8) | (sensor_data[1] & 0xFC);
  float temp = -46.85 + 175.72 * ((float) meritev / (1 << 16));

  return temp;
}

float calculate_power(float temperature, float prag) {
    float alpha = 1.0f; // faktor ostrine S-krivulje
    float x = temperature - prag;
    return -3.0f * tanhf(alpha * x); // tanhf je float verzija tanh()
}

float prag = 22.0f; // Set temp
float alpha = 1.0f; // faktor ostrine S-krivulje
uint8_t loggedIn = 0;
char commandBuffer[MAX_COMMAND_LEN];
int commandIndex = 0;
uint8_t receivedChar;
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* System Init, System clock, voltage scaling and L1-Cache configuration are done by CPU1 (Cortex-M7)
     in the meantime Domain D2 is put in STOP mode(Cortex-M4 in deep-sleep)
  */

  //MPU_Config();

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* STM32H7xx HAL library initialization:
       - Systick timer is configured by default as source of time base, but user
         can eventually implement his proper time base source (a general purpose
         timer for example or other time source), keeping in mind that Time base
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 400 MHz */
  SystemClock_Config();

  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USART3_UART_Init();
  MX_I2C4_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_2, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOI, GPIO_PIN_13, GPIO_PIN_SET);
  HAL_Delay(20);

  BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE);
  UTIL_LCD_SetFuncDriver(&LCD_Driver);

  Touchscreen_demo();

  Start_UART_Receive_IT();

    /*****************************************************************************************************
   * 1. Wake-up PN532 and check if it is available by getting its firmware version
   *****************************************************************************************************/
  HAL_UART_Transmit(&huart3, "\r\n--------------REBOOT--------------\r\nLooking for PN532... \r\n", sizeof("\r\n--------------REBOOT--------------\r\nLooking for PN532... \r\n"), HAL_MAX_DELAY);
  PN532_SPI_Init();
  while(1) {
	  firmwareVersion = PN532_getFirmwareVersion();
	  if (firmwareVersion != STATUS_532_ERROR) {               // if not able to read version number, quit
		  break;
	  }
	  HAL_Delay(250);
  }

  sprintf(txFWversion, "PN532 found. Firmware version: 0x%08X \r\n", firmwareVersion);
  HAL_UART_Transmit(&huart3, txFWversion, sizeof(txFWversion), HAL_MAX_DELAY);

  /*****************************************************************************************************
   * 2. Configure SAM: set normal operation mode and initialize the RF interface
   *****************************************************************************************************/

  HAL_UART_Transmit(&huart3, "Configuring SAM.... \r\n", sizeof("Configuring SAM.... \r\n"), HAL_MAX_DELAY);
  while (PN532_SAMConfiguration() != STATUS_532_OK){
	  HAL_Delay(100);
  }
  HAL_UART_Transmit(&huart3, "SAM configured.\r\n\r\n", sizeof("SAM configured.\r\n\r\n"), HAL_MAX_DELAY);
  HAL_UART_Transmit(&huart3, "> ", sizeof("> "), HAL_MAX_DELAY);

  /* Wait For User inputs */
  while (1)
  {

	statusCode = InListPassiveTarget(uid, &uidLength);
	if (statusCode == STATUS_532_OK) {
		sprintf(txUIDLength, "\r\nUID Length: %d \r\n\0", uidLength);
		HAL_UART_Transmit(&huart3, txUIDLength, sizeof(txUIDLength), HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart3, "UID: ", sizeof("UID: "), HAL_MAX_DELAY);
		bool jeTag = true;
		for (int i = 0; i < uidLength; i++) {
			if (uid[i] != tagUid[i]) jeTag = false;
			sprintf(txUID, "0x%X ", uid[i]);
			HAL_UART_Transmit(&huart3, txUID, sizeof(txUID), HAL_MAX_DELAY);
		}
		HAL_UART_Transmit(&huart3, "\r\n", sizeof("\r\n"), HAL_MAX_DELAY);

		if (jeTag) {
			loggedIn = true;
      setLastActivityTime();
		}
		HAL_UART_Transmit(&huart3, "> ", sizeof("> "), HAL_MAX_DELAY);
	}

    if (loggedIn) {
      float temp = Read_HTU21D_Temperature();
      // //float temp = 22.0f;
      // float diff = temp - prag;

      // // PD3 = ogrevanje ali hlajenje (leva lucka)
      // // PJ2 = sredinska lucka
      // // PI13 = desna lucka

      // // leva lucka (ogravanje ali hlajenje)
      //   if (temp < prag - 0.5) {
      //       HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);
      //   } else {
      //       HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);
      //   }

      //   if ((-1.5 <= diff && diff < -0.5) || (0.5 < diff && diff <= 1.5)) {
      //     // nivo 1
      //       HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_2, GPIO_PIN_SET);
      //       HAL_GPIO_WritePin(GPIOI, GPIO_PIN_13, GPIO_PIN_RESET);
      //   } else if ((-2.5 <= diff && diff < -1.5) || (1.5 < diff && diff <= 2.5)) {
      //     // nivo 2
      //       HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_2, GPIO_PIN_RESET);
      //       HAL_GPIO_WritePin(GPIOI, GPIO_PIN_13, GPIO_PIN_SET);
      //   } else if (diff < -2.5 || diff > 2.5) {
      //     // nivo 3
      //       HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_2, GPIO_PIN_RESET);
      //       HAL_GPIO_WritePin(GPIOI, GPIO_PIN_13, GPIO_PIN_RESET);
      //   } else {
      //     // ugasnjeno
      //       HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_2, GPIO_PIN_SET);
      //       HAL_GPIO_WritePin(GPIOI, GPIO_PIN_13, GPIO_PIN_SET);
      //   }

        float power = calculate_power(temp, prag);

      //   update_display(prag, temp, power); // tvoja funkcija za izris
		  update_display(prag, temp, power);
	  } else {
		  drawLoginScreen();
	  }
	  HAL_Delay(20);
    checkActivity();
  }
}

void Start_UART_Receive_IT()
{
    HAL_UART_Receive_IT(&huart3, &receivedChar, 1);  // Start reception
}

// Callback gets called when a character is received
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        // Echo back the received character
        HAL_UART_Transmit(&huart3, &receivedChar, 1, HAL_MAX_DELAY);

        if (receivedChar == '\r' || receivedChar == '\n')
        {
            commandBuffer[commandIndex] = '\0';  // Terminate the string

            // Process the command
            if (strcmp(commandBuffer, "red") == 0)
            {
            	  textColor = UTIL_LCD_COLOR_RED;
                char msg[] = "\r\nYou entered RED!\r\n";
                HAL_UART_Transmit(&huart3, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
            } else if (strcmp(commandBuffer, "black") == 0) {
            	  textColor = UTIL_LCD_COLOR_BLACK;
                char msg[] = "\r\nYou entered BLACK!\r\n";
                HAL_UART_Transmit(&huart3, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
            } else if (strncmp(commandBuffer, "time", 4) == 0) {
                const char *dateTimeStr = commandBuffer + 5;  // Skip "time "
                setDateTimeFromString(dateTimeStr);
                char msg[] = "\r\nSet datetime.\r\n";
                HAL_UART_Transmit(&huart3, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
            }
            else
            {
                char msg[] = "\r\nUnknown command\r\n";
                HAL_UART_Transmit(&huart3, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
            }

            // Reset the buffer
            commandIndex = 0;
            memset(commandBuffer, 0, MAX_COMMAND_LEN);

            HAL_UART_Transmit(&huart3, "> ", sizeof("> "), HAL_MAX_DELAY);
        }
        else
        {
            if (commandIndex < MAX_COMMAND_LEN - 1)
            {
                commandBuffer[commandIndex++] = receivedChar;
            }
        }

        // Continue receiving next character
        HAL_UART_Receive_IT(&huart3, &receivedChar, 1);
    }
}

void setDateTimeFromString(const char *dateTimeStr) {
  // time 2025-05-18 14:30:00
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    int year, month, day, hour, minute, second;

    // Parse the string
    if (sscanf(dateTimeStr, "%d-%d-%d %d:%d:%d",
               &year, &month, &day, &hour, &minute, &second) != 6) {
        // Invalid format
        //Error_Handler();  // or handle gracefully
        return;
    }

    // Convert to 2-digit year for RTC (valid range: 00-99 = 2000-2099)
    if (year < 2000 || year > 2099) {
        //Error_Handler();
        return;
    }

    year -= 2000;

    // Fill RTC time structure
    sTime.Hours   = hour;
    sTime.Minutes = minute;
    sTime.Seconds = second;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    // Set time
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
        Error_Handler();
        return;
    }

    // Fill RTC date structure
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;  // Optional — RTC may ignore it
    sDate.Month   = month;
    sDate.Date    = day;
    sDate.Year    = year;

    // Set date
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
        Error_Handler();
        return;
    }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 400000000 (Cortex-M7 CPU Clock)
  *            HCLK(Hz)                       = 200000000 (Cortex-M4 CPU, Bus matrix Clocks)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 5
  *            PLL_N                          = 160
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /*!< Supply configuration update enable */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }

/* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }

 /*
  Note : The activation of the I/O Compensation Cell is recommended with communication  interfaces
          (GPIO, SPI, FMC, QSPI ...)  when  operating at  high frequencies(please refer to product datasheet)
          The I/O Compensation Cell activation  procedure requires :
        - The activation of the CSI clock
        - The activation of the SYSCFG clock
        - Enabling the I/O Compensation Cell : setting bit[0] of register SYSCFG_CCCSR
 */

  /*activate CSI clock mondatory for I/O Compensation Cell*/
  __HAL_RCC_CSI_ENABLE() ;

  /* Enable SYSCFG clock mondatory for I/O Compensation Cell */
  __HAL_RCC_SYSCFG_CLK_ENABLE() ;

  /* Enables the I/O Compensation Cell */
  HAL_EnableCompensationCell();
}


/**
  * @brief I2C4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C4_Init(void)
{

  /* USER CODE BEGIN I2C4_Init 0 */

  /* USER CODE END I2C4_Init 0 */

  /* USER CODE BEGIN I2C4_Init 1 */

  /* USER CODE END I2C4_Init 1 */
  hi2c4.Instance = I2C4;
  hi2c4.Init.Timing = 0x10707DBC;
  hi2c4.Init.OwnAddress1 = 0;
  hi2c4.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c4.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c4.Init.OwnAddress2 = 0;
  hi2c4.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c4.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c4.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C4_Init 2 */

  /* USER CODE END I2C4_Init 2 */

}


/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_LSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 0x0;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);

  /*Configure GPIO pin : PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOI, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pin : PD3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PI13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pin : PJ2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

//void MPU_Config(void)
//{
//  MPU_Region_InitTypeDef MPU_InitStruct = {0};
//
//  /* Disables the MPU */
//  HAL_MPU_Disable();
//
//  /** Initializes and configures the Region and the memory to be protected
//  */
//  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
//  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
//  MPU_InitStruct.BaseAddress = 0x0;
//  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
//  MPU_InitStruct.SubRegionDisable = 0x87;
//  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
//  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
//  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
//  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
//  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
//  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
//
//  HAL_MPU_ConfigRegion(&MPU_InitStruct);
//  /* Enables the MPU */
//  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
//
//}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* Turn LED REDon */
  BSP_LED_On(LED_RED);
  while(1)
  {
  }
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif /* USE_FULL_ASSERT */

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/**
  * @}
  */

/**
  * @}
  */


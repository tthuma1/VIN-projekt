/**
  ******************************************************************************
  * @file    BSP/Src/touchscreen.c
  * @author  MCD Application Team
  * @brief   This example code shows how to use the touchscreen driver.
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

/** @addtogroup STM32H7xx_HAL_Examples
  * @{
  */

/** @addtogroup BSP
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define  CIRCLE_RADIUS        40
#define  LINE_LENGHT          30
/* Private macro -------------------------------------------------------------*/
#define  CIRCLE_XPOS(i)       ((i * 480) / 5)
#define  CIRCLE_YPOS(i)       (272 - CIRCLE_RADIUS - 60)


// Barve za background

#define BC_MAX_HEAT 0xFFFF5A00UL
#define BC_MED_HEAT 0xFFFF9A00UL
#define BC_LOW_HEAT 0xFFFFCE00UL
#define BC_NEUTRAL UTIL_LCD_COLOR_WHITE
#define BC_LOW_COOL 0xFFE2FCFFUL
#define BC_MED_COOL 0xFFCBF1FAUL
#define BC_MAX_COOL 0xFF6BA7CCUL

#define GET_R(color) (((color) >> 16) & 0xFF)
#define GET_G(color) (((color) >> 8) & 0xFF)
#define GET_B(color) ((color) & 0xFF)
#define RGB_TO_COLOR(r,g,b) (0xFF000000UL | ((r) << 16) | ((g) << 8) | (b))

static uint32_t currBackgroundColor = UTIL_LCD_COLOR_WHITE;
static uint32_t targetBackgroundColor = UTIL_LCD_COLOR_WHITE;
static int fadeStepsRemaining = 0;
#define TOTAL_FADE_STEPS 20



/* Private Structures and Enumerations ------------------------------------------------------------*/
/* Global variables ---------------------------------------------------------*/
TS_State_t  TS_State;

/* Private variables ---------------------------------------------------------*/
/* Static variable holding the current touch color index : used to change color at each touch */
TS_Init_t hTS;

uint32_t ts_status = BSP_ERROR_NONE;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Touchscreen Demo1 : test touchscreen calibration and single touch in polling mode
  * @param  None
  * @retval None
  *
  *
  */
void processBackgroundFade(void);
void drawTempControls();
uint8_t is_touch_in_area(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);


uint32_t heatDownBtnPosX = 40;
uint32_t heatDownBtnPosY = 0;
uint32_t heatUpBtnPosX = 0;
uint32_t heatUpBtnPosY = 0;
uint32_t btnW = 60;
uint32_t btnH = 60;


void Touchscreen_demo(void)
{
//  uint16_t x1, y1;
//  uint8_t state = 0;
  uint32_t x_size, y_size;

  BSP_LCD_GetXSize(0, &x_size);
  BSP_LCD_GetYSize(0, &y_size);

  hTS.Width = x_size;
  hTS.Height = y_size;
  hTS.Orientation =TS_SWAP_XY ;
  hTS.Accuracy = 5;

  heatDownBtnPosY = hTS.Height - 90;
  heatUpBtnPosX = hTS.Width - 100;
  heatUpBtnPosY = hTS.Height - 90;


  /* Touchscreen initialization */
  ts_status = BSP_TS_Init(0, &hTS);

  if(ts_status == BSP_ERROR_NONE)
  {
    /* Display touch screen demo description */
	uint32_t x_size, y_size;

	BSP_LCD_GetXSize(0, &x_size);
	BSP_LCD_GetYSize(0, &y_size);


	/* Clear the LCD */
	UTIL_LCD_Clear(UTIL_LCD_COLOR_WHITE);

	/* Set Touchscreen Demo1 description */
//	UTIL_LCD_FillRect(0, 0, x_size, 80, UTIL_LCD_COLOR_BLUE);
	UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
	UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_BLUE);
//	UTIL_LCD_SetFont(&Font24);
//
//	UTIL_LCD_DisplayStringAt(0, 0, (uint8_t *)"Touchscreen basic polling", CENTER_MODE);
	UTIL_LCD_SetFont(&Font16);



//	UTIL_LCD_DisplayStringAt(0, 30, (uint8_t *)"Please use the Touchscreen to", CENTER_MODE);
//	UTIL_LCD_DisplayStringAt(0, 45, (uint8_t *)"activate the colored circle", CENTER_MODE);
//	UTIL_LCD_DisplayStringAt(0, 60, (uint8_t *)"inside the rectangle.", CENTER_MODE);
//
//	UTIL_LCD_DrawRect(10, 90, x_size - 20, y_size - 100, UTIL_LCD_COLOR_BLUE);
//	UTIL_LCD_DrawRect(11, 91, x_size - 22, y_size - 102, UTIL_LCD_COLOR_BLUE);
//
//    Touchscreen_DrawBackground_Circles(state);

//	while (1)
//    {
      /* Check in polling mode in touch screen the touch status and coordinates */
      /* of touches if touch occurred                                           */
//  	  ts_status = BSP_TS_GetState(0, &TS_State);
//  	  if(TS_State.TouchDetected) {
//  		  UTIL_LCD_FillCircle(TS_State.TouchX, TS_State.TouchY, 20, UTIL_LCD_COLOR_BLUE);
//  	  }
//    	HAL_Delay(100);
        /* One or dual touch have been detected          */
        /* Only take into account the first touch so far */

        /* Get X and Y position of the first touch post calibrated */
//        x1 = TS_State.TouchX;
//        y1 = TS_State.TouchY;
//
//        if ((y1 > (CIRCLE_YPOS(1) - CIRCLE_RADIUS)) &&
//            (y1 < (CIRCLE_YPOS(1) + CIRCLE_RADIUS)))
//        {
//          if ((x1 > (CIRCLE_XPOS(1) - CIRCLE_RADIUS)) &&
//              (x1 < (CIRCLE_XPOS(1) + CIRCLE_RADIUS)))
//          {
//            if ((state & 1) == 0)
//            {
//              Touchscreen_DrawBackground_Circles(state);
//              UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);
//              state = 1;
//            }
//          }
//          if ((x1 > (CIRCLE_XPOS(2) - CIRCLE_RADIUS)) &&
//              (x1 < (CIRCLE_XPOS(2) + CIRCLE_RADIUS)))
//          {
//            if ((state & 2) == 0)
//            {
//              Touchscreen_DrawBackground_Circles(state);
//              UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS, UTIL_LCD_COLOR_RED);
//              state = 2;
//            }
//          }
//
//          if ((x1 > (CIRCLE_XPOS(3) - CIRCLE_RADIUS)) &&
//              (x1 < (CIRCLE_XPOS(3) + CIRCLE_RADIUS)))
//          {
//            if ((state & 4) == 0)
//            {
//              Touchscreen_DrawBackground_Circles(state);
//              UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_YELLOW);
//              state = 4;
//            }
//          }
//
//          if ((x1 > (CIRCLE_XPOS(4) - CIRCLE_RADIUS)) &&
//              (x1 < (CIRCLE_XPOS(4) + CIRCLE_RADIUS)))
//          {
//            if ((state & 8) == 0)
//            {
//              Touchscreen_DrawBackground_Circles(state);
//              UTIL_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_GREEN);
//              state = 8;
//            }
//          }
//        }
//
//      } /* of if(TS_State.TouchDetected) */
//      HAL_Delay(20);
//    }
  } /* of if(status == BSP_ERROR_NONE) */
}


void update_display(float set_temp, float measured, float power) {

	uint32_t newTargetColor;

	if (power >= 3) newTargetColor = BC_MAX_HEAT;
	else if (power >= 2) newTargetColor = BC_MED_HEAT;
	else if (power >= 1) newTargetColor = BC_LOW_HEAT;
	else if (power >= 0) newTargetColor = BC_NEUTRAL;
	else if (power >= -1) newTargetColor = BC_LOW_COOL;
	else if (power >= -2) newTargetColor = BC_MED_COOL;
	else newTargetColor = BC_MAX_COOL;

	if (newTargetColor != targetBackgroundColor) {
		targetBackgroundColor = newTargetColor;
		fadeStepsRemaining = TOTAL_FADE_STEPS;
	}

	processBackgroundFade();

	drawTempControls();

    char buf[64];
    sprintf(buf, "Nastavljena temperatura: %.1f C", set_temp);
    UTIL_LCD_DisplayStringAt(10, 10, (uint8_t *)buf, CENTER_MODE);
    sprintf(buf, "Izmerjena temperatura: %.1f C", measured);
    UTIL_LCD_DisplayStringAt(10, 30, (uint8_t *)buf, CENTER_MODE);
    sprintf(buf, "Moc gretja: %.2f", power);
    UTIL_LCD_DisplayStringAt(10, 50, (uint8_t *)buf, CENTER_MODE);
}

void drawTempControls() {

	UTIL_LCD_DrawRect(heatDownBtnPosX, heatDownBtnPosY, btnW, btnH, UTIL_LCD_COLOR_BLUE);
	UTIL_LCD_DrawRect(heatUpBtnPosX, heatUpBtnPosY, btnW, btnH, UTIL_LCD_COLOR_RED);


	ts_status = BSP_TS_GetState(0, &TS_State);
	if(TS_State.TouchDetected) {
		UTIL_LCD_FillCircle(TS_State.TouchX, TS_State.TouchY, 20, UTIL_LCD_COLOR_BLACK);
		if (is_touch_in_area(TS_State.TouchX, TS_State.TouchY, heatDownBtnPosX, heatDownBtnPosY, btnW, btnH)) {
			UTIL_LCD_FillRect(heatDownBtnPosX, heatDownBtnPosY, btnW, btnH, UTIL_LCD_COLOR_BLUE);
			prag = prag - 1;
		}
		if (is_touch_in_area(TS_State.TouchX, TS_State.TouchY, heatUpBtnPosX, heatUpBtnPosY, btnW, btnH)) {
			UTIL_LCD_FillRect(heatUpBtnPosX, heatUpBtnPosY, btnW, btnH, UTIL_LCD_COLOR_RED);
			prag = prag + 1;
		}

	}
}


void drawLoginScreen() { // Hvala ChadGPT
    UTIL_LCD_Clear(UTIL_LCD_COLOR_WHITE);

    // Rectangle position and size
    uint32_t rect_x = hTS.Width/2 - 70;
    uint32_t rect_y = hTS.Height/2 - 40;
    uint32_t rect_w = 140;
    uint32_t rect_h = 80;

    UTIL_LCD_FillRect(rect_x, rect_y, rect_w, rect_h, UTIL_LCD_COLOR_BLUE);

    // Get font size (assuming single line height)
    uint32_t font_h = UTIL_LCD_GetFont()->Height;

    // Calculate center Y position for text
    uint32_t text_y = rect_y + (rect_h / 2) - (font_h / 2);

    // Display text at center X, calculated Y
    char buf[64];
    sprintf(buf, "LOGIN");
    UTIL_LCD_DisplayStringAt(0, text_y, (uint8_t *)buf, CENTER_MODE);

    ts_status = BSP_TS_GetState(0, &TS_State);
    	if(TS_State.TouchDetected) {
    		if (is_touch_in_area(TS_State.TouchX, TS_State.TouchY, rect_x, rect_y, rect_w, rect_h)) {
    			loggedIn = 1;
    			UTIL_LCD_Clear(UTIL_LCD_COLOR_WHITE);
    		}
    	}

}


uint8_t is_touch_in_area(uint32_t touch_x, uint32_t touch_y, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (touch_x >= x && touch_x <= (x + w) &&
        touch_y >= y && touch_y <= (y + h)) {
        return 1;  // touch is inside area
    }
    return 0;      // touch is outside
}

void processBackgroundFade(void) {
    if (fadeStepsRemaining <= 0) return;  // No fade in progress

    int r1 = GET_R(currBackgroundColor);
    int g1 = GET_G(currBackgroundColor);
    int b1 = GET_B(currBackgroundColor);

    int r2 = GET_R(targetBackgroundColor);
    int g2 = GET_G(targetBackgroundColor);
    int b2 = GET_B(targetBackgroundColor);

    int step = TOTAL_FADE_STEPS - fadeStepsRemaining + 1;

    int r = r1 + (r2 - r1) * step / TOTAL_FADE_STEPS;
    int g = g1 + (g2 - g1) * step / TOTAL_FADE_STEPS;
    int b = b1 + (b2 - b1) * step / TOTAL_FADE_STEPS;

    uint32_t newColor = RGB_TO_COLOR(r, g, b);
    UTIL_LCD_Clear(newColor);
    currBackgroundColor = newColor;

    fadeStepsRemaining--;
}



/**
  * @brief  Draw Touchscreen Background
  * @param  state : touch zone state
  * @retval None
  */
void Touchscreen_DrawBackground_Circles(uint8_t state)
{
  uint16_t x, y;
  switch (state)
  {
    case 0:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);

      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS, UTIL_LCD_COLOR_RED);

      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_YELLOW);


      UTIL_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_GREEN);

      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      break;

    case 1:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      break;

    case 2:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS, UTIL_LCD_COLOR_RED);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      break;

    case 4:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_YELLOW);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      break;

    case 8:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(4), CIRCLE_RADIUS, UTIL_LCD_COLOR_GREEN);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(4), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      break;

    case 16:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);

      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);

      UTIL_LCD_DrawHLine(CIRCLE_XPOS(1)-LINE_LENGHT, CIRCLE_YPOS(1), 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawHLine(CIRCLE_XPOS(2)-LINE_LENGHT, CIRCLE_YPOS(2), 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawVLine(CIRCLE_XPOS(2), CIRCLE_YPOS(2)-LINE_LENGHT, 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawHLine(CIRCLE_XPOS(3)-LINE_LENGHT, CIRCLE_YPOS(3), 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawHLine(CIRCLE_XPOS(4)-LINE_LENGHT, CIRCLE_YPOS(4), 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawVLine(CIRCLE_XPOS(4), CIRCLE_YPOS(4)-LINE_LENGHT, 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);

      UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_SetFont(&Font24);
      x = CIRCLE_XPOS(1);
      y = CIRCLE_YPOS(1) - CIRCLE_RADIUS - UTIL_LCD_GetFont()->Height;
      UTIL_LCD_DisplayStringAt(x, y, (uint8_t *)"Volume", CENTER_MODE);
      x = CIRCLE_XPOS(4);
      y = CIRCLE_YPOS(4) - CIRCLE_RADIUS - UTIL_LCD_GetFont()->Height;
      UTIL_LCD_DisplayStringAt(x, y, (uint8_t *)"Frequency", CENTER_MODE);

      break;

    case 32:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLACK);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLACK);

      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);


      UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_BLACK);
      UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_SetFont(&Font20);
      x = CIRCLE_XPOS(1) - 10;
      y = CIRCLE_YPOS(1) - (UTIL_LCD_GetFont()->Height)/2;
      UTIL_LCD_DisplayStringAt(x, y, (uint8_t *)"Up", LEFT_MODE);
      x = CIRCLE_XPOS(2) - 10;
      y = CIRCLE_YPOS(3)  - (UTIL_LCD_GetFont()->Height)/2;
      UTIL_LCD_DisplayStringAt(x, y, (uint8_t *)"Dw", LEFT_MODE);
      UTIL_LCD_SetFont(&Font12);

      break;

    case 64:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);

      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);

      UTIL_LCD_DrawHLine(CIRCLE_XPOS(2)-LINE_LENGHT, CIRCLE_YPOS(2), 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawHLine(CIRCLE_XPOS(3)-LINE_LENGHT, CIRCLE_YPOS(3), 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawVLine(CIRCLE_XPOS(3), CIRCLE_YPOS(3)-LINE_LENGHT, 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);

      UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_SetFont(&Font24);
      x = CIRCLE_XPOS(2);
      y = CIRCLE_YPOS(1) - CIRCLE_RADIUS - UTIL_LCD_GetFont()->Height;
      UTIL_LCD_DisplayStringAt(x, y, (uint8_t *)"Frequency", LEFT_MODE);
      break;
  }
}

/**
  * @brief  TouchScreen get touch position
  * @param  None
  * @retval None
  */
uint8_t TouchScreen_GetTouchPosition(void)
{
  uint16_t x1, y1;
  uint8_t circleNr = 0;

  /* Check in polling mode in touch screen the touch status and coordinates */
  /* of touches if touch occurred                                           */
  BSP_TS_GetState(0, &TS_State);
  if(TS_State.TouchDetected)
  {
    /* One or dual touch have been detected          */
    /* Only take into account the first touch so far */

    /* Get X and Y position of the first */
    x1 = TS_State.TouchX;
    y1 = TS_State.TouchY;

    if ((y1 > (CIRCLE_YPOS(1) - CIRCLE_RADIUS)) &&
        (y1 < (CIRCLE_YPOS(1) + CIRCLE_RADIUS)))
    {
      if ((x1 > (CIRCLE_XPOS(1) - CIRCLE_RADIUS)) &&
          (x1 < (CIRCLE_XPOS(1) + CIRCLE_RADIUS)))
      {
        circleNr = 1;
      }
      if ((x1 > (CIRCLE_XPOS(2) - CIRCLE_RADIUS)) &&
          (x1 < (CIRCLE_XPOS(2) + CIRCLE_RADIUS)))
      {
        circleNr = 2;
      }

      if ((x1 > (CIRCLE_XPOS(3) - CIRCLE_RADIUS)) &&
          (x1 < (CIRCLE_XPOS(3) + CIRCLE_RADIUS)))
      {
        circleNr = 3;
      }

      if ((x1 > (CIRCLE_XPOS(4) - CIRCLE_RADIUS)) &&
          (x1 < (CIRCLE_XPOS(4) + CIRCLE_RADIUS)))
      {
        circleNr = 4;
      }
    }
    else
    {
      if (((y1 < 220) && (y1 > 140)) &&
          ((x1 < 160) && (x1 > 100)))
      {
        circleNr = 0xFE;   /* top part of the screen */
      }
      else
      {
        circleNr = 0xFF;
      }
    }
  } /* of if(TS_State.TouchDetected) */
  return circleNr;
}

/**
  * @}
  */

/**
  * @}
  */

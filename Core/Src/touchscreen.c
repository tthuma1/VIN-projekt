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
#define BC_MAX_COOL 0xFFCBF1FAUL

#define GET_R(color) (((color) >> 16) & 0xFF)
#define GET_G(color) (((color) >> 8) & 0xFF)
#define GET_B(color) ((color) & 0xFF)
#define RGB_TO_COLOR(r,g,b) (0xFF000000UL | ((r) << 16) | ((g) << 8) | (b))

static uint32_t currBackgroundColor = UTIL_LCD_COLOR_WHITE;
static uint32_t targetBackgroundColor = UTIL_LCD_COLOR_WHITE;
static int fadeStepsRemaining = 0;
#define TOTAL_FADE_STEPS 20



/* Private Structures and Enumerations ------------------------------------------------------------*/

typedef struct {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
} Control;

/* Global variables ---------------------------------------------------------*/
TS_State_t  TS_State;
TS_Init_t hTS;
/* Private variables ---------------------------------------------------------*/

Control tempUp;
Control tempDown;
Control alphaUp;
Control alphaDown;

uint32_t lastInteractionTime = 0;

/* Static variable holding the current touch color index : used to change color at each touch */


static float currPrag = 0;
static uint8_t currMin = 0;

float currPower = 999;

uint32_t ts_status = BSP_ERROR_NONE;
/* Private function prototypes -----------------------------------------------*/
void processBackgroundFade(void);
void drawControl(uint16_t, uint16_t, uint8_t, uint32_t);
void drawTime(void);
static uint8_t lerp(uint8_t, uint8_t, float);
uint32_t getInterpolatedColor(float);
void drawText(uint16_t, uint16_t, sFONT, const char*);
void eventListener(void);
uint8_t isPressed(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
/* Private functions ---------------------------------------------------------*/

void update_display(float set_temp, float measured, float power) {

	// ce se power ali background posodobi se vse posodobi drugace nic
	if (power != currPower || fadeStepsRemaining > 0) {

		uint32_t newTargetColor = getInterpolatedColor(power);

		if (newTargetColor != targetBackgroundColor) {
			targetBackgroundColor = newTargetColor;
			fadeStepsRemaining = TOTAL_FADE_STEPS;
		}

		processBackgroundFade();

		UTIL_LCD_SetBackColor(currBackgroundColor);

		char buf[64];

		currPower = power;
		// Narise power
		if(power > 0) sprintf(buf, "Heat: %.2f ", power);
		else if(power < 0) sprintf(buf, "Cool: %.2f ", power);
		else sprintf(buf, "Off: %.2f ", power);
		drawText(30, 30, Font16, buf);


		drawControl(tempUp.x, tempUp.y, 1, UTIL_LCD_COLOR_BLACK); // Narise UP control za set temp
		drawControl(alphaUp.x, alphaUp.y, 1, UTIL_LCD_COLOR_BLACK); // Narise UP control za alpha

		// Spodnje tri funkcije narisejo samo static tekst
		drawText(30, 100, Font16, "Room");
		drawText(hTS.Width/2, 100, Font16, "Set");
		drawText(hTS.Width - 100, 100, Font16, "Alpha");


		sprintf(buf, "%.1f", set_temp);
		drawText(hTS.Width/2, 118, Font24, buf); // Narise set_temp
		drawText(hTS.Width/2 + 50, 118, Font16, "C"); // Narise enoto -> To je treba se pravilno position-at
		sprintf(buf, "%.1f", alpha);
		drawText(hTS.Width - 100, 118, Font24, buf); // Narise alpha

		sprintf(buf, "%.1f", measured);
		drawText(30, 118, Font32, buf); // Narise measured
		drawText(100, 118, Font16, "C"); // Narise enoto -> To je treba se pravilno position-at


		drawControl(tempDown.x, tempDown.y, 0, UTIL_LCD_COLOR_BLACK); // Narise DOWN control za set temp
		drawControl(alphaDown.x, alphaDown.y, 0, UTIL_LCD_COLOR_BLACK); // Narise DOWN control za alpha

		drawTime(); // Narise cas ker zakaj ne
	} else drawTime(); // Tut narise cas v primeru da se power ni spremenu cas pa se je

    eventListener();
}

// Narise puscico
void drawControl(uint16_t x, uint16_t y, uint8_t up, uint32_t color)
{
    if (up)
    {
        // Draw up arrow
        UTIL_LCD_DrawLine(x, y, x-10, y+20, color);
        UTIL_LCD_DrawLine(x, y, x+10, y+20, color);
        UTIL_LCD_DrawLine(x-10, y+20, x+10, y+20, color);
    }
    else
    {
        // Draw down arrow
        UTIL_LCD_DrawLine(x-10, y, x, y+20, color);
        UTIL_LCD_DrawLine(x+10, y, x, y+20, color);
        UTIL_LCD_DrawLine(x-10, y, x+10, y, color);
    }
}

void drawTime(void) { // Narise cas
//    RTC_TimeTypeDef sTime;
//    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
//
//    if (sTime.Minutes == currMin) return; // preveri ce je treba posodobit izpis
//    currMin = sTime.Minutes;
//    char timeString[6];
//    sprintf(timeString, "%02d:%02d", sTime.Hours, sTime.Minutes);
//    drawText(30, hTS.Height - 50, Font20, UTIL_LCD_COLOR_BLACK);
}


void drawLoginScreen(void) { // narise login screen
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
    UTIL_LCD_SetFont(&Font24);
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_BLUE);
    UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_DisplayStringAt(0, text_y, (uint8_t *)buf, CENTER_MODE);

    ts_status = BSP_TS_GetState(0, &TS_State);
    	if(TS_State.TouchDetected) {
    		if (isPressed(TS_State.TouchX, TS_State.TouchY, rect_x, rect_y, rect_w, rect_h)) {
    			loggedIn = 1;
    			UTIL_LCD_Clear(UTIL_LCD_COLOR_WHITE);
    		}
    }
}

void checkActivity(void) { // preveri aktivnost uporabnika
	if (HAL_GetTick() - lastInteractionTime >= 30000) {
		loggedIn = 0;
	}
}

// Basic funkcija, ki izpise neki na zaslon
void drawText(uint16_t x, uint16_t y, sFONT fontSize, const char* content) {
    UTIL_LCD_SetFont(&fontSize);
    UTIL_LCD_DisplayStringAt(x, y, (uint8_t*)content, LEFT_MODE);
}


// preveri ali je bila kaksna iterakcija z zaslonom -> Ne preveri ce je bila interakcija z fizicnim gumbom
void eventListener(void) {
	ts_status = BSP_TS_GetState(0, &TS_State);
	if(TS_State.TouchDetected) {
		uint32_t tx = TS_State.TouchX;
		uint32_t ty = TS_State.TouchY;

		Control active;
		uint8_t up = -1; // ce bo 0 ali 1 je bil nek gumb pressed, sluzi tudi kot boolean za drawControl
		if (isPressed(tx, ty, tempUp.x, tempUp.y, tempUp.w, tempUp.h)) {
			prag = prag + 0.5;
			active = tempUp;
			up = 1;
		}
		if (isPressed(tx, ty, tempDown.x, tempDown.y, tempDown.w, tempDown.h)) {
			prag = prag - 0.5;
			active = tempDown;
			up = 0;
		}
		if (isPressed(tx, ty, alphaUp.x, alphaUp.y, alphaUp.w, alphaUp.h)) {
			alpha = alpha + 0.1;
			active = alphaUp;
			up = 1;
		}
		if (isPressed(tx, ty, alphaDown.x, alphaDown.y, alphaDown.w, alphaDown.h)) {
			alpha = alpha - 0.1;
			active = alphaDown;
			up = 0;
		}
		if (up > -1) { // preveri ce je bil gumb pressed
			drawControl(active.x, active.y, up, UTIL_LCD_COLOR_RED);
		}
		lastInteractionTime = HAL_GetTick();

	}


}

// collison detection za press
uint8_t isPressed(uint32_t touch_x, uint32_t touch_y, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (touch_x >= x && touch_x <= (x + w) &&
        touch_y >= y && touch_y <= (y + h)) {
        return 1;  // touch is inside area
    }
    return 0;      // touch is outside
}


// lorp larp lerp
static uint8_t lerp(uint8_t a, uint8_t b, float t) {
    return (uint8_t)((1.0f - t) * a + t * b);
}


// interpolira barvo based na power, da dobimo targetBackgroundColor
uint32_t getInterpolatedColor(float power) {
    // Normalize power from [-3,3] to [0,1]
    float t = (power + 3.0f) / 6.0f;

    // Clamp t to [0,1] just in case
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    uint8_t minR = GET_R(BC_MAX_COOL);
    uint8_t minG = GET_G(BC_MAX_COOL);
    uint8_t minB = GET_B(BC_MAX_COOL);

    uint8_t maxR = GET_R(BC_MAX_HEAT);
    uint8_t maxG = GET_G(BC_MAX_HEAT);
    uint8_t maxB = GET_B(BC_MAX_HEAT);

    uint8_t r = lerp(minR, maxR, t);
    uint8_t g = lerp(minG, maxG, t);
    uint8_t b = lerp(minB, maxB, t);

    return (r << 16) | (g << 8) | b;
}

// fade efekt za background
void processBackgroundFade(void) {

    if (fadeStepsRemaining <= 0) {
    	return;
    }  // No fade in progress

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
  * @brief  Touchscreen Demo1 : test touchscreen calibration and single touch in polling mode
  * @param  None
  * @retval None
  *
  *
  */
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

  tempUp = (Control){x_size/2, 30, 40, 40};
  tempDown = (Control){x_size/2, y_size - 60, 40, 40};
  alphaUp = (Control){x_size - 80, 30, 40, 40};
  alphaDown = (Control){x_size - 80, y_size - 60, 40, 40};


  /* Touchscreen initialization */
  ts_status = BSP_TS_Init(0, &hTS);

  if(ts_status == BSP_ERROR_NONE)
  {
	UTIL_LCD_Clear(UTIL_LCD_COLOR_WHITE); // Inital screen clear
	UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_BLACK); // Vse bo s crno
  }
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

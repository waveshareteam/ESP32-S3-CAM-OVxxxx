#pragma once
#include <Arduino.h> 

// LCD driver selection, remove front #, leaving only one driver enabled

#define CONFIG_WAVESHARE_2INCH_TOUCH_LCD               1

// LCD rotation Angle selection, remove the front #, leaving only one drive
 #define CONFIG_EXAMPLE_DISPLAY_ROTATION_0_DEGREE       1 
// #define CONFIG_EXAMPLE_DISPLAY_ROTATION_90_DEGREE      1
// #define CONFIG_EXAMPLE_DISPLAY_ROTATION_180_DEGREE     1
//#define CONFIG_EXAMPLE_DISPLAY_ROTATION_270_DEGREE      1


#define TMP_LCD_WIDTH   240
#define TMP_LCD_HEIGHT  320
#define TouchPad_swap_xy     0
#define TouchPad_mirror_x    0
#define TouchPad_mirror_y    0

/*************************************************************************************************************************************************************************************************************************
**************************************************************************************************************************************************************************************************************************/
#ifdef CONFIG_EXAMPLE_DISPLAY_ROTATION_90_DEGREE
  #define EXAMPLE_LCD_WIDTH   TMP_LCD_HEIGHT
  #define EXAMPLE_LCD_HEIGHT  TMP_LCD_WIDTH
  #define Touch_swap_xy     (!TouchPad_swap_xy)
  #define Touch_mirror_x    (!TouchPad_mirror_x)
  #define Touch_mirror_y    (TouchPad_mirror_y)
#elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_180_DEGREE)
  #define EXAMPLE_LCD_WIDTH   TMP_LCD_WIDTH
  #define EXAMPLE_LCD_HEIGHT  TMP_LCD_HEIGHT
  #define Touch_swap_xy     (TouchPad_swap_xy)
  #define Touch_mirror_x    (!TouchPad_mirror_x)
  #define Touch_mirror_y    (!TouchPad_mirror_y)
#elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_270_DEGREE)
  #define EXAMPLE_LCD_WIDTH   TMP_LCD_HEIGHT
  #define EXAMPLE_LCD_HEIGHT  TMP_LCD_WIDTH
  #define Touch_swap_xy     (!TouchPad_swap_xy)
  #define Touch_mirror_x    (TouchPad_mirror_x)
  #define Touch_mirror_y    (!TouchPad_mirror_y)
#else
  #define EXAMPLE_LCD_WIDTH   TMP_LCD_WIDTH
  #define EXAMPLE_LCD_HEIGHT  TMP_LCD_HEIGHT
  #define Touch_swap_xy     (TouchPad_swap_xy)
  #define Touch_mirror_x    (TouchPad_mirror_x)
  #define Touch_mirror_y    (TouchPad_mirror_y)
#endif


//lcd driver gpio config 
#define LCD_HOST  SPI2_HOST

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (80 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_MISO           2
#define EXAMPLE_PIN_NUM_MOSI           1
#define EXAMPLE_PIN_NUM_SCLK           5
#define EXAMPLE_PIN_NUM_LCD_CS         6
#define EXAMPLE_PIN_NUM_LCD_DC         3
#define EXAMPLE_PIN_NUM_LCD_RST        -1
#define EXAMPLE_LCD_PIN_NUM_BK_LIGHT   -1


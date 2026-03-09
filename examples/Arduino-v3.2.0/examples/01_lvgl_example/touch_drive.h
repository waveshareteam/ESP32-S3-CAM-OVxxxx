#pragma once

#include <Arduino.h>
#include "Board_Configuration.h"


#ifdef CONFIG_WAVESHARE_1_47INCH_TOUCH_LCD
  #define TOUCH_MAX_POINTS             (2) 
#elif defined(CONFIG_WAVESHARE_2INCH_TOUCH_LCD)
  #define TOUCH_MAX_POINTS             (1) 
#elif defined(CONFIG_WAVESHARE_2_8INCH_TOUCH_LCD)
  #define TOUCH_MAX_POINTS             (5)
#elif defined(CONFIG_WAVESHARE_3_5INCH_TOUCH_LCD)
  #define TOUCH_MAX_POINTS             (1) 
#endif



enum GESTURE {
  NONE = 0x00,
  SWIPE_UP = 0x01,
  SWIPE_DOWN = 0x02,
  SWIPE_LEFT = 0x03,
  SWIPE_RIGHT = 0x04,
  SINGLE_CLICK = 0x05,
  DOUBLE_CLICK = 0x0B,
  LONG_PRESS = 0x0C
};
struct Touch_Data{
  uint8_t points;    // Number of touch points
  struct {
    uint16_t x; /*!< X coordinate */
    uint16_t y; /*!< Y coordinate */
    uint16_t strength; /*!< Strength */
  }coords[TOUCH_MAX_POINTS];
  GESTURE gesture;    /*!< uint8_t */
};

void touch_driver_init(void);
void touch_read_data(struct Touch_Data *touch_data);
String Touch_GestureName(void);
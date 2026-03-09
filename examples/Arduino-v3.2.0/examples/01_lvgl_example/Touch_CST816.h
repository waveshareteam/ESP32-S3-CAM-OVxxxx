#pragma once

#include "touch_drive.h"
#include <Arduino.h>
#include <Wire.h>


#define CST816_ADDR             0x15
#define CST816_INT_PIN          -1
#define CST816_RST_PIN          -1                      // EXIO1
   
/* CST816 GESTURE */
//debug info
/****************HYN_REG_MUT_DEBUG_INFO_MODE address start***********/
#define CST816_REG_GestureID      0x01
#define CST816_REG_Version        0x15
#define CST816_REG_ChipID         0xA7
#define CST816_REG_ProjID         0xA8
#define CST816_REG_FwVersion      0xA9
#define CST816_REG_AutoSleepTime  0xF9
#define CST816_REG_DisAutoSleep   0xFE

uint8_t cst816_touch_init(void);
uint8_t CST816_touch_read_data(struct Touch_Data *touch_data);

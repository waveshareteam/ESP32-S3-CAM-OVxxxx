#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lcd_driver.h"
#include "touch_drive.h"

#include "Board_Configuration.h"

//#include "Touch_Driver.h"
#include <lvgl.h>
#include "lv_conf.h"
 

#define Backlight_MAX   100


void lvgl_driver_init(void);
void lvgl_loop(void);                                                
void Set_Backlight(uint8_t Light);                   // Call this function to adjust the brightness of the backlight. The value of the parameter Light ranges from 0 to 100


bool lvgl_port_lock(uint32_t timeout_ms);
void lvgl_port_unlock(void);

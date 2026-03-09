#include "esp_err.h"
#include "esp_log.h"
#include <Arduino.h>

#include "i2c.h"
#include "io_extension.h"
#include "lvgl_driver.h"
#include "lcd_driver.h"
#include "touch_drive.h"
#include "Touch_CST816.h"

#include <lvgl.h>
#include "lv_conf.h"
#include <demos/lv_demos.h>

#define KEY_PIN 15                // 按键连接的IO口
#define LONG_PRESS_TIME 1000      // 长按判定时间（毫秒，这里设为1秒）
#define DEBOUNCE_DELAY 50         // 消抖时间（毫秒）

// 状态变量
static bool isPressing = false;    // 是否正在按下
static unsigned long pressStartTime = 0;  // 按下开始时间
static bool longPressTriggered = false;   // 长按是否已触发

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    pinMode(KEY_PIN, INPUT_PULLUP);  
    DEV_I2C_Init();

    IO_EXTENSION_Init();
    IO_EXTENSION_Output(IO_EXTENSION_IO_6, 1);
    IO_EXTENSION_Pwm_Output(100);
    
    lcd_driver_init();
    touch_driver_init();
    lvgl_driver_init();

    lvgl_port_lock(0);

     lv_demo_widgets();
    // lv_demo_benchmark();
    // lv_demo_keypad_encoder();
    // lv_demo_music();
    // lv_demo_stress();

    lvgl_port_unlock();

    
}


void loop()
{
    // 读取按键电平（低电平表示按下）
    int keyLevel = digitalRead(KEY_PIN);
    
    // 按键按下（带消抖）
    if (keyLevel == LOW) {
        if (!isPressing) {
            // 刚按下时记录开始时间
            isPressing = true;
            pressStartTime = millis();
            longPressTriggered = false;  // 重置长按触发状态
            Serial.println("按键已按下，开始计时...");
        } else {
            // 持续按下时判断是否达到长按时间
            if (!longPressTriggered && (millis() - pressStartTime >= LONG_PRESS_TIME)) {
                longPressTriggered = true;
                Serial.println("检测到长按！");
                // 长按关机
                //Set_Exio_Level(LOGIC_GPIO_PIN_5_MASK,0);
                //Set_Exio_PWM_Duty(LOGIC_GPIO_PIN_9_MASK,0); 
                while(1);
                
            }
        }
    } else {
        // 按键释放
        if (isPressing) {
            isPressing = false;
            Serial.println("按键已释放");
        }
    }
    
    delay(10);  
}

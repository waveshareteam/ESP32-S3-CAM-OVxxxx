/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2025 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

#include "esp_lcd_touch_cst328.h"

static const char *TAG = "CST328";

/* Working Mode Register */
#define CST328_REG_DEBUG_INFO_MODE              0xD101
#define CST328_REG_RESET_MODE                   0xD102
#define CST328_REG_DEBUG_RECALIBRATION_MODE     0xD104
#define CST328_REG_DEEP_SLEEP_MODE              0xD105
#define CST328_REG_DEBUG_POINT_MODE             0xD108
#define CST328_REG_NORMAL_MODE                  0xD109

#define CST328_REG_DEBUG_RAWDATA_MODE           0xD10A
#define CST328_REG_DEBUG_DIFF_MODE              0xD10D
#define CST328_REG_DEBUG_FACTORY_MODE           0xD119
#define CST328_REG_DEBUG_FACTORY_MODE_2         0xD120

/* Debug Info Register */
#define CST328_REG_DEBUG_INFO_BOOT_TIME         0xD1FC
#define CST328_REG_DEBUG_INFO_RES_Y             0xD1FA
#define CST328_REG_DEBUG_INFO_RES_X             0xD1F8
#define CST328_REG_DEBUG_INFO_KEY_NUM           0xD1F7
#define CST328_REG_DEBUG_INFO_TP_NRX            0xD1F6
#define CST328_REG_DEBUG_INFO_TP_NTX            0xD1F4

/* data register */
#define CST328_REG_DATA_BASE                    0xD000  // D000 ~ D01A
#define CST328_REG_STATUS                       0xD005  

static esp_err_t esp_lcd_touch_cst328_read_data(esp_lcd_touch_handle_t tp);
static bool esp_lcd_touch_cst328_get_xy(esp_lcd_touch_handle_t tp,
                                        uint16_t *x,
                                        uint16_t *y,
                                        uint16_t *strength,
                                        uint8_t *point_num,
                                        uint8_t max_point_num);
static esp_err_t esp_lcd_touch_cst328_del(esp_lcd_touch_handle_t tp);
static esp_err_t touch_cst328_i2c_read(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len);
static esp_err_t touch_cst328_i2c_write(esp_lcd_touch_handle_t tp, uint16_t reg, const uint8_t *data, uint8_t len);
static esp_err_t touch_cst328_reset(esp_lcd_touch_handle_t tp);
static void touch_cst328_read_cfg(esp_lcd_touch_handle_t tp);

/*******************************************************************************
 * Public API
 *******************************************************************************/

esp_err_t esp_lcd_touch_new_i2c_cst328(const esp_lcd_panel_io_handle_t io,
                                       const esp_lcd_touch_config_t *config,
                                       esp_lcd_touch_handle_t *out_touch)
{
    esp_err_t ret = ESP_OK;
    ESP_RETURN_ON_FALSE(io != NULL, ESP_ERR_INVALID_ARG, TAG, "io is NULL");
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is NULL");
    ESP_RETURN_ON_FALSE(out_touch != NULL, ESP_ERR_INVALID_ARG, TAG, "out_touch is NULL");

    esp_lcd_touch_handle_t tp = heap_caps_calloc(1, sizeof(esp_lcd_touch_t), MALLOC_CAP_DEFAULT);
    ESP_GOTO_ON_FALSE(tp, ESP_ERR_NO_MEM, err, TAG, "no mem for CST328 controller");

    tp->io = io;
    tp->read_data = esp_lcd_touch_cst328_read_data;
    tp->get_xy = esp_lcd_touch_cst328_get_xy;
    tp->del = esp_lcd_touch_cst328_del;
    tp->data.lock.owner = portMUX_FREE_VAL;
    memcpy(&tp->config, config, sizeof(esp_lcd_touch_config_t));

    /* 中断引脚配置（如果使用） */
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t int_gpio_config = {
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = (tp->config.levels.interrupt ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE),
            .pin_bit_mask = BIT64(tp->config.int_gpio_num),
        };
        ret = gpio_config(&int_gpio_config);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "INT GPIO config failed");

        /* 注册中断回调：仿照 CST816S 的做法 */
        if (tp->config.interrupt_callback) {
            esp_lcd_touch_register_interrupt_callback(tp, tp->config.interrupt_callback);
        }
    }

    /* 复位引脚配置（如果使用） */
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t rst_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
            .pin_bit_mask = BIT64(tp->config.rst_gpio_num),
        };
        ret = gpio_config(&rst_gpio_config);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "RST GPIO config failed");
    }

    /* 复位控制器 */
    ret = touch_cst328_reset(tp);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "CST328 reset failed");

    /* 读配置信息（分辨率等，仅调试用） */
    touch_cst328_read_cfg(tp);

    *out_touch = tp;
    return ESP_OK;

err:
    ESP_LOGE(TAG, "Error (0x%x)! Touch controller CST328 initialization failed!", ret);
    if (tp) {
        esp_lcd_touch_cst328_del(tp);
    }
    *out_touch = NULL;
    return ret;
}

/*******************************************************************************
 * Private functions
 *******************************************************************************/

static esp_err_t esp_lcd_touch_cst328_read_data(esp_lcd_touch_handle_t tp)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "tp is NULL");

    // 根据寄存器表：D000 ~ D01A 共 0x1B = 27 字节
    uint8_t buf[27] = {0};
    esp_err_t err = touch_cst328_i2c_read(tp, CST328_REG_DATA_BASE, buf, sizeof(buf));
    ESP_RETURN_ON_ERROR(err, TAG, "I2C read error!");

    // 调试：打印前面几个寄存器，方便观察按下/松手时的变化
    ESP_LOGV(TAG, "RAW D000~D00F: "
                  "%02X %02X %02X %02X %02X %02X %02X %02X "
                  "%02X %02X %02X %02X %02X %02X %02X %02X",
             buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
             buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

    // buf[5] 对应 D005：高 4 位按键标志(0x80)，低 4 位手指数量
    uint8_t status_reg = buf[5];
    uint8_t touch_cnt  = status_reg & 0x0F;

    // 第 1 点的 ID+状态在 D000（buf[0]）
    uint8_t id_state = buf[0];
    uint8_t state    = id_state & 0x0F;   // 低 4 位是状态：按下(0x06)或者抬起(根据你提供的说明)

    // 如果状态不是“按下(0x06)”，即使 D005 里数量不为 0，也认为当前没有有效触点
    if (state != 0x06) {
        touch_cnt = 0;
    }

    if (touch_cnt == 0) {
        taskENTER_CRITICAL(&tp->data.lock);
        tp->data.points = 0;
        taskEXIT_CRITICAL(&tp->data.lock);
        return ESP_OK;
    }

    // 目前先按最多 5 点处理
    if (touch_cnt > 5) {
        touch_cnt = 5;
    }
    if (touch_cnt > CONFIG_ESP_LCD_TOUCH_MAX_POINTS) {
        touch_cnt = CONFIG_ESP_LCD_TOUCH_MAX_POINTS;
    }

    taskENTER_CRITICAL(&tp->data.lock);

    tp->data.points = touch_cnt;

    // 每个点 5 字节：D000~D004, D007~D00B, ...
    // 你给的定义（以第 1 点为例）：
    // D000: ID + 状态
    // D001: X_H = X >> 4
    // D002: Y_H = Y >> 4
    // D003: [7:4] X_L = X & 0x0F, [3:0] Y_L = Y & 0x0F
    // D004: 压力
    for (uint8_t i = 0; i < touch_cnt; i++) {
        uint8_t *p = &buf[i * 5];

        uint16_t x = ((uint16_t)p[1] << 4) | ((p[3] & 0xF0) >> 4);
        uint16_t y = ((uint16_t)p[2] << 4) |  (p[3] & 0x0F);
        uint16_t strength = p[4];

        tp->data.coords[i].x = x;
        tp->data.coords[i].y = y;
        tp->data.coords[i].strength = strength;
    }

    taskEXIT_CRITICAL(&tp->data.lock);

    return ESP_OK;
}

static bool esp_lcd_touch_cst328_get_xy(esp_lcd_touch_handle_t tp,
                                        uint16_t *x,
                                        uint16_t *y,
                                        uint16_t *strength,
                                        uint8_t *point_num,
                                        uint8_t max_point_num)
{
    assert(tp != NULL);
    assert(x != NULL);
    assert(y != NULL);
    assert(point_num != NULL);
    assert(max_point_num > 0);

    taskENTER_CRITICAL(&tp->data.lock);

    uint8_t cnt = tp->data.points;
    if (cnt > max_point_num) {
        cnt = max_point_num;
    }

    for (uint8_t i = 0; i < cnt; i++) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;
        if (strength) {
            strength[i] = tp->data.coords[i].strength;
        }
    }

    *point_num = cnt;
    // 取出后清零，符合 esp_lcd_touch 通用约定
    tp->data.points = 0;

    taskEXIT_CRITICAL(&tp->data.lock);

    return (*point_num > 0);
}

static esp_err_t esp_lcd_touch_cst328_del(esp_lcd_touch_handle_t tp)
{
    if (tp == NULL) {
        return ESP_OK;
    }

    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.int_gpio_num);
    }
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.rst_gpio_num);
    }

    free(tp);
    return ESP_OK;
}

static esp_err_t touch_cst328_reset(esp_lcd_touch_handle_t tp)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "tp is NULL");

    if (tp->config.rst_gpio_num == GPIO_NUM_NC) {
        // 没有接 RST 引脚就不做硬件复位
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, !tp->config.levels.reset),
                        TAG, "GPIO set level error!");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, tp->config.levels.reset),
                        TAG, "GPIO set level error!");
    vTaskDelay(pdMS_TO_TICKS(10));

    return ESP_OK;
}

static void touch_cst328_read_cfg(esp_lcd_touch_handle_t tp)
{
    uint8_t buf[24] = {0};
    assert(tp != NULL);

    // 进入 Debug Info 模式（具体是否需要、写入长度等需参考 CST328 正式文档）
    touch_cst328_i2c_write(tp, CST328_REG_DEBUG_INFO_MODE, NULL, 0);

    // 读 ID / Boot Time 等
    touch_cst328_i2c_read(tp, CST328_REG_DEBUG_INFO_BOOT_TIME, &buf[0], 4);
    ESP_LOGI(TAG, "TouchPad_ID/BootTime:0x%02x,0x%02x,0x%02x,0x%02x",
             buf[0], buf[1], buf[2], buf[3]);

    // 分辨率 X
    touch_cst328_i2c_read(tp, CST328_REG_DEBUG_INFO_RES_X, &buf[0], 1);
    touch_cst328_i2c_read(tp, CST328_REG_DEBUG_INFO_RES_X + 1, &buf[1], 1);
    uint16_t x_max = ((uint16_t)buf[1] << 8) | buf[0];
    ESP_LOGI(TAG, "TouchPad_X_MAX:%d", x_max);

    // 分辨率 Y
    touch_cst328_i2c_read(tp, CST328_REG_DEBUG_INFO_RES_Y, &buf[2], 1);
    touch_cst328_i2c_read(tp, CST328_REG_DEBUG_INFO_RES_Y + 1, &buf[3], 1);
    uint16_t y_max = ((uint16_t)buf[3] << 8) | buf[2];
    ESP_LOGI(TAG, "TouchPad_Y_MAX:%d", y_max);

    // 读一段 TP_NTX 等调试信息
    touch_cst328_i2c_read(tp, CST328_REG_DEBUG_INFO_TP_NTX, buf, 24);
    ESP_LOGI(TAG, "D1F4:0x%02x,0x%02x,0x%02x,0x%02x", buf[0], buf[1], buf[2], buf[3]);
    ESP_LOGI(TAG, "D1F8:0x%02x,0x%02x,0x%02x,0x%02x", buf[4], buf[5], buf[6], buf[7]);
    ESP_LOGI(TAG, "D1FC:0x%02x,0x%02x,0x%02x,0x%02x", buf[8], buf[9], buf[10], buf[11]);
    ESP_LOGI(TAG, "D200:0x%02x,0x%02x,0x%02x,0x%02x", buf[12], buf[13], buf[14], buf[15]);
    ESP_LOGI(TAG, "D204:0x%02x,0x%02x,0x%02x,0x%02x", buf[16], buf[17], buf[18], buf[19]);
    ESP_LOGI(TAG, "D208:0x%02x,0x%02x,0x%02x,0x%02x", buf[20], buf[21], buf[22], buf[23]);

    // 回到 Normal 模式
    touch_cst328_i2c_write(tp, CST328_REG_NORMAL_MODE, NULL, 0);
}

static esp_err_t touch_cst328_i2c_read(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len)
{
    assert(tp != NULL);
    assert(data != NULL);
    return esp_lcd_panel_io_rx_param(tp->io, reg, data, len);
}

static esp_err_t touch_cst328_i2c_write(esp_lcd_touch_handle_t tp, uint16_t reg, const uint8_t *data, uint8_t len)
{
    assert(tp != NULL);
    return esp_lcd_panel_io_tx_param(tp->io, reg, data, len);
}

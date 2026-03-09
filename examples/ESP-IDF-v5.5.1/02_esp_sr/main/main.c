#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

#include "bsp/esp-bsp.h"
#include "mic_speech.h"


static char *TAG = "app main";


static void Speech_event_callback(esp_sr_rec_event_t event,esp_sr_evt_data_t evt_data, void *user_data)
{
    static bool play_flag = 0;
    switch (event)
    {
        case ESP_SR_EVT_AWAKEN:
            ESP_LOGI(TAG, "ESP_SR_EVT_AWAKEN = %d",evt_data.awaken_channel);

            break;
        case ESP_SR_EVT_CMD:
            
            switch (evt_data.sr_cmd)
            {
                case 0:
                    ESP_LOGI(TAG, "Turn on the backlight");
                    break;
                case 1:
                    ESP_LOGI(TAG, "Turn off the backlight");
                    break;
                case 2:
                    ESP_LOGI(TAG, "Backlight is brightest");
                    break;
                case 3:
                    ESP_LOGI(TAG, "Backlight is darkest");
                    break;
                default:
                    break;
            }
            break;
        case ESP_SR_EVT_CMD_TIMEOUT:
            ESP_LOGI(TAG, "ESP_SR_EVT_CMD_TIMEOUT");

            break;
        default:
            break;
    }
}

void app_main()
{
    bsp_io_expander_init();
    vTaskDelay(pdMS_TO_TICKS(100));
    Speech_Init();
    Speech_register_callback(Speech_event_callback);
}




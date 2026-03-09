#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "bsp/esp-bsp.h"

#include "esp_brookesia.hpp"
#include "private/esp_brookesia_utils.h"

#include "dark/stylesheet.hpp"

static char *TAG = "app main";



static void on_clock_update_timer_cb(struct _lv_timer_t *t)
{
    time_t now;
    struct tm timeinfo;
    ESP_Brookesia_Phone *phone = (ESP_Brookesia_Phone *)t->user_data;

    time(&now);
    localtime_r(&now, &timeinfo);

    // char strftime_buf[64];
    // strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    // ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);

    /* Since this callback is called from LVGL task, it is safe to operate LVGL */
    // Update clock on "Status Bar"
    ESP_BROOKESIA_CHECK_FALSE_EXIT(
        phone->getHome().getStatusBar()->setClock(timeinfo.tm_hour, timeinfo.tm_min),
        "Refresh status bar failed"
    );
}

extern "C" void app_main(void)
{

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    bsp_io_expander_init();
    vTaskDelay(pdMS_TO_TICKS(100));
    lv_display_t *disp = bsp_display_start();
    lv_indev_t *tp = bsp_display_get_input_dev();
    bsp_display_backlight_on();
    bsp_sdcard_mount();


    ESP_LOGI(TAG, "Display ESP-Brookesia phone demo");
    /**
     * To avoid errors caused by multiple tasks simultaneously accessing LVGL,
     * should acquire a lock before operating on LVGL.
        */
    bsp_display_lock(0xffff);
    
    /* Create a phone object */
    ESP_Brookesia_Phone *phone = new ESP_Brookesia_Phone(disp);
    ESP_BROOKESIA_CHECK_NULL_EXIT(phone, "Create phone failed");

    /* Try using a stylesheet that corresponds to the resolution */

    #if CONFIG_BSP_LCD_SIZE_2INCH || CONFIG_BSP_LCD_SIZE_2_8INCH
        ESP_Brookesia_PhoneStylesheet_t *stylesheet = new ESP_Brookesia_PhoneStylesheet_t(ESP_BROOKESIA_PHONE_320_240_DARK_STYLESHEET());
    #elif CONFIG_BSP_LCD_SIZE_3_5INCH
        ESP_Brookesia_PhoneStylesheet_t *stylesheet = new ESP_Brookesia_PhoneStylesheet_t(ESP_BROOKESIA_PHONE_320_480_DARK_STYLESHEET());
    #elif  CONFIG_BSP_LCD_SIZE_1_83INCH
        ESP_Brookesia_PhoneStylesheet_t *stylesheet = new ESP_Brookesia_PhoneStylesheet_t(ESP_BROOKESIA_PHONE_284_240_DARK_STYLESHEET());
    #endif

    ESP_BROOKESIA_CHECK_NULL_EXIT(stylesheet, "Create stylesheet failed");

    ESP_LOGI(TAG, "Using stylesheet (%s)", stylesheet->core.name);
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->addStylesheet(stylesheet), "Add stylesheet failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->activateStylesheet(stylesheet), "Activate stylesheet failed");
    delete stylesheet;


    /* Configure and begin the phone */
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->setTouchDevice(bsp_display_get_input_dev()), "Set touch device failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->begin(), "Begin failed");
    // ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->getCoreHome().showContainerBorder(), "Show container border failed");

    /* Install apps */

    PhoneAppSquareline *app_squareline = PhoneAppSquareline::getInstance();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_squareline, "Create app squareline failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_squareline) >= 0), "Install app squareline failed");


    /* Create a timer to update the clock */
    ESP_BROOKESIA_CHECK_NULL_EXIT(lv_timer_create(on_clock_update_timer_cb, 1000, phone), "Create clock update timer failed");
    
    /* Release the lock */
    bsp_display_unlock();
 
    char buffer[128];    /* Make sure buffer is enough for `sprintf` */
    size_t internal_free = 0;
    size_t internal_total = 0;
    size_t external_free = 0;
    size_t external_total = 0;
    while (1) {

        internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
        external_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        external_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        sprintf(buffer, "   Biggest /     Free /    Total\n"
                "\t  SRAM : [%8d / %8d / %8d]\n"
                "\t PSRAM : [%8d / %8d / %8d]",
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), internal_free, internal_total,
                heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM), external_free, external_total);
        //ESP_LOGI(TAG, "%s", buffer);

        /**
         * The `lockLv()` and `unlockLv()` functions are used to lock and unlock the LVGL task.
         * They are registered by the `registerLvLockCallback()` and `registerLvUnlockCallback()` functions.
         */
        bsp_display_lock(0xffff);
        // Update memory label on "Recents Screen"
        if (!phone->getHome().getRecentsScreen()->setMemoryLabel(internal_free / 1024, internal_total / 1024,
                external_free / 1024, external_total / 1024)) {
            ESP_LOGE(TAG, "Set memory label failed");
        }
        bsp_display_unlock();


        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

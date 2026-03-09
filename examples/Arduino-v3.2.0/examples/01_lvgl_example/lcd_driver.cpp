#include "lcd_driver.h"
#include "io_extension.h"
#include "Board_Configuration.h"
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define Offset_X 0
#define Offset_Y 0

static const char *TAG_LCD = "ST7789";

esp_lcd_panel_handle_t panel_handle = NULL;

static void lcd_reset(void)
{
    IO_EXTENSION_Output(IO_EXTENSION_IO_1, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    IO_EXTENSION_Output(IO_EXTENSION_IO_1, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
}

void lcd_driver_init(void)
{
    ESP_LOGI(TAG_LCD, "Initialize SPI bus");                                            
    spi_bus_config_t buscfg = {                                                    
        .mosi_io_num = EXAMPLE_PIN_NUM_MOSI,                                            
        .miso_io_num = EXAMPLE_PIN_NUM_MISO,                                                 
        .sclk_io_num = EXAMPLE_PIN_NUM_SCLK,                                            
        .quadwp_io_num = -1,                                                            
        .quadhd_io_num = -1,                                                            
        .max_transfer_sz = EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT * sizeof(uint16_t),    
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));            

    ESP_LOGI(TAG_LCD, "Install panel IO");                                              
    esp_lcd_panel_io_handle_t io_handle = NULL;                                         
    esp_lcd_panel_io_spi_config_t io_config = { 
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,                                            
        .dc_gpio_num = EXAMPLE_PIN_NUM_LCD_DC,
        .spi_mode = 0,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    

    esp_lcd_panel_dev_st7789t_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_LOGI(TAG_LCD, "Install ST7789 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789t(io_handle, &panel_config, &panel_handle));



    lcd_reset();
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
                    

    #ifdef CONFIG_EXAMPLE_DISPLAY_ROTATION_90_DEGREE
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false));
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    #elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_180_DEGREE)
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, true));
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false));
    #elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_270_DEGREE)
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    #else
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    #endif




    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

}




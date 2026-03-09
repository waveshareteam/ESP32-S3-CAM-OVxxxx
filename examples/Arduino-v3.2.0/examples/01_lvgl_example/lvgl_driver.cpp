#include "lvgl_driver.h"

#include <Arduino.h>
#define DRAW_BUF_SIZE (EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))


uint16_t draw_buf[DRAW_BUF_SIZE / 2];
uint32_t bufSize;
lv_color_t *disp_draw_buf1;
lv_color_t *disp_draw_buf2;
static StaticSemaphore_t lvgl_mutex_buf;
static SemaphoreHandle_t lvgl_mutex = NULL;
static struct Touch_Data touch_data;



/**
 * 初始化 LVGL 互斥锁
 * 应在 lv_init() 之前调用
 */
static void lvgl_port_lock_init(void) {
    if(lvgl_mutex == NULL) {
        // 修复：传递静态缓冲区给函数
        lvgl_mutex = xSemaphoreCreateRecursiveMutexStatic(&lvgl_mutex_buf);
        if(lvgl_mutex == NULL) {
            Serial.println("Failed to create LVGL recursive mutex!");
        }
    }
}

/**
 * 锁定 LVGL 操作
 * @param timeout_ms 超时时间（毫秒），0 表示无限等待
 * @return 成功返回 true，失败返回 false
 */
bool lvgl_port_lock(uint32_t timeout_ms) {
    if(lvgl_mutex == NULL) return false;

    TickType_t timeout_ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    
    // 递归锁定
    if(xSemaphoreTakeRecursive(lvgl_mutex, timeout_ticks) == pdTRUE) {
        return true;
    } else {
        Serial.println("LVGL lock timeout!");
        return false;
    }
}

/**
 * 解锁 LVGL 操作
 */
void lvgl_port_unlock(void) {
    if(lvgl_mutex != NULL) {
        // 递归解锁
        xSemaphoreGiveRecursive(lvgl_mutex);
    }
}

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{

  int offsetx1 = area->x1;
  int offsetx2 = area->x2;
  int offsety1 = area->y1;
  int offsety2 = area->y2;


  #ifdef CONFIG_LVGL_COLOR_16_SWAP 
    uint32_t size = (offsetx2 - offsetx1 +1 ) * (offsety2 - offsety1 + 1);
    uint16_t * color = (uint16_t *)px_map;
    for (size_t i = 0; i < size; i++) {
      color[i] = (((color[i] >> 8) & 0xFF) | ((color[i] << 8) & 0xFF00));
    }
  #endif


  esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 +1, offsety2 + 1, (uint16_t*)px_map);

  lv_display_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_t * indev_drv, lv_indev_data_t * data )
{
    touch_read_data(&touch_data);
  if(Touch_mirror_x){
    if(!Touch_swap_xy)
      touch_data.coords[0].x = EXAMPLE_LCD_WIDTH - touch_data.coords[0].x;
    else
      touch_data.coords[0].x = EXAMPLE_LCD_HEIGHT - touch_data.coords[0].x;
  }
  if(Touch_mirror_y){
    if(!Touch_swap_xy)
      touch_data.coords[0].y = EXAMPLE_LCD_HEIGHT - touch_data.coords[0].y;
    else
      touch_data.coords[0].y = EXAMPLE_LCD_WIDTH - touch_data.coords[0].y;
  }
  if(Touch_swap_xy){
    uint16_t swap_xy = touch_data.coords[0].x;
    touch_data.coords[0].x = touch_data.coords[0].y;
    touch_data.coords[0].y = swap_xy;
  }
  if (touch_data.points != 0x00) {
    data->point.x = touch_data.coords[0].x;
    data->point.y = touch_data.coords[0].y;
    data->state = LV_INDEV_STATE_PR;
    //printf("LVGL : X=%u Y=%u points=%d\r\n",  touch_data.coords[0].x , touch_data.coords[0].y,touch_data.points);
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
  if (touch_data.gesture != NONE ) {    
  }
  touch_data.coords[0].x = 0;
  touch_data.coords[0].y = 0;
  touch_data.points = 0;
  touch_data.gesture = NONE;
}

/*use Arduinos millis() as tick source*/
static uint32_t my_tick(void)
{
    return millis();
}

void LVGL_Loop(void *parameter)
{
    while(1)
    {
      lvgl_port_lock(0);
      lv_timer_handler(); /* let the GUI do its work */
      lvgl_port_unlock();
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

void lvgl_driver_init(void)
{
  lvgl_port_lock_init();

  bufSize = EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT;
  disp_draw_buf1 = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  disp_draw_buf2 = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  lv_init();

  /*Set a tick source so that LVGL will know how much time elapsed. */
  lv_tick_set_cb(my_tick);


  lv_display_t * disp;

  /*Else create a display yourself*/
  disp = lv_display_create(EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, disp_draw_buf1, disp_draw_buf2, bufSize*2, LV_DISPLAY_RENDER_MODE_FULL);

  /*Initialize the (dummy) input device driver*/
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); //Touchpad should have POINTER type
  lv_indev_set_read_cb(indev, my_touchpad_read);
  

  xTaskCreatePinnedToCore(LVGL_Loop, "LVGL Loop",8196, NULL, 4, NULL,0);
  

}


#include "lvgl_app_camera.hpp"
#include "lvgl.h"
#include "esp_brookesia.hpp"
#include "private/esp_brookesia_utils.h"
#include "bsp/esp-bsp.h"


using namespace std;
using namespace esp_brookesia::gui;


static char *TAG = "app camera";
static lv_display_t *p_disp = NULL;
static TaskHandle_t cam_task_handle = NULL;
static lv_obj_t *list1;
static lv_style_t style_list;
static lv_style_t style_list_btn;
static lv_style_t style_list_text;
static lv_style_t style_list_btn_pressed;
static lv_style_t style_list_btn_hover; 
static lv_obj_t *cam_play_bg = NULL;

sensor_t *g_sensor = NULL;
LV_IMG_DECLARE(icon_camera);

PhoneCameraConf::PhoneCameraConf(bool use_status_bar, bool use_navigation_bar):
    ESP_Brookesia_PhoneApp("Camera", &icon_camera, true, use_status_bar, use_navigation_bar)
{
}

PhoneCameraConf::PhoneCameraConf():
    ESP_Brookesia_PhoneApp("Camera", &icon_camera, true)
{
}

PhoneCameraConf::~PhoneCameraConf()
{
    ESP_BROOKESIA_LOGD("Destroy(@0x%p)", this);

}

static void button_boot_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event((button_handle_t)arg);
    switch(event)
    {
        case BUTTON_SINGLE_CLICK:
            ESP_LOGI(TAG, "boot BUTTON_SINGLE_CLICK", );
            break;
        default:break;
    }
    
}

static void button_pwr_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event((button_handle_t)arg);
    switch(event)
    {
        case BUTTON_SINGLE_CLICK:
            ESP_LOGI(TAG, "pwr BUTTON_SINGLE_CLICK", );
            ESP_ERROR_CHECK(esp_lv_adapter_set_dummy_draw(p_disp, false));
            break;
        default:break;
    }
    
}


static void vtask_camera_pic(void *pvParameters) 
{
    while (1)
    {
        if(esp_lv_adapter_get_dummy_draw_enabled(p_disp))
        {
            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) {
                ESP_LOGE(TAG, "Camera capture failed");
                break;
            }

            #if CONFIG_BSP_LCD_SIZE_2INCH || CONFIG_BSP_LCD_SIZE_2_8INCH
            esp_lv_adapter_dummy_draw_blit(p_disp, 0, 0, 320, 240, fb->buf, true);
            #elif CONFIG_BSP_LCD_SIZE_3_5INCH
            esp_lv_adapter_dummy_draw_blit(p_disp, 0, 0, 320, 480, fb->buf, true);
            #elif  CONFIG_BSP_LCD_SIZE_1_83INCH
            esp_lv_adapter_dummy_draw_blit(p_disp, 22, 0, 262, 240, fb->buf, true);
            #endif

            

            esp_camera_fb_return(fb);
        }
        vTaskDelay(pdMS_TO_TICKS(33));
    }
    
} 



static void btn_open_cam_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ESP_ERROR_CHECK(esp_lv_adapter_set_dummy_draw(p_disp, true));

        bsp_display_lock(0xffff);
        lv_obj_remove_flag(cam_play_bg, LV_OBJ_FLAG_HIDDEN);
        bsp_display_unlock();
    }
}

/* VFLIP 开关回调 */
static void vflip_switch_event_cb(uint8_t state)
{
    if (!g_sensor) {
            return;
        }
    g_sensor->set_vflip(g_sensor, state);

}

/* HMIRROR 开关回调 */
static void hmirror_switch_event_cb(uint8_t state)
{
    if (!g_sensor) {
        return;
    }
    g_sensor->set_hmirror(g_sensor, state);
}


// 定义用户回调函数类型（接收开关状态：1=开，0=关，返回void）
typedef void (*switch_state_cb_t)(uint8_t state);

/**
 * 开关状态变化通用回调（内部函数）
 * 执行逻辑：获取开关状态 → 执行用户回调（传入状态值）
 */
static void switch_state_change_cb(lv_event_t *e)
{
    // 1. 获取开关对象和当前状态（1=开，0=关）
    lv_obj_t *switch_obj = (lv_obj_t *)lv_event_get_target(e);
    uint8_t cur_state = lv_obj_has_state(switch_obj, LV_STATE_CHECKED) ? 1 : 0;

    // 2. 执行用户回调（如果不为NULL）
    switch_state_cb_t user_cb = (switch_state_cb_t)lv_obj_get_user_data(switch_obj);
    if (user_cb != NULL)
    {
        user_cb(cur_state); // 直接传入开关状态，调用用户回调
    }
}

/**
 * 带开关的按钮创建函数（替换滑动条为Switch）
 * @param icon          按钮左侧图标
 * @param text          按钮文本
 * @param user_cb       用户回调函数（参数：开关状态(0=关/1=开)，返回void；NULL则无回调）
 */
static void add_button_with_switch(const void *icon, const char *text, switch_state_cb_t user_cb)
{
    // 1. 创建列表按钮（保留基础样式，禁用点击）
    lv_obj_t *btn = lv_list_add_button(list1, icon, text);

    // 2. 创建Switch开关控件（适配按钮布局）
    lv_obj_t *switch_obj = lv_switch_create(btn);
    lv_obj_align(switch_obj, LV_ALIGN_RIGHT_MID, -12, 0); // 右侧对齐
    lv_obj_set_size(switch_obj, 50, 30);                  // 开关尺寸适配按钮高度

    // Switch样式优化（橙色主题，匹配之前的滑动条风格）
    // 关闭状态背景
    lv_obj_set_style_bg_color(switch_obj, lv_color_hex(0xE9EEF5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(switch_obj, lv_color_hex(0xFF8C00), LV_PART_MAIN | LV_STATE_CHECKED); // 开启状态背景
    // 滑块样式
    lv_obj_set_style_bg_color(switch_obj, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_radius(switch_obj, 15, LV_PART_MAIN);   // 开关圆角
    lv_obj_set_style_radius(switch_obj, 12, LV_PART_INDICATOR); // 滑块圆角
    lv_obj_set_style_shadow_width(switch_obj, 2, LV_PART_INDICATOR);
    lv_obj_set_style_shadow_color(switch_obj, lv_color_hex(0xCC8800), LV_PART_INDICATOR);
    lv_obj_set_style_shadow_opa(switch_obj, LV_OPA_30, LV_PART_INDICATOR);

    // 4. 保存用户回调到开关对象，绑定通用回调
    lv_obj_set_user_data(switch_obj, (void*)user_cb);
    lv_obj_add_event_cb(switch_obj, switch_state_change_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void stop_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        lv_obj_add_flag(cam_play_bg, LV_OBJ_FLAG_HIDDEN);
        esp_lv_adapter_refresh_now(p_disp);
        ESP_ERROR_CHECK(esp_lv_adapter_set_dummy_draw(p_disp, false));
    }
}

static void camera_app_ui_init(void)
{

    g_sensor = esp_camera_sensor_get();
    sensor_id_t id = g_sensor->id;
    camera_sensor_info_t *info = esp_camera_sensor_get_info(&id);

    // 清空当前屏幕
    lv_obj_clean(lv_screen_active());

    // ===================== 初始化样式对象 =====================
    lv_style_init(&style_list);
    lv_style_init(&style_list_btn);
    lv_style_init(&style_list_text);
    lv_style_init(&style_list_btn_pressed);
    lv_style_init(&style_list_btn_hover);  // 初始化悬浮样式

    // ===================== 列表容器样式（浅色主容器） =====================
    lv_style_set_pad_all(&style_list, 8);          // 增加内边距更宽松
    lv_style_set_pad_row(&style_list, 4);          // 行间距优化
    lv_style_set_border_width(&style_list, 0);
    lv_style_set_radius(&style_list, 12);          // 更大圆角更柔和
    lv_style_set_bg_color(&style_list, lv_color_hex(0xF8F9FA));  // 浅灰背景
    lv_style_set_bg_opa(&style_list, LV_OPA_COVER);
    lv_style_set_shadow_width(&style_list, 2);     // 轻微阴影提升层次
    lv_style_set_shadow_color(&style_list, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&style_list, LV_OPA_10);
    lv_style_set_shadow_offset_y(&style_list, 2);

    // ===================== 列表按钮默认样式（白底按钮） =====================
    lv_style_set_height(&style_list_btn, 48);      // 按钮高度增加更易点击
    lv_style_set_pad_left(&style_list_btn, 12);    // 左内边距优化
    lv_style_set_pad_right(&style_list_btn, 12);   // 右内边距优化
    lv_style_set_border_width(&style_list_btn, 0);
    lv_style_set_radius(&style_list_btn, 8);       // 按钮圆角
    lv_style_set_bg_color(&style_list_btn, lv_color_hex(0xFFFFFF));  // 按钮白底
    lv_style_set_text_font(&style_list_btn, &lv_font_montserrat_18);
    lv_style_set_text_color(&style_list_btn, lv_color_hex(0x333333));  // 深灰文本
    // 底部分隔线样式
    lv_style_set_border_width(&style_list_btn, 1);
    lv_style_set_border_side(&style_list_btn, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_color(&style_list_btn, lv_color_hex(0xE5E7EB));  // 浅灰分隔线

    // ===================== 分类文本样式（浅灰标题） =====================
    lv_style_set_text_font(&style_list_text, &lv_font_montserrat_18);
    lv_style_set_text_color(&style_list_text, lv_color_hex(0x6C757D));  // 浅灰标题
    lv_style_set_bg_color(&style_list_text, lv_color_hex(0xF8F9FA));    // 和列表背景一致
    lv_style_set_bg_opa(&style_list_text, LV_OPA_COVER);
    lv_style_set_pad_left(&style_list_text, 12);    // 左对齐按钮
    lv_style_set_pad_top(&style_list_text, 10);     // 分类顶部间距
    lv_style_set_pad_bottom(&style_list_text, 6);   // 分类底部间距

    // ===================== 按钮按下样式（淡蓝反馈） =====================
    lv_style_set_bg_color(&style_list_btn_pressed, lv_color_hex(0xEFF6FF));  // 淡蓝背景
    lv_style_set_text_color(&style_list_btn_pressed, lv_color_hex(0x0D6EFD));  // 蓝色文本
    lv_style_set_border_color(&style_list_btn_pressed, lv_color_hex(0xE5E7EB));  // 分隔线保留

    // ===================== 按钮悬浮样式（轻微高亮） =====================
    lv_style_set_bg_color(&style_list_btn_hover, lv_color_hex(0xF9FBFF));  // 极浅蓝悬浮
    lv_style_set_text_color(&style_list_btn_hover, lv_color_hex(0x333333));

    // ===================== 屏幕背景（全局浅色） =====================
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xF5F5F5), LV_PART_MAIN);  // 全局浅灰
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, LV_PART_MAIN);

    // ===================== 创建列表容器 =====================
    list1 = lv_list_create(lv_screen_active());
    lv_obj_add_style(list1, &style_list, 0);
    lv_obj_set_size(list1, 280, 200);  // 适度放大容器更美观
    lv_obj_center(list1);
    lv_obj_set_scrollbar_mode(list1, LV_SCROLLBAR_MODE_OFF);
    // 列表内边距优化
    lv_obj_set_style_pad_top(list1, 8, 0);
    lv_obj_set_style_pad_bottom(list1, 8, 0);

    char buff[30] = {0};
    //Camera message 
    lv_obj_t *text_obj = lv_list_add_text(list1, "Camera message ");
    lv_obj_add_style(text_obj, &style_list_text, 0);

    lv_list_add_button(list1, LV_SYMBOL_LIST, info->name);

    memset(buff,0,30);
    sprintf(buff,"sccb_addr:0x%02X",info->sccb_addr);
    lv_list_add_button(list1, LV_SYMBOL_LIST, buff);

    memset(buff,0,30);
    sprintf(buff,"pid:0x%02X",info->pid);
    lv_list_add_button(list1, LV_SYMBOL_LIST, buff);
    
    // Camera ctrl 
    text_obj = lv_list_add_text(list1, "Camera ctrl");
    lv_obj_add_style(text_obj, &style_list_text, 0);
    add_button_with_switch(&LV_SYMBOL_SHUFFLE, "hmirror",hmirror_switch_event_cb);
    add_button_with_switch(&LV_SYMBOL_SHUFFLE, "vflip",vflip_switch_event_cb);
    lv_obj_t *btn = lv_list_add_button(list1, LV_SYMBOL_SHUFFLE, "open Camera");
    lv_obj_add_event_cb(btn, btn_open_cam_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *top_layer = lv_layer_top();
    cam_play_bg = lv_button_create(top_layer);
    lv_obj_set_size(cam_play_bg, 300, 300);
    lv_obj_set_style_bg_color(cam_play_bg, lv_color_hex(0x000000), LV_STATE_DEFAULT); 
    lv_obj_add_event_cb(cam_play_bg, stop_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(cam_play_bg, LV_ALIGN_CENTER, 0, 0);
    //lv_obj_add_flag(cam_play_bg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(cam_play_bg);
    esp_lv_adapter_refresh_now(p_disp);
}

bool PhoneCameraConf::run(void)
{
    ESP_BROOKESIA_LOGD("Run");
    camera_app_ui_init();
    p_disp = bsp_display_get_disp_dev();
    if(p_disp!=NULL && cam_task_handle==NULL)
    {
        xTaskCreatePinnedToCore(vtask_camera_pic, "vtask_camera_pic task",1024*4, NULL, 2, &cam_task_handle, 0);
    }
    
    ESP_ERROR_CHECK(esp_lv_adapter_set_dummy_draw(p_disp, true));
    lv_obj_remove_flag(cam_play_bg, LV_OBJ_FLAG_HIDDEN);
    return true;
}

bool PhoneCameraConf::back(void)
{
    ESP_BROOKESIA_LOGD("Back");

    // If the app needs to exit, call notifyCoreClosed() to notify the core to close the app
    ESP_BROOKESIA_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");

    return true;
}


bool PhoneCameraConf::close(void)
{
    ESP_BROOKESIA_LOGD("Close");

    /* Do some operations here if needed */
    ESP_BROOKESIA_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");

     if(cam_task_handle!=NULL)
    {
        vTaskDelete(cam_task_handle);
        cam_task_handle = NULL;
    }

    return true;
}


bool PhoneCameraConf::init()
{
    ESP_BROOKESIA_LOGD("Init");
    
    camera_config_t camera_config = BSP_CAMERA_DEFAULT_CONFIG;
    camera_config.grab_mode = CAMERA_GRAB_LATEST;
    #if CONFIG_BSP_LCD_SIZE_2INCH || CONFIG_BSP_LCD_SIZE_2_8INCH
        camera_config.frame_size = FRAMESIZE_QVGA;
    #elif CONFIG_BSP_LCD_SIZE_3_5INCH
        camera_config.frame_size = FRAMESIZE_320X480;
    #elif  CONFIG_BSP_LCD_SIZE_1_83INCH
        camera_config.frame_size = FRAMESIZE_240X240;
    #endif

    esp_err_t err = esp_camera_init(&camera_config);
    /* Do some initialization here if needed */

    return true;
}

bool PhoneCameraConf::deinit()
{
    ESP_BROOKESIA_LOGD("Deinit");

    /* Do some deinitialization here if needed */

    return true;
}

bool PhoneCameraConf::pause()
{
    ESP_BROOKESIA_LOGD("Pause");

    /* Do some operations here if needed */

    return true;
}

bool PhoneCameraConf::resume()
{
    ESP_BROOKESIA_LOGD("Resume");

    /* Do some operations here if needed */
    ESP_ERROR_CHECK(esp_lv_adapter_set_dummy_draw(p_disp, true));
    lv_obj_remove_flag(cam_play_bg, LV_OBJ_FLAG_HIDDEN);
    return true;
}

bool PhoneCameraConf::cleanResource()
{
    ESP_BROOKESIA_LOGD("Clean resource");
    
    /* Do some cleanup here if needed */

    return true;
}

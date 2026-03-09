#include "mic_speech.h"

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "model_path.h"
#include "esp_process_sdkconfig.h"

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

static const char *TAG = "App/Speech";

#define ADC_I2S_CHANNEL 4
int wakeup_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;
static volatile int task_flag = 0;
srmodel_list_t *models = NULL;
static esp_sr_event_callback_t spench_callback =  NULL;
static esp_codec_dev_handle_t record_dev = NULL;

esp_err_t esp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_read;
    int audio_chunksize = buffer_len / (sizeof(int16_t) * ADC_I2S_CHANNEL);

    ret = esp_codec_dev_read(record_dev, (void *)buffer, buffer_len);
    if (!is_get_raw_channel) {
        for (int i = 0; i < audio_chunksize; i++) {
            int16_t ref = buffer[4 * i + 0];
            buffer[3 * i + 0] = buffer[4 * i + 1];
            buffer[3 * i + 1] = buffer[4 * i + 3];
            buffer[3 * i + 2] = ref;
        }
    }

    return ret;
}

int esp_get_feed_channel(void)
{
    return ADC_I2S_CHANNEL;
}

char* esp_get_input_format(void)
{
    return "RMNM";
}

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_feed_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    assert(nch == feed_channel);
    //int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    int16_t *i2s_buff = heap_caps_malloc(audio_chunksize * sizeof(int16_t) * feed_channel,MALLOC_CAP_SPIRAM);

    assert(i2s_buff);

    esp_task_wdt_add(NULL);  
    while (task_flag) {
        esp_get_feed_data(true, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);
        afe_handle->feed(afe_data, i2s_buff);
        esp_task_wdt_reset();
    }
    if (i2s_buff) {
        free(i2s_buff);
        i2s_buff = NULL;
    }
    vTaskDelete(NULL);
}

void detect_Task(void *arg)
{
    uint8_t backlight_last = 0;
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_ENGLISH);
    printf("multinet:%s\n", mn_name);
    esp_mn_iface_t *multinet = esp_mn_handle_from_name(mn_name);
    model_iface_data_t *model_data = multinet->create(mn_name, 8000);
    int mu_chunksize = multinet->get_samp_chunksize(model_data);
    esp_mn_commands_update_from_sdkconfig(multinet, model_data); // Add speech commands from sdkconfig
    assert(mu_chunksize == afe_chunksize);
    //print active speech commands
    multinet->print_active_speech_commands(model_data);

    esp_task_wdt_add(NULL);    

    while (task_flag) {
        afe_fetch_result_t* res = afe_handle->fetch(afe_data); 
        if (!res || res->ret_value == ESP_FAIL) {
            printf("fetch error!\n");
            break;
        }

        if (res->wakeup_state == WAKENET_DETECTED) {
            //printf("WAKEWORD DETECTED\n");
	        multinet->clean(model_data);
        }

        if (res->raw_data_channels == 1 && res->wakeup_state == WAKENET_DETECTED) {
            wakeup_flag = 1;
        } else if (res->raw_data_channels > 1 && res->wakeup_state == WAKENET_CHANNEL_VERIFIED) {
            // For a multi-channel AFE, it is necessary to wait for the channel to be verified.
            //printf("AFE_FETCH_CHANNEL_VERIFIED, channel index: %d\n", res->trigger_channel_id);
            esp_sr_evt_data_t evtdata;
            evtdata.awaken_channel = res->trigger_channel_id;
            if(spench_callback!=NULL)
            {
                spench_callback(ESP_SR_EVT_AWAKEN,evtdata,NULL);
            }
            wakeup_flag = 1;
        }

        esp_task_wdt_reset();
        if (wakeup_flag == 1) {
            
            esp_mn_state_t mn_state = multinet->detect(model_data, res->data);

            if (mn_state == ESP_MN_STATE_DETECTING) {
                continue;
            }

            if (mn_state == ESP_MN_STATE_DETECTED) {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);

                ESP_LOGI(TAG, "TOP %d, command_id: %d, phrase_id: %d, string:%s prob: %f\n", 
                1, mn_result->command_id[0], mn_result->phrase_id[0], mn_result->string, mn_result->prob[0]);

                esp_sr_evt_data_t evtdata;
                evtdata.sr_cmd = mn_result->command_id[0];
                if(spench_callback!=NULL)
                {
                    spench_callback(ESP_SR_EVT_CMD,evtdata,NULL);
                }
            }

            if (mn_state == ESP_MN_STATE_TIMEOUT) {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                printf("timeout, string:%s\n", mn_result->string);
                afe_handle->enable_wakenet(afe_data);
                wakeup_flag = 0;

                esp_sr_evt_data_t evtdata;
                if(spench_callback!=NULL)
                {
                    spench_callback(ESP_SR_EVT_CMD_TIMEOUT,evtdata,NULL);
                }

                continue;
            }

        }
        esp_task_wdt_reset();
        
    }
    if (model_data) {
        multinet->destroy(model_data);
        model_data = NULL;
    }
    printf("detect exit\n");
    vTaskDelete(NULL);
}

void Speech_Init(void)
{
    record_dev = bsp_audio_codec_microphone_init();
    esp_codec_dev_sample_info_t fs = {
                .bits_per_sample = 32,
                .channel = 2,
                .sample_rate = 16000,
            };
    esp_codec_dev_set_in_gain(record_dev, 42.0);
    esp_codec_dev_open(record_dev, &fs);

    models = esp_srmodel_init("model"); // partition label defined in partitions.csv
    afe_config_t *afe_config = afe_config_init(esp_get_input_format(), models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    afe_handle = esp_afe_handle_from_config(afe_config);
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);
    afe_config_free(afe_config);

    task_flag = 1;
    xTaskCreatePinnedToCore(&detect_Task, "detect", 8 * 1024, (void*)afe_data, 5, NULL, 1);
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);
}


esp_err_t Speech_register_callback(esp_sr_event_callback_t callback) 
{
    if (!callback) return ESP_ERR_INVALID_ARG;
    spench_callback = callback;
    return ESP_OK;
}
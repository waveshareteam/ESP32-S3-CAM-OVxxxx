#include <Arduino.h>

#include "esp_check.h"
#include "es8311.h"
#include "io_extension.h"


#include "FS.h"
#include "SD_MMC.h"
#include "Audio.h"

int clk = 16;
int cmd = 43;
int d0 = 44;
Audio audio;


#define I2S_BCK_PIN GPIO_NUM_11   
#define I2S_LRCK_PIN GPIO_NUM_12  
#define I2S_DOUT_PIN GPIO_NUM_14  
#define I2S_MCLK_PIN GPIO_NUM_10

#define EXAMPLE_SAMPLE_RATE (16000)
#define EXAMPLE_MCLK_MULTIPLE (256)  // If not using 24-bit data width, 256 should be enough
#define EXAMPLE_MCLK_FREQ_HZ (EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE)
#define EXAMPLE_VOICE_VOLUME (65)

static esp_err_t es8311_codec_init(void) {
  es8311_handle_t es_handle = es8311_create(I2C_NUM_0, ES8311_ADDRRES_0);
  ESP_RETURN_ON_FALSE(es_handle, ESP_FAIL, TAG, "es8311 create failed");
  const es8311_clock_config_t es_clk = {
    .mclk_inverted = false,
    .sclk_inverted = false,
    .mclk_from_mclk_pin = true,
    .mclk_frequency = EXAMPLE_MCLK_FREQ_HZ,
    .sample_frequency = EXAMPLE_SAMPLE_RATE
  };

  ESP_ERROR_CHECK(es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16));
  ESP_RETURN_ON_ERROR(es8311_voice_volume_set(es_handle, EXAMPLE_VOICE_VOLUME, NULL), TAG, "set es8311 volume failed");
  ESP_RETURN_ON_ERROR(es8311_microphone_config(es_handle, false), TAG, "set es8311 microphone failed");

  return ESP_OK;
}


void sdmmc_init(void)
{
      // If you want to change the pin assignment on ESP32-S3 uncomment this block and the appropriate
    // line depending if you want to use 1-bit or 4-bit line.
    // Please note that ESP32 does not allow pin change and will always fail.
    if(! SD_MMC.setPins(clk, cmd, d0)){
        Serial.println("Pin change failed!");
        return;
    }
    

  if (!SD_MMC.begin("/sdcard", true)) { // true 表示 1-bit 模式
      Serial.println("Card Mount Failed");
      return;
  }
  uint8_t cardType = SD_MMC.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD_MMC card attached");
    return;
  }

  Serial.print("SD_MMC Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);
}
void setup() {

  Serial.begin(115200);
  
  DEV_I2C_Init();
  IO_EXTENSION_Init();
  IO_EXTENSION_Output(IO_EXTENSION_IO_2, 1);
  IO_EXTENSION_Output(IO_EXTENSION_IO_6, 1);
  
  sdmmc_init();
  es8311_codec_init();

  audio.setPinout(I2S_BCK_PIN, I2S_LRCK_PIN, I2S_DOUT_PIN,I2S_MCLK_PIN);
  audio.connecttoFS(SD_MMC, "ff-16b-1c-44100hz.mp3");

  IO_EXTENSION_Output(IO_EXTENSION_IO_4, 1);
  Serial.println("init ok!");
}

void loop() {
  vTaskDelay(1);
  audio.loop();
}

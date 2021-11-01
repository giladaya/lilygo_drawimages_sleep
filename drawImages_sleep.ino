/* Simple firmware for a ESP32 displaying a static image on an EPaper Screen.

   Write an image into a header file using a 3...2...1...0 format per pixel,
   for 4 bits color (16 colors - well, greys.) MSB first.  At 80 MHz, screen
   clears execute in 1.075 seconds and images are drawn in 1.531 seconds.
*/

#include <Arduino.h>
#include "epd_driver.h"
#include "firasans.h"
#include "pic1.h"
#include "pic2.h"
#include "pic3.h"

#include "esp_adc_cal.h"
#include <SPI.h>
#include <SD.h>

#define BATT_PIN            36
#define SD_MISO             12
#define SD_MOSI             13
#define SD_SCLK             14
#define SD_CS               15

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */

uint8_t *framebuffer;
int vref = 1100;

// Track which file to draw now
RTC_DATA_ATTR int bootCount = 0;

void setup()
{
  Serial.begin(115200);

  // Init SD card
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI);
  int sds = SD.begin(SD_CS);

  // Correct the ADC reference voltage
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    Serial.printf("eFuse Vref:%u mV\n", adc_chars.vref);
    vref = adc_chars.vref;
  }

  epd_init();

  framebuffer = (uint8_t *)heap_caps_malloc(EPD_WIDTH * EPD_HEIGHT / 2, MALLOC_CAP_SPIRAM);
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  ++bootCount;
  Serial.printf("Boot # %d\n", bootCount);
  int picIdx = bootCount % 3;

  // ----------------

  // draw image to buffer
  uint32_t width;
  uint32_t height;
  const uint8_t * data;

  switch (picIdx) {
    case 0:
      width = pic1_width;
      height = pic1_height;
      data = pic1_data;
      break;
    case 1:
      width = pic2_width;
      height = pic2_height;
      data = pic2_data;
      break;
    case 2:
    default:
      width = pic3_width;
      height = pic3_height;
      data = pic3_data;
      break;
  }

  Rect_t area = {
    .x = 0,
    .y = 0,
    .width = width,
    .height =  height
  };
  epd_copy_to_framebuffer(area, (uint8_t *) data, framebuffer);

  // ----------------

  // on-screen log
  char logBuf[128];

  epd_poweron();

  uint16_t vRaw = analogRead(BATT_PIN);
  float battery_voltage = ((float)vRaw / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);

  sprintf(logBuf, "b# %d, v: %.2f, sd: %d, %d", bootCount, battery_voltage, sds, SD.cardType());
  Serial.println(logBuf);

  epd_fill_rect(100, 460, 760, 50, 0xff, framebuffer);
  int cursor_x = 110;
  int cursor_y = 500;
  writeln((GFXfont *)&FiraSans, logBuf, &cursor_x, &cursor_y, framebuffer);

  // ----------------

  // Draw to screen
  //  epd_poweron();
  epd_clear();
  epd_draw_grayscale_image(epd_full_screen(), framebuffer);
  epd_poweroff();

  // ----------------

  // Go to sleep
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Going to sleep now for " + String(TIME_TO_SLEEP) + " Seconds");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop()
{
  // empty
}

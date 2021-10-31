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

#define BATT_PIN            36

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */

uint8_t *framebuffer;

// Track which file to draw now
RTC_DATA_ATTR int bootCount = 0;

void setup()
{
  Serial.begin(115200);
  Serial.printf("Boot # %d\n", bootCount);

  epd_init();

  framebuffer = (uint8_t *)heap_caps_malloc(EPD_WIDTH * EPD_HEIGHT / 2, MALLOC_CAP_SPIRAM);
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  ++bootCount;
  uint32_t width;
  uint32_t height;
  const uint8_t * data;
  bool canDraw = true;

  switch (bootCount) {
    case 1:
      width = pic1_width;
      height = pic1_height;
      data = pic1_data;
      break;
    case 2:
      width = pic2_width;
      height = pic2_height;
      data = pic2_data;
      break;
    case 3:
      width = pic3_width;
      height = pic3_height;
      data = pic3_data;
      break;
    default:
      canDraw = false;
      break;
  }

  if (canDraw) {
    Rect_t area = {
      .x = 0,
      .y = 0,
      .width = width,
      .height =  height
    };
    epd_copy_to_framebuffer(area, (uint8_t *) data, framebuffer);
  } else {
    bootCount = 0;
  }

  // on-screen log
  char logBuf[128];
  uint16_t vRaw = analogRead(BATT_PIN);
  
  epd_fill_rect(100, 460, 760, 50, 0xff, framebuffer);
  sprintf(logBuf, "b# %d, v: %d", bootCount, vRaw);
  int cursor_x = 110;
  int cursor_y = 500;
  writeln((GFXfont *)&FiraSans, logBuf, &cursor_x, &cursor_y, framebuffer);


  epd_poweron();
  epd_clear();
  epd_draw_grayscale_image(epd_full_screen(), framebuffer);
  epd_poweroff();

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Going to sleep now for " + String(TIME_TO_SLEEP) + " Seconds");
  Serial.flush();
  esp_deep_sleep_start();
}

void update(uint32_t delay_ms)
{
  epd_poweron();
  epd_clear();
  volatile uint32_t t1 = millis();
  epd_draw_grayscale_image(epd_full_screen(), framebuffer);
  volatile uint32_t t2 = millis();
  Serial.printf("EPD draw took %dms.\n", t2 - t1);
  epd_poweroff();
  delay(delay_ms);
}

void loop()
{
  // empty
}

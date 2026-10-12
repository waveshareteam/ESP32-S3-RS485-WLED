#include <Arduino.h>
#include "driver/ledc.h"

// PWM output pin. This follows the ESP-IDF C PWM example.
#define PWM_GPIO    6
#define PWM_CH      LEDC_CHANNEL_0
#define PWM_TIMER   LEDC_TIMER_0

// 8-bit PWM duty range: 0 (off) ~ 255 (full on).
#define PWM_MAX_DUTY 255
#define PWM_FREQ_HZ  25000

// Set to 1 for breathing PWM, or 0 for fixed brightness.
#define ENABLE_BREATHING_PWM 1
#define FIXED_BRIGHTNESS    128

static void pwmInit()
{
  ledc_timer_config_t timer_conf = {};
  timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
  timer_conf.timer_num = PWM_TIMER;
  timer_conf.duty_resolution = LEDC_TIMER_8_BIT;
  timer_conf.freq_hz = PWM_FREQ_HZ;
  timer_conf.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&timer_conf);

  ledc_channel_config_t chan_conf = {};
  chan_conf.gpio_num = PWM_GPIO;
  chan_conf.speed_mode = LEDC_LOW_SPEED_MODE;
  chan_conf.channel = PWM_CH;
  chan_conf.timer_sel = PWM_TIMER;
  chan_conf.duty = 0;
  chan_conf.hpoint = 0;
  ledc_channel_config(&chan_conf);
}

static void pwmWriteDuty(uint32_t duty)
{
  if (duty > PWM_MAX_DUTY) {
    duty = PWM_MAX_DUTY;
  }

  ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CH, duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CH);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  pwmInit();
  Serial.println("PWM example started: GPIO6, 25kHz, 8-bit duty.");

#if !ENABLE_BREATHING_PWM
  pwmWriteDuty(FIXED_BRIGHTNESS);
  Serial.printf("Fixed brightness duty: %d\r\n", FIXED_BRIGHTNESS);
#endif
}

void loop()
{
#if ENABLE_BREATHING_PWM
  static int duty = 0;
  static int dir = 1;

  pwmWriteDuty(duty);

  duty += dir;
  if (duty >= PWM_MAX_DUTY) {
    duty = PWM_MAX_DUTY;
    dir = -1;
  } else if (duty <= 0) {
    duty = 0;
    dir = 1;
  }

  delay(8);
#else
  delay(1000);
#endif
}

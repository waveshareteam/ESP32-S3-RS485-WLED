#include <FastLED.h>

#define NUM_LEDS 160        
#define DATA_PIN 2       
#define BRIGHTNESS 100    
#define LED_TYPE WS2812  
#define COLOR_ORDER GRB   
#define GPIO6_CTRL_PIN   (gpio_num_t)6

CRGB leds[NUM_LEDS];
uint8_t currentMode = 0;  
unsigned long lastModeSwitch = 0;
#define MODE_SWITCH_INTERVAL 7000  // Mode switch interval (ms)
uint8_t hue = 0;              

void setup() {

   gpio_config_t io_conf = {};
  io_conf.pin_bit_mask = (1ULL << GPIO6_CTRL_PIN);
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&io_conf));

  ESP_ERROR_CHECK(gpio_set_drive_capability(GPIO6_CTRL_PIN, GPIO_DRIVE_CAP_3));

  gpio_set_level(GPIO6_CTRL_PIN, 1);
  ESP_LOGI(TAG, "GPIO6 set HIGH");

  // Initialize FastLED with LED type, data pin, color order, and number of LEDs
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  // Initialize all LEDs to red
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;
  }
  FastLED.show();
  delay(2000);
}

// Marquee effect: single LED moves
void runMarquee(CRGB color) {
  static int pos = 0;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  leds[pos] = color;
  FastLED.show();
  pos = (pos + 1) % NUM_LEDS;
  delay(200);
}

// Waterfall effect: multiple LEDs flow
void runWaterfall(CRGB color) {
  static int pos = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = (i == pos || i == (pos + 1) % NUM_LEDS) ? color : CRGB::Black;
  }
  FastLED.show();
  pos = (pos + 1) % NUM_LEDS;
  delay(150);
}

// Rainbow gradient effect: overall color cycles
void runRainbow() {
  fill_rainbow(leds, NUM_LEDS, hue, 20);
  FastLED.show();
  hue++;
  delay(50);
}

// Breathing effect: overall brightness fades
void runBreathing(CRGB color) {
  static uint8_t brightness = 0;
  static bool increasing = true;
  fill_solid(leds, NUM_LEDS, color);
  FastLED.setBrightness(brightness);
  FastLED.show();
  if (increasing) {
    brightness += 5;
    if (brightness >= 255) {
      increasing = false;
    }
  } else {
    brightness -= 5;
    if (brightness <= 0) {
      increasing = true;
    }
  }
  delay(30);
}

// Chase effect: multiple LEDs chase each other
void runChase(CRGB color) {
  static int pos = 0;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  for (int i = 0; i < 3; i++) {
    leds[(pos + i) % NUM_LEDS] = color;
  }
  FastLED.show();
  pos = (pos + 1) % NUM_LEDS;
  delay(100);
}

void loop() {
  unsigned long now = millis();
  // Switch mode periodically
  if (now - lastModeSwitch > MODE_SWITCH_INTERVAL) {
    currentMode = (currentMode + 1) % 6;
    lastModeSwitch = now;
  }

  switch (currentMode) {
    case 0:
      // Random color mode
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = random(0xffffff);
      }
      FastLED.show();
      delay(500);
      break;
    case 1:  
      runMarquee(CRGB::Green);
      break;
    case 2: 
      runWaterfall(CRGB::Blue);
      break;
    case 3:  
      runRainbow();
      break;
    case 4: 
      runBreathing(CRGB::Purple);
      break;
    case 5:  
      runChase(CRGB::Orange);
      break;
  }
}
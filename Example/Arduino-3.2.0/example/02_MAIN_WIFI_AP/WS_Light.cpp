#include "WS_Light.h"

#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/semphr.h"

static const char *TAG = "WS_Light";

static const uint32_t RMT_RESOLUTION_HZ = 10000000;
static const uint16_t LIGHT_MAX_COUNT = 2048;
static const uint16_t WS2812_RESET_US = 300;
static const gpio_num_t LIGHT_POWER_ENABLE_GPIO = GPIO_NUM_6;

static const uint16_t WS2812_T0H_NS = 350;
static const uint16_t WS2812_T0L_NS = 900;
static const uint16_t WS2812_T1H_NS = 900;
static const uint16_t WS2812_T1L_NS = 350;

static rmt_channel_handle_t txChannel = NULL;
static rmt_encoder_handle_t ledEncoder = NULL;
static SemaphoreHandle_t lightMutex = NULL;

static uint8_t lightGpio = LIGHT_DEFAULT_GPIO;
static uint16_t physicalCount = LIGHT_DEFAULT_COUNT;
static uint8_t currentR = 0;
static uint8_t currentG = 0;
static uint8_t currentB = 0;
static bool lightReady = false;
static char lightStatus[80] = "light not initialized";

static void setStatus(const char *status)
{
  strncpy(lightStatus, status, sizeof(lightStatus) - 1);
  lightStatus[sizeof(lightStatus) - 1] = '\0';
}

static bool lockLight(TickType_t waitTicks)
{
  return lightMutex == NULL || xSemaphoreTake(lightMutex, waitTicks) == pdTRUE;
}

static void unlockLight()
{
  if (lightMutex != NULL) {
    xSemaphoreGive(lightMutex);
  }
}

static bool isValidGpio(uint8_t gpio)
{
  if (!GPIO_IS_VALID_OUTPUT_GPIO((gpio_num_t)gpio)) {
    return false;
  }

  // Reserved pins: BOOT 0, LED power 6, audio 7/8/9/10/45/46,
  // I2C 14/15, RS485 17/18/21, reserved range 22-37,
  // and the console UART 43/44.
  return gpio != 0 && gpio != 6 && gpio != 7 && gpio != 8 && gpio != 9 &&
         gpio != 10 && gpio != 14 && gpio != 15 && gpio != 17 && gpio != 18 &&
         gpio != 21 && !(gpio >= 22 && gpio <= 37) && gpio != 43 &&
         gpio != 44 && gpio != 45 && gpio != 46;
}

static void enableLightPower()
{
  gpio_config_t ioConf = {};
  ioConf.pin_bit_mask = 1ULL << LIGHT_POWER_ENABLE_GPIO;
  ioConf.mode = GPIO_MODE_OUTPUT;
  ioConf.pull_up_en = GPIO_PULLUP_DISABLE;
  ioConf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  ioConf.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&ioConf);
  gpio_set_level(LIGHT_POWER_ENABLE_GPIO, 1);
}

static void deinitDriver()
{
  if (txChannel != NULL) {
    rmt_disable(txChannel);
  }
  if (ledEncoder != NULL) {
    rmt_del_encoder(ledEncoder);
    ledEncoder = NULL;
  }
  if (txChannel != NULL) {
    rmt_del_channel(txChannel);
    txChannel = NULL;
  }
  lightReady = false;
}

static bool initDriver(uint8_t gpio)
{
  gpio_config_t ioConf = {};
  ioConf.pin_bit_mask = 1ULL << gpio;
  ioConf.mode = GPIO_MODE_OUTPUT;
  ioConf.pull_up_en = GPIO_PULLUP_ENABLE;
  ioConf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  ioConf.intr_type = GPIO_INTR_DISABLE;
  if (gpio_config(&ioConf) != ESP_OK) {
    setStatus("gpio config failed");
    return false;
  }
  gpio_set_drive_capability((gpio_num_t)gpio, GPIO_DRIVE_CAP_3);

  rmt_tx_channel_config_t txConfig = {};
  txConfig.gpio_num = (gpio_num_t)gpio;
  txConfig.clk_src = RMT_CLK_SRC_DEFAULT;
  txConfig.resolution_hz = RMT_RESOLUTION_HZ;
  txConfig.mem_block_symbols = 128;
  txConfig.trans_queue_depth = 4;
  if (rmt_new_tx_channel(&txConfig, &txChannel) != ESP_OK) {
    setStatus("rmt channel failed");
    return false;
  }

  rmt_bytes_encoder_config_t encoderConfig = {};
  encoderConfig.bit0.duration0 = WS2812_T0H_NS / (1000000000 / RMT_RESOLUTION_HZ);
  encoderConfig.bit0.level0 = 1;
  encoderConfig.bit0.duration1 = WS2812_T0L_NS / (1000000000 / RMT_RESOLUTION_HZ);
  encoderConfig.bit0.level1 = 0;
  encoderConfig.bit1.duration0 = WS2812_T1H_NS / (1000000000 / RMT_RESOLUTION_HZ);
  encoderConfig.bit1.level0 = 1;
  encoderConfig.bit1.duration1 = WS2812_T1L_NS / (1000000000 / RMT_RESOLUTION_HZ);
  encoderConfig.bit1.level1 = 0;
  encoderConfig.flags.msb_first = 1;
  if (rmt_new_bytes_encoder(&encoderConfig, &ledEncoder) != ESP_OK) {
    deinitDriver();
    setStatus("rmt encoder failed");
    return false;
  }

  if (rmt_enable(txChannel) != ESP_OK) {
    deinitDriver();
    setStatus("rmt enable failed");
    return false;
  }

  lightReady = true;
  setStatus("light ready");
  ESP_LOGI(TAG, "WS2812 initialized on GPIO %u, count %u", gpio, physicalCount);
  return true;
}

static bool showLocked(uint8_t r, uint8_t g, uint8_t b)
{
  if (!lightReady || txChannel == NULL || ledEncoder == NULL) {
    return false;
  }

  size_t dataLen = (size_t)physicalCount * 3;
  uint8_t *ledData = (uint8_t *)heap_caps_malloc(dataLen, MALLOC_CAP_8BIT);
  if (ledData == NULL) {
    setStatus("light buffer alloc failed");
    return false;
  }

  for (uint16_t i = 0; i < physicalCount; i++) {
    size_t pixel = (size_t)i * 3;
    ledData[pixel + 0] = g;
    ledData[pixel + 1] = r;
    ledData[pixel + 2] = b;
  }

  rmt_transmit_config_t txConfig = {};
  txConfig.loop_count = 0;
  txConfig.flags.eot_level = 0;
  esp_err_t err = rmt_transmit(txChannel, ledEncoder, ledData, dataLen, &txConfig);
  if (err == ESP_OK) {
    err = rmt_tx_wait_all_done(txChannel, portMAX_DELAY);
  }
  free(ledData);
  esp_rom_delay_us(WS2812_RESET_US);

  if (err != ESP_OK) {
    setStatus("light transmit failed");
    return false;
  }

  currentR = r;
  currentG = g;
  currentB = b;
  setStatus("light updated");
  return true;
}

void Light_Init()
{
  if (lightMutex == NULL) {
    lightMutex = xSemaphoreCreateMutex();
  }

  if (!lockLight(portMAX_DELAY)) {
    return;
  }
  enableLightPower();
  deinitDriver();
  initDriver(lightGpio);
  showLocked(currentR, currentG, currentB);
  unlockLight();
}

bool Light_SetConfig(uint8_t gpio, uint16_t count)
{
  if (count == 0 || count > LIGHT_MAX_COUNT || !isValidGpio(gpio)) {
    setStatus("invalid gpio or count");
    return false;
  }

  if (!lockLight(pdMS_TO_TICKS(500))) {
    return false;
  }

  // Clear the strip using the old count before changing the active length.
  // WS2812 pixels beyond the new count otherwise keep their last latched color.
  const uint8_t savedR = currentR;
  const uint8_t savedG = currentG;
  const uint8_t savedB = currentB;
  if (lightReady) {
    showLocked(0, 0, 0);
  }

  deinitDriver();
  lightGpio = gpio;
  physicalCount = count;
  bool ok = initDriver(lightGpio);
  if (ok) {
    ok = showLocked(savedR, savedG, savedB);
  }
  unlockLight();
  return ok;
}

bool Light_SetColor(uint8_t r, uint8_t g, uint8_t b)
{
  if (!lockLight(pdMS_TO_TICKS(500))) {
    return false;
  }
  bool ok = showLocked(r, g, b);
  unlockLight();
  return ok;
}

bool Light_Off()
{
  return Light_SetColor(0, 0, 0);
}

String Light_GetStatusJson()
{
  String json = "{";
  json += "\"ready\":";
  json += lightReady ? "true" : "false";
  json += ",\"gpio\":";
  json += String(lightGpio);
  json += ",\"count\":";
  json += String(physicalCount);
  json += ",\"r\":";
  json += String(currentR);
  json += ",\"g\":";
  json += String(currentG);
  json += ",\"b\":";
  json += String(currentB);
  json += ",\"status\":\"";
  json += lightStatus;
  json += "\"}";
  return json;
}

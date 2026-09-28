#ifndef __SPEAKER_MICROPHONE_H
#define __SPEAKER_MICROPHONE_H

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "err_check.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_err.h"
#include "i2c.h"

#define I2S_NUM   (1)
#define I2S_SCLK  (GPIO_NUM_9)
#define I2S_MCLK  (GPIO_NUM_7)
#define I2S_LCLK  (GPIO_NUM_45)
#define I2S_DOUT  (GPIO_NUM_8)

#define I2S_GPIO_CFG       \
    {                      \
        .mclk = I2S_MCLK,  \
        .bclk = I2S_SCLK,  \
        .ws = I2S_LCLK,    \
        .dout = I2S_DOUT,  \
        .din = GPIO_NUM_NC,\
        .invert_flags = {  \
            .mclk_inv = false, \
            .bclk_inv = false, \
            .ws_inv = false,   \
        },                 \
    }

#define I2S_DUPLEX_MONO_CFG(_sample_rate)                                                            \
    {                                                                                                \
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sample_rate),                                         \
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO), \
        .gpio_cfg = I2S_GPIO_CFG,                                                                    \
    }

esp_codec_dev_handle_t speaker_init(void);

#endif

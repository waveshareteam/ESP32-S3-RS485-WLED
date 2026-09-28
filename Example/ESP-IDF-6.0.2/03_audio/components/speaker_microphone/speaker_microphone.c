#include "speaker_microphone.h"

#include <assert.h>

#include "codec_dev.h"
#include "esp_log.h"

static const char *TAG = "speaker";

static i2s_chan_handle_t i2s_tx_chan;
static const audio_codec_data_if_t *i2s_data_if;
static i2c_master_bus_handle_t i2c_handle;
static bool i2c_initialized;

static esp_err_t codec_i2c_init(void)
{
    if (i2c_initialized) {
        return ESP_OK;
    }

    i2c_handle = DEV_I2C_Get_Bus_Device();
    i2c_initialized = true;
    return ESP_OK;
}

static esp_err_t codec_audio_init(void)
{
    esp_err_t ret = ESP_OK;

    if (i2s_tx_chan) {
        return ESP_OK;
    }

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &i2s_tx_chan, NULL), TAG, "Create I2S TX channel failed");

    i2s_std_config_t std_cfg = I2S_DUPLEX_MONO_CFG(CODEC_DEFAULT_SAMPLE_RATE);
    ESP_GOTO_ON_ERROR(i2s_channel_init_std_mode(i2s_tx_chan, &std_cfg), err, TAG, "I2S TX init failed");
    ESP_GOTO_ON_ERROR(i2s_channel_enable(i2s_tx_chan), err, TAG, "I2S TX enable failed");

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_NUM,
        .rx_handle = NULL,
        .tx_handle = i2s_tx_chan,
    };
    i2s_data_if = audio_codec_new_i2s_data(&i2s_cfg);
    assert(i2s_data_if);
    return ESP_OK;

err:
    if (i2s_tx_chan) {
        i2s_del_channel(i2s_tx_chan);
        i2s_tx_chan = NULL;
    }
    return ret;
}

static esp_err_t speaker_audio_poweramp_enable(bool enable)
{
    gpio_set_direction(GPIO_NUM_46, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_46, enable ? 1 : 0);
    return ESP_OK;
}

esp_codec_dev_handle_t speaker_init(void)
{
    if (i2s_data_if == NULL) {
        USER_ERROR_CHECK_RETURN_NULL(codec_i2c_init());
        USER_ERROR_CHECK_RETURN_NULL(codec_audio_init());
    }
    assert(i2s_data_if);

    speaker_audio_poweramp_enable(true);

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = EXAMPLE_I2C_MASTER_NUM,
        .addr = ES8389_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_handle,
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    USER_NULL_CHECK(i2c_ctrl_if, NULL);

    esp_codec_dev_hw_gain_t gain = {
        .pa_voltage = 5,
        .codec_dac_voltage = 3.3,
    };
    es8389_codec_cfg_t es8389_cfg = {
        .ctrl_if = i2c_ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = GPIO_NUM_NC,
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .hw_gain = gain,
    };

    const audio_codec_if_t *es8389_dev = es8389_codec_new(&es8389_cfg);
    USER_NULL_CHECK(es8389_dev, NULL);

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = es8389_dev,
        .data_if = i2s_data_if,
    };
    return esp_codec_dev_new(&codec_dev_cfg);
}

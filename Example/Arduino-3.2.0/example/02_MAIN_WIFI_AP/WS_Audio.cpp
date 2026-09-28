#include "WS_Audio.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

extern "C" {
#include "src/i2c/i2c.h"
#include "src/speaker_microphone/codec_dev.h"
}

#include "music.h"

static const char *TAG = "WS_Audio";

static const size_t PLAY_FRAME_SIZE = 2048;
static const size_t MIC_FRAME_SIZE = 1024;
static const uint32_t RECORD_SECONDS = 5; //time
static const int32_t RECORD_PLAYBACK_GAIN = 2; //4
static const size_t RECORD_BYTES =
    CODEC_DEFAULT_SAMPLE_RATE * RECORD_SECONDS * 4 * (CODEC_DEFAULT_BIT_WIDTH / 8);

static int16_t playFrame[PLAY_FRAME_SIZE * 2];
static int16_t micFrame[MIC_FRAME_SIZE * 4];
static uint8_t *recordBuffer = NULL;
static size_t recordBytes = 0;

static TaskHandle_t audioTaskHandle = NULL;
static volatile bool audioReady = false;
static volatile bool audioBusy = false;
static volatile bool stopPlayback = false;
static volatile bool playRecordedAfterRecord = false;
static volatile bool recordAfterPlaybackStop = false;
static volatile uint8_t audioMode = 0;
static uint32_t musicIndex = 0;
static char audioStatus[96] = "audio not initialized";

enum {
  AUDIO_MODE_IDLE = 0,
  AUDIO_MODE_MUSIC = 1,
  AUDIO_MODE_RECORDING = 2,
  AUDIO_MODE_RECORD_PLAYBACK = 3
};

static void record5sTask(void *parameter);

static void setStatus(const char *status)
{
  strncpy(audioStatus, status, sizeof(audioStatus) - 1);
  audioStatus[sizeof(audioStatus) - 1] = '\0';
}

static bool startAudioTask(TaskFunction_t task, const char *name)
{
  if (!audioReady || audioBusy || audioTaskHandle != NULL) {
    return false;
  }

  audioBusy = true;
  stopPlayback = false;
  BaseType_t ok = xTaskCreatePinnedToCore(
      task,
      name,
      8192,
      NULL,
      3,
      &audioTaskHandle,
      1);

  if (ok != pdPASS) {
    audioBusy = false;
    audioTaskHandle = NULL;
    setStatus("audio task create failed");
    return false;
  }

  return true;
}

static void finishAudioTask(const char *status)
{
  setStatus(status);
  audioMode = AUDIO_MODE_IDLE;
  audioBusy = false;
  audioTaskHandle = NULL;
  vTaskDelete(NULL);
}

static int32_t sampleAbs(int16_t value)
{
  return value < 0 ? -(int32_t)value : (int32_t)value;
}

static int16_t selectRecordedSample(const int16_t *frame)
{
  int16_t sample = frame[3];
  int32_t peak = sampleAbs(sample);

  for (size_t ch = 0; ch < 4; ch++) {
    int32_t level = sampleAbs(frame[ch]);
    if (level > peak) {
      peak = level;
      sample = frame[ch];
    }
  }

  return sample;
}

static int16_t amplifyRecordedSample(int16_t sample)
{
  int32_t amplified = (int32_t)sample * RECORD_PLAYBACK_GAIN;
  if (amplified > INT16_MAX) {
    return INT16_MAX;
  }
  if (amplified < INT16_MIN) {
    return INT16_MIN;
  }
  return (int16_t)amplified;
}

static void playRecordedBuffer()
{
  if (recordBuffer == NULL || recordBytes == 0) {
    setStatus("no recording");
    return;
  }

  ESP_LOGI(TAG, "start recorded playback, bytes=%u", (unsigned)recordBytes);
  setStatus("playing recording");
  audioMode = AUDIO_MODE_RECORD_PLAYBACK;
  speaker_codec_mute_set(false);

  int16_t *samples = (int16_t *)recordBuffer;
  size_t totalSamples = recordBytes / sizeof(int16_t);
  size_t totalFrames = totalSamples / 4;
  size_t frameIndex = 0;

  while (frameIndex < totalFrames && !stopPlayback) {
    size_t frames = PLAY_FRAME_SIZE;
    if (frameIndex + frames > totalFrames) {
      frames = totalFrames - frameIndex;
    }

    for (size_t i = 0; i < frames; i++) {
      int16_t sample = amplifyRecordedSample(selectRecordedSample(&samples[(frameIndex + i) * 4]));
      playFrame[i * 2 + 0] = sample;
      playFrame[i * 2 + 1] = sample;
    }

    size_t bytesWritten = 0;
    speaker_i2s_write(playFrame, frames * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
    frameIndex += frames;
  }

  speaker_codec_mute_set(true);
}

static void playMusicTask(void *parameter)
{
  (void)parameter;
  ESP_LOGI(TAG, "start playing music.h, samples=%lu", (unsigned long)AUDIO_SAMPLES);
  setStatus("playing music");
  audioMode = AUDIO_MODE_MUSIC;
  speaker_codec_mute_set(false);

  while (musicIndex < AUDIO_SAMPLES && !stopPlayback) {
    size_t samples = PLAY_FRAME_SIZE;
    if (musicIndex + samples > AUDIO_SAMPLES) {
      samples = AUDIO_SAMPLES - musicIndex;
    }

    for (size_t i = 0; i < samples; i++) {
      int16_t sample = (int16_t)audio_data[musicIndex + i];
      playFrame[i * 2 + 0] = sample;
      playFrame[i * 2 + 1] = sample;
    }

    size_t bytesWritten = 0;
    speaker_i2s_write(playFrame, samples * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
    musicIndex += samples;
  }

  speaker_codec_mute_set(true);
  if (stopPlayback && musicIndex < AUDIO_SAMPLES) {
    if (recordAfterPlaybackStop) {
      recordAfterPlaybackStop = false;
      stopPlayback = false;
      playRecordedAfterRecord = true;
      record5sTask(NULL);
      return;
    }
    ESP_LOGI(TAG, "music paused at sample %lu", (unsigned long)musicIndex);
    finishAudioTask("music paused");
    return;
  }

  musicIndex = 0;
  ESP_LOGI(TAG, "music playback done");
  finishAudioTask("music done");
}

static void record5sTask(void *parameter)
{
  (void)parameter;
  ESP_LOGI(TAG, "start recording microphone for %lu seconds", (unsigned long)RECORD_SECONDS);
  setStatus("recording 5s");
  audioMode = AUDIO_MODE_RECORDING;
  speaker_codec_mute_set(true);

  if (recordBuffer == NULL) {
    recordBuffer = (uint8_t *)heap_caps_malloc(RECORD_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
  if (recordBuffer == NULL) {
    recordBytes = 0;
    playRecordedAfterRecord = false;
    recordAfterPlaybackStop = false;
    ESP_LOGE(TAG, "record buffer alloc failed, bytes=%u", (unsigned)RECORD_BYTES);
    finishAudioTask("record buffer alloc failed");
    return;
  }

  size_t totalBytes = 0;
  while (totalBytes < RECORD_BYTES) {
    size_t requestBytes = sizeof(micFrame);
    if (totalBytes + requestBytes > RECORD_BYTES) {
      requestBytes = RECORD_BYTES - totalBytes;
    }

    size_t bytesRead = 0;
    mic_i2s_read(micFrame, requestBytes, &bytesRead, portMAX_DELAY);
    if (bytesRead == 0) {
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }

    memcpy(recordBuffer + totalBytes, micFrame, bytesRead);
    totalBytes += bytesRead;
  }

  recordBytes = totalBytes;
  ESP_LOGI(TAG, "recording done, bytes=%u", (unsigned)recordBytes);
  if (playRecordedAfterRecord) {
    playRecordedBuffer();
    if (recordAfterPlaybackStop) {
      recordAfterPlaybackStop = false;
      stopPlayback = false;
      playRecordedAfterRecord = true;
      record5sTask(NULL);
      return;
    }
    playRecordedAfterRecord = false;
    finishAudioTask(stopPlayback ? "record playback stopped" : "record playback done");
    return;
  }
  finishAudioTask("record done");
}

void Audio_Init()
{
  DEV_I2C_Init();
  delay(100);

  if (codec_init() == ESP_OK) {
    speaker_codec_volume_set(80, NULL);
    microphone_codec_gain_set(80, NULL);
    speaker_codec_mute_set(true);
    audioReady = true;
    setStatus("audio ready");
  } else {
    audioReady = false;
    setStatus("audio init failed");
  }
}

bool Audio_PlayMusic()
{
  if (audioReady && !audioBusy && musicIndex >= AUDIO_SAMPLES) {
    musicIndex = 0;
  }
  return startAudioTask(playMusicTask, "AudioPlayMusic");
}

bool Audio_ToggleMusic()
{
  if (!audioReady) {
    return false;
  }

  if (audioBusy && audioMode == AUDIO_MODE_MUSIC) {
    stopPlayback = true;
    setStatus("pausing music");
    return true;
  }

  if (audioBusy && audioMode == AUDIO_MODE_RECORD_PLAYBACK) {
    stopPlayback = true;
    setStatus("stopping record playback");
    return true;
  }

  if (audioBusy) {
    return false;
  }

  return Audio_PlayMusic();
}

bool Audio_Record5s()
{
  playRecordedAfterRecord = false;
  recordAfterPlaybackStop = false;
  return startAudioTask(record5sTask, "AudioRecord5s");
}

bool Audio_Record5sAndPlay()
{
  if (audioBusy && (audioMode == AUDIO_MODE_MUSIC || audioMode == AUDIO_MODE_RECORD_PLAYBACK)) {
    recordAfterPlaybackStop = true;
    stopPlayback = true;
    setStatus("stopping playback for record");
    return true;
  }

  if (audioBusy) {
    return false;
  }
  recordAfterPlaybackStop = false;
  playRecordedAfterRecord = true;
  return startAudioTask(record5sTask, "AudioRecordPlay");
}

bool Audio_StopPlayback()
{
  if (audioBusy && (audioMode == AUDIO_MODE_MUSIC || audioMode == AUDIO_MODE_RECORD_PLAYBACK)) {
    stopPlayback = true;
    setStatus("stopping playback");
    return true;
  }
  return false;
}

bool Audio_IsBusy()
{
  return audioBusy;
}

String Audio_GetStatusJson()
{
  String json = "{";
  json += "\"ready\":";
  json += audioReady ? "true" : "false";
  json += ",\"busy\":";
  json += audioBusy ? "true" : "false";
  json += ",\"mode\":";
  json += String(audioMode);
  json += ",\"musicIndex\":";
  json += String(musicIndex);
  json += ",\"recordBytes\":";
  json += String(recordBytes);
  json += ",\"status\":\"";
  json += audioStatus;
  json += "\"}";
  return json;
}

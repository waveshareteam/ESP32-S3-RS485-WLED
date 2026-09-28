#pragma once

#include <Arduino.h>

void Audio_Init();
bool Audio_PlayMusic();
bool Audio_ToggleMusic();
bool Audio_Record5s();
bool Audio_Record5sAndPlay();
bool Audio_StopPlayback();
bool Audio_IsBusy();
String Audio_GetStatusJson();

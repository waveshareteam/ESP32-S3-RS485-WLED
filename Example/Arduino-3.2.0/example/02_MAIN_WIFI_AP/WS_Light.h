#pragma once

#include <Arduino.h>

#define LIGHT_DEFAULT_GPIO 2
#define LIGHT_DEFAULT_COUNT 160

void Light_Init();
bool Light_SetConfig(uint8_t gpio, uint16_t count);
bool Light_SetColor(uint8_t r, uint8_t g, uint8_t b);
bool Light_Off();
String Light_GetStatusJson();

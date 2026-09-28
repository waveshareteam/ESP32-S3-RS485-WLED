#pragma once

#include "stdio.h"
#include <stdint.h>
#include <WiFi.h>
#include <WebServer.h> 
#include <WiFiClient.h>
#include <WiFiAP.h>
#include "WS_Information.h"
#include "WS_RS485.h"
#include "WS_Light.h"
#include "WS_Audio.h"

extern char ipStr[16];

void handleGetRS485Data();
void handleRS485Send();
void handleLightSetConfig();
void handleLightSetColor();
void handleLightOff();
void handleLightStatus();
void handleAudioPlayMusic();
void handleAudioRecord5s();
void handleAudioStatus();

void WIFI_Init();
void WebTask(void *parameter);

bool ParseRS485Config(const char* Text,uint8_t* RS485_Read_Data_Type);
bool ParseRS485Data(const char* Text, RS485_Receive* RS485Data);
bool ParseRS485BaudRateConfig(const char* Text,  unsigned long * RS485_BaudRate);





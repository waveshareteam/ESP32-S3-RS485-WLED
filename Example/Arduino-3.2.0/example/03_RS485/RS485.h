#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

// RS485 UART pin definition.
#define RS485_TXD       17
#define RS485_RXD       18
#define RS485_TXDEN     21
#define RS485_BAUD_RATE 115200

void RS485_Init();
void RS485_Loop();
void RS485_Send(const uint8_t *data, size_t length);
void USB_To_RS485_Loop();

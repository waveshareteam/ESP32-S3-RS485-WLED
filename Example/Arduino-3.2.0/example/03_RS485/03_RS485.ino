#include "RS485.h"

void setup()
{
  Serial.begin(115200);
  delay(1000);

  RS485_Init();
  Serial.println("RS485 USB bridge example started.");
  Serial.println("RXD=GPIO18, TXD=GPIO17, TXDEN=GPIO21, baud=115200.");
}

void loop()
{
  USB_To_RS485_Loop();
  RS485_Loop();
  delay(10);
}

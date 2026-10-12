
#include "WS_Bluetooth.h"
#include "WS_RS485.h"
#include "WS_WIFI.h"
#include "WS_Light.h"
#include "WS_Audio.h"
#include "WS_Button.h"


/********************************************************  Initializing  ********************************************************/
void setup() { 
  RS485_Init();
  Light_Init();
  Audio_Init();
  Button_Init();
  WIFI_Init();// WIFI
  Bluetooth_Init();// Bluetooth
  
  printf("Connect to the WIFI network named \"ESP32-S3-RS485-WLED\" and access the Internet using the connected IP address!!!\r\n");
}

/**********************************************************  While  **********************************************************/
void loop() {
  Button_Loop();
  delay(10);
}

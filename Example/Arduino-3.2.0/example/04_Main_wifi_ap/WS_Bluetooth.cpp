#include "WS_Bluetooth.h"
/*
0A 0B + 数据  : RS485 发送数据（0A 0B + 多个字节数据）
0A 0D         : 获取 WIFI 的 IP
*/
BLEServer* pServer;                                                             // Used to represent a BLE server
BLECharacteristic* pTxCharacteristic;
BLECharacteristic* pRxCharacteristic;

/**********************************************************  Bluetooth   *********************************************************/

class MyServerCallbacks : public BLEServerCallbacks {                           //By overriding the onConnect() and onDisconnect() functions
    void onConnect(BLEServer* pServer) {                                        // When the Device is connected, "Device connected" is printed.
    Serial.println("Device connected"); 
  }

  void onDisconnect(BLEServer* pServer) {                                       // "Device disconnected" will be printed when the device is disconnected
    Serial.println("Device disconnected");

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();                 // Re-broadcast so that the device can query
    pAdvertising->addServiceUUID(SERVICE_UUID);                                 // Re-broadcast so that the device can query
    pAdvertising->setScanResponse(true);                                        // Re-broadcast so that the device can query
    pAdvertising->setMinPreferred(0x06);                                        // Re-broadcast so that the device can query 
    pAdvertising->setMinPreferred(0x12);                                        // Re-broadcast so that the device can query 
    BLEDevice::startAdvertising();                                              // Re-broadcast so that the device can query 
    pRxCharacteristic->notify();                                                // Re-broadcast so that the device can query  
    pAdvertising->start();                                                      // Re-broadcast so that the device can query
  }
};

uint8_t IP_Flag = 0;
class MyRXCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {                            // The onWrite function is called when the remote device sends data to your feature
    String rxValue = pCharacteristic->getValue();
    if (!rxValue.isEmpty()) {
      // The received data rxValue is processed here
      uint16_t Ble_Length = rxValue.length();
      if(Ble_Length == 2)
      {
        printf("%s\n", rxValue.c_str());                                        // Print output through the serial port
        uint8_t* valueBytes = reinterpret_cast<uint8_t*>(const_cast<char*>(rxValue.c_str())); // Convert value to uint8 t*
        if(valueBytes[0] == 0x0A && valueBytes[1] == 0x0D){                      // Instruction check correct
          printf("BLE Printf IP\r\n");
          IP_Flag = 50;
        }
      }
      else if(Ble_Length > 2)
      {
        printf("%s\n", rxValue.c_str());                                      // Print output through the serial port       
        uint8_t* valueBytes = reinterpret_cast<uint8_t*>(const_cast<char*>(rxValue.c_str())); // Convert value to uint8 t*
        if(valueBytes[0] == 0x0A && valueBytes[1] == 0x0B)                    // Instruction check correct
          SetData(valueBytes + 2, Ble_Length - 2);                            // Send data from the RS485
        else
          printf("Note : Non-instruction data was received - Bluetooth !\r\n");
      }
      else
      {
        printf("Note : Non-instruction data was received - Bluetooth !\r\n");
      }
      pRxCharacteristic->setValue("");                                           // After data is read, set it to blank for next read
    }
  }
};

void Bluetooth_SendData(char* Data) {  // Send data using Bluetooth
  if (Data != nullptr && strlen(Data) > 0) {
    if (pServer->getConnectedCount() > 0) {
      String SendValue = String(Data);  // Convert char* to String
      pTxCharacteristic->setValue(SendValue);  // Set SendValue to the eigenvalue (String type)
      pTxCharacteristic->notify();  // Sends a notification to all connected devices
    }
  }
}
void Bluetooth_Init()
{
  /*************************************************************************
  Bluetooth
  *************************************************************************/
  BLEDevice::init("ESP32-S3-RS485-WLED");                                       // Initialize Bluetooth and start broadcasting                           
  pServer = BLEDevice::createServer();                                          
  pServer->setCallbacks(new MyServerCallbacks());                               
  BLEService* pService = pServer->createService(SERVICE_UUID);                  
  pTxCharacteristic = pService->createCharacteristic(
                                    TX_CHARACTERISTIC_UUID,
                                    BLECharacteristic:: PROPERTY_READ);         // The eigenvalues are readable and can be read by remote devices
  pRxCharacteristic = pService->createCharacteristic(
                                    RX_CHARACTERISTIC_UUID,
                                    BLECharacteristic::PROPERTY_WRITE);         // The eigenvalues are writable and can be written to by remote devices
  pRxCharacteristic->setCallbacks(new MyRXCallback());

  pRxCharacteristic->setValue("Successfully Connect To ESP32-S3-RS485-WLED");      
  pService->start();   

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();                   
  pAdvertising->addServiceUUID(SERVICE_UUID);                                   
  pAdvertising->setScanResponse(true);                                          
  pAdvertising->setMinPreferred(0x06);                                          
  pAdvertising->setMinPreferred(0x12);                                          
  BLEDevice::startAdvertising();                                                
  pRxCharacteristic->notify();                                                    
  pAdvertising->start();
  printf("Now you can read it in your phone!\r\n");
  xTaskCreatePinnedToCore(
    BLETask,    
    "BLETask",   
    4096,                
    NULL,                 
    2,                   
    NULL,                 
    0                   
  );
}

void BLETask(void *parameter) {
  while(1){
    if(IP_Flag){
      Bluetooth_SendData(ipStr);
      vTaskDelay(pdMS_TO_TICKS(100));
      IP_Flag --;
    }
    else{
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
  vTaskDelete(NULL);
}

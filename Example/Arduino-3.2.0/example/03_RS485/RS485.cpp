#include "RS485.h"

static HardwareSerial rs485Serial(1);
static uint8_t rxBuffer[256];
static uint8_t usbBuffer[256];

static size_t FilterCrLf(uint8_t *data, size_t length)
{
  size_t filteredLength = 0;

  for (size_t i = 0; i < length; i++) {
    if (data[i] == '\r' || data[i] == '\n') {
      continue;
    }
    data[filteredLength++] = data[i];
  }

  return filteredLength;
}

void RS485_Init()
{
  pinMode(RS485_TXDEN, OUTPUT);
  digitalWrite(RS485_TXDEN, LOW);

  rs485Serial.begin(RS485_BAUD_RATE, SERIAL_8N1, RS485_RXD, RS485_TXD);

  if (!rs485Serial.setPins(RS485_RXD, RS485_TXD, -1, RS485_TXDEN)) {
    Serial.println("Failed to set RS485 TXDEN pin.");
  }

  if (!rs485Serial.setMode(UART_MODE_RS485_HALF_DUPLEX)) {
    Serial.println("Failed to set RS485 half-duplex mode.");
  }

  rs485Serial.setTimeout(20);
  while (rs485Serial.available()) {
    rs485Serial.read();
  }

  Serial.println("RS485 initialized.");
}

void RS485_Send(const uint8_t *data, size_t length)
{
  if (data == NULL || length == 0) {
    return;
  }

  delay(2);
  rs485Serial.write(data, length);
  rs485Serial.flush();
  delay(2);
}

void USB_To_RS485_Loop()
{
  int available = Serial.available();
  if (available <= 0) {
    return;
  }

  // Wait briefly so bytes typed from the serial tool are forwarded as one frame.
  delay(5);
  available = Serial.available();
  size_t length = (available > (int)sizeof(usbBuffer)) ? sizeof(usbBuffer) : available;
  size_t readLen = Serial.readBytes(usbBuffer, length);

  if (readLen == 0) {
    return;
  }

  // Drop CR/LF added by the serial tool, and forward the remaining bytes only.
  size_t sendLen = FilterCrLf(usbBuffer, readLen);

  if (sendLen == 0) {
    return;
  }

  RS485_Send(usbBuffer, sendLen);
  Serial.printf("USB -> RS485 %u bytes.\r\n", (unsigned int)sendLen);
}

void RS485_Loop()
{
  int available = rs485Serial.available();
  if (available <= 0) {
    return;
  }

  // Wait briefly so bytes from the same frame can arrive together.
  delay(10);
  available = rs485Serial.available();
  size_t length = (available > (int)sizeof(rxBuffer)) ? sizeof(rxBuffer) : available;
  size_t readLen = rs485Serial.readBytes(rxBuffer, length);

  if (readLen == 0) {
    return;
  }

  readLen = FilterCrLf(rxBuffer, readLen);
  if (readLen == 0) {
    return;
  }

  Serial.print("RS485 RX HEX: ");
  for (size_t i = 0; i < readLen; i++) {
    if (rxBuffer[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print(rxBuffer[i], HEX);
    Serial.print(' ');
  }
  Serial.println();

  Serial.print("RS485 RX ASCII: ");
  for (size_t i = 0; i < readLen; i++) {
    char c = (char)rxBuffer[i];
    Serial.print((c >= 32 && c <= 126) ? c : '.');
  }
  Serial.println();

  Serial.printf("RS485 received %u bytes.\r\n", (unsigned int)readLen);
}

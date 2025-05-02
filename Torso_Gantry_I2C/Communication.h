#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <Arduino.h>
#include "config.h"

// Initialize communication (Serial and Serial2)
inline void initCommunication() {
  Serial.begin(115200);
  while (!Serial);  // Wait for USB Serial connection if needed
  Serial2.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
}

// Send message to both Serial and Serial2
inline void sendMessage(const String &message) {
  Serial.println(message);
  Serial2.println(message);
}

#endif // COMMUNICATION_H

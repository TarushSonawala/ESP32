#ifndef AS5048B_H
#define AS5048B_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// Read raw angle from AS5048B sensor
inline uint16_t readAngleRaw() {
  Wire.beginTransmission(AS5048B_I2C_ADDRESS);
  Wire.write(AS5048B_REG_ANGLE_MSB);
  if (Wire.endTransmission(false) != 0) return 0xFFFF;
  if (Wire.requestFrom(AS5048B_I2C_ADDRESS, (uint8_t)2) < 2) return 0xFFFF;
  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  uint16_t rawAngle = ((msb & 0x3F) << 8) | lsb;
  return rawAngle;
}

#endif // AS5048B_H

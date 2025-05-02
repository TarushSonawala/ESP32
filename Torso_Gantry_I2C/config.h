#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ------------------------------
// Motor Control Settings
// ------------------------------
const int motorPwmPin     = 16;     // PWM output to MDDS30
const int motorDirPin     = 17;     // Direction control pin to MDDS30
const int pwmChannel      = 0;      // PWM channel
const int pwmFrequency    = 1000;   // PWM frequency (1 kHz)
const int pwmResolution   = 8;      // 8-bit (0–255)
const int motorSpeed      = 255;    // Speed for position control
const int homingSpeed     = 100;    // Speed during homing

// ------------------------------
// AS5048B I2C Settings
// ------------------------------
#define AS5048B_I2C_ADDRESS     0x40
#define AS5048B_REG_ANGLE_MSB   0xFE
#define AS5048B_REG_ANGLE_LSB   0xFF

// ------------------------------
// Limit Switch Settings
// ------------------------------
const int limitSwitch1Pin = 21; // Limit switch 1 (active LOW)
const int limitSwitch2Pin = 22; // Limit switch 2 (active LOW)

// ------------------------------
// Position & Conversion Settings
// ------------------------------
const float mmPerRevolution = 1.25;
const float maxHeightMM      = 955.0;
const float TARGET_THRESHOLD = 1.0;   // Threshold (in mm) for target reached

// ------------------------------
// UART Pins for Jetson Communication
// ------------------------------
const int UART_RX_PIN = 13;
const int UART_TX_PIN = 12;

// ------------------------------
// System State
// ------------------------------
enum SystemState {
  MOVING_TO_TARGET,
  WAIT_FOR_NEW_TARGET
};

#endif // CONFIG_H

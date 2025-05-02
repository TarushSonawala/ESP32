#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>
#include "config.h"
#include <driver/ledc.h>

// Initialize motor control
inline void initMotorControl() {
  pinMode(motorDirPin, OUTPUT);
  ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
  ledcAttachPin(motorPwmPin, pwmChannel);
}

// Move motor in a given direction at a specified speed
// direction: HIGH for DOWN, LOW for UP
inline void moveMotor(bool direction, int speed) {
  // for (int i=0; i<=100 ; i++){
  // Serial.println("Moving Motror Downmeowww");
  // Serial.println(speed);

  digitalWrite(motorDirPin, direction);
  ledcWrite(pwmChannel, speed);
  // }
}

// Stop the motor
inline void stopMotor() {
  Serial.println("Motor Stop Command Called");
  ledcWrite(pwmChannel, 0);
}

#endif // MOTOR_CONTROL_H

//#include <gpio_viewer.h>/
#include "config.h"
#include "Globals.h"
#include "MotorControl.h"
#include "AS5048B.h"
#include "Communication.h"

#include <Wire.h>
#include <Arduino.h>
#include <math.h>
//GPIOViewer gpio_viewer;/

// Global variable definitions
bool homed = false;
bool initialHomingComplete = false; // Flag set to true once the initial homing and home-position move are done.
float oldAngle = 0.0;
float totalAngle = 0.0;
long revolutionCount = 0;
unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL_MS = 1000;
SystemState systemState = MOVING_TO_TARGET;
float targetHeightMM = 500.0; // Initial target height

// Define safe working limits (always enforced after initial homing)
const float LOWER_HEIGHT_LIMIT = 100.0; // Gantry should never go below 100 mm (post-homing)
const float UPPER_HEIGHT_LIMIT = 900.0; // Gantry should never go above 900 mm

// Define a margin to safely exit the limit zone (e.g. 50 mm).
const float SAFE_MARGIN = 50.0;

// ---------------------- PID Controller Definitions ---------------------- //
// Example PID gains (tune these based on your system)
const float Kp = 2.0;
const float Ki = 0.05;
const float Kd = 1.0;

// Motor command limits (adjust as needed; here we assume PWM range 0-255)
const float MIN_MOTOR_SPEED = 50.0;
const float MAX_MOTOR_SPEED = 255.0;

// PID controller state variables
float integral = 0.0;
float lastError = 0.0;
unsigned long lastPIDTime = 0;

// ---------------------- Limit Switch ISR ---------------------- //
// Define a volatile flag to be set within the ISR.
volatile bool limitSwitchTriggered = false;

// This ISR checks if both limit switches are hit (LOW) before setting the flag.
void limitSwitchISR()
{
  if (digitalRead(limitSwitch1Pin) == LOW && digitalRead(limitSwitch2Pin) == LOW)
  {
    limitSwitchTriggered = true;
  }
}

// ---------------------- Homing Procedure ---------------------- //
// Encapsulated homing function (during homing, safe limits are not enforced)
void performHoming()
{
  // Reset key state variables to ensure a clean start
  homed = false;
  initialHomingComplete = false; // Don't enforce limits during homing.
  totalAngle = 0.0;
  revolutionCount = 0;
  oldAngle = 0.0;
  integral = 0.0;
  lastError = 0.0;
  lastPIDTime = millis();

  sendMessage("Restarting homing sequence due to limit switch trigger...");

  // Start homing: move downward until both limit switches are triggered.
  stopMotor();
  for (int i = 0; i <= 100; i++)
  {
    stopMotor();
  }
  digitalWrite(motorDirPin, HIGH); // Set direction DOWN.
  moveMotor(HIGH, homingSpeed);
  sendMessage("Homing: moving downward...");

  // Blocking loop: wait until both limit switches are triggered.
  while (digitalRead(limitSwitch1Pin) != LOW && digitalRead(limitSwitch2Pin) != LOW)
  {
    // Wait until both limit switches are hit.
  }
  Serial.println("I'm here");
  stopMotor();
  sendMessage("Homing complete. Limit switches triggered.");

  // Read angle sensor data and reset tracking.
  uint16_t raw = readAngleRaw();
  if (raw != 0xFFFF)
  {
    oldAngle = raw * (360.0f / 16384.0f);
  }
  homed = true;

  // After homing, move to a safe home position.
  targetHeightMM = 500.0;
  systemState = MOVING_TO_TARGET;
  sendMessage("Homing setup complete. Moving to home position (200mm)...");
}

void setup()
{
  initCommunication();
//  gpio_viewer.connectToWifi("UD/C", "stochlab");
//  gpio_viewer.setSamplingInter/val(25); // You can set/ the sampling interval in ms, if not set default is 100ms
  Wire.begin(18, 19);                  // I2C: SDA=19, SCL=18.
  Wire.setClock(100000);

  initMotorControl();

  // Configure limit switch pins as inputs with internal pull-ups.
  pinMode(limitSwitch1Pin, INPUT_PULLUP);
  pinMode(limitSwitch2Pin, INPUT_PULLUP);

  // Attach interrupts for each limit switch (FALLING edge triggers when activated).
  attachInterrupt(digitalPinToInterrupt(limitSwitch1Pin), limitSwitchISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(limitSwitch2Pin), limitSwitchISR, FALLING);

  sendMessage("Initializing motor control and homing...");

//  gpio_viewer.begin();/
  noInterrupts();
  interrupts();
  // stopMotor();

  // digitalWrite(motorDirPin, HIGH); // Set direction DOWN.
  // for (int i=0; i<=500;i++){
  //   Serial.println("Meowing");
  // moveMotor(HIGH, homingSpeed);
  // }
  // Perform the initial homing sequence.
  performHoming();
}

void loop()
{
  // ---------------------- Limit Switch Interrupt Check ---------------------- //
  if (limitSwitchTriggered)
  {
    noInterrupts(); // Temporarily disable interrupts for safe flag clearing.
    limitSwitchTriggered = false;
    interrupts();

    stopMotor();
    sendMessage("Both limit switches hit (interrupt detected), restarting homing sequence...");
    performHoming();
    delay(500); // Optional debounce delay.
    return;
  }

  // Ensure homing has completed before normal operation.
  if (!homed)
    return;

  // ---------------------- Angle Sensor Reading and Conversion ---------------------- //
  uint16_t angleRaw = readAngleRaw();
  if (angleRaw == 0xFFFF)
  {
    sendMessage("AS5048B read error!");
    return;
  }
  float currentAngle = angleRaw * (360.0f / 16384.0f);
  float diff = currentAngle - oldAngle;
  if (diff > 180.0f)
    diff -= 360.0f;
  else if (diff < -180.0f)
    diff += 360.0f;
  totalAngle += diff;
  revolutionCount = (long)floor(totalAngle / 360.0f);
  oldAngle = currentAngle;

  // ---------------------- Calculate Current Height ---------------------- //
  float currentHeightMM = (totalAngle / 360.0f) * mmPerRevolution;

  // ---------------------- Safe Boundary Enforcement ---------------------- //
  if (initialHomingComplete)
  {
    if (currentHeightMM < LOWER_HEIGHT_LIMIT)
    {
      targetHeightMM = LOWER_HEIGHT_LIMIT + SAFE_MARGIN;
      systemState = MOVING_TO_TARGET;
      sendMessage("Lower limit reached. Exiting limit zone: Moving upward...");
      moveMotor(LOW, motorSpeed); // Constant speed backup move.
      return;
    }
    if (currentHeightMM > UPPER_HEIGHT_LIMIT)
    {
      targetHeightMM = UPPER_HEIGHT_LIMIT - SAFE_MARGIN;
      systemState = MOVING_TO_TARGET;
      sendMessage("Upper limit reached. Exiting limit zone: Moving downward...");
      moveMotor(HIGH, motorSpeed); // Constant speed backup move.
      return;
    }
  }

  // ---------------------- PID-Controlled Motion ---------------------- //
  if (systemState == MOVING_TO_TARGET)
  {
    // Compute the error between target and current height.
    float error = targetHeightMM - currentHeightMM;

    // If the error is within acceptable threshold, stop and wait for the next command.
    if (fabs(error) < TARGET_THRESHOLD)
    {
      stopMotor();
      String msg = "Target reached: " + String(targetHeightMM, 2) +
                   " mm. Waiting for new target...";
      sendMessage(msg);

      // Mark the initial homing complete if this is the home position.
      if (!initialHomingComplete && targetHeightMM == 200.0)
      {
        initialHomingComplete = true;
        sendMessage("Initial homing complete; gantry is at a safe home position.");
      }

      // Reset PID controller variables.
      integral = 0.0;
      lastError = 0.0;
      systemState = WAIT_FOR_NEW_TARGET;
    }
    else
    {
      // Calculate time elapsed for PID update.
      unsigned long now = millis();
      float dt = (now - lastPIDTime) / 1000.0; // Convert ms to seconds.
      lastPIDTime = now;

      // Simple protection against division by zero.
      if (dt <= 0)
        dt = 0.001;

      // Update PID integral and derivative.
      integral += error * dt;
      float derivative = (error - lastError) / dt;
      lastError = error;

      // PID output for motor speed command (unsing absolute value later).
      float pidOutput = Kp * error + Ki * integral + Kd * derivative;

      // Determine direction and calculate absolute motor speed command.
      float motorSpeedCommand = fabs(pidOutput);
      if (motorSpeedCommand < MIN_MOTOR_SPEED)
        motorSpeedCommand = MIN_MOTOR_SPEED;
      if (motorSpeedCommand > MAX_MOTOR_SPEED)
        motorSpeedCommand = MAX_MOTOR_SPEED;

      // Choose motor direction based on error sign.
      // (Assuming that moveMotor(LOW, speed) drives upward and moveMotor(HIGH, speed) drives downward)
      if (error > 0)
      {
        moveMotor(LOW, motorSpeedCommand);
      }
      else
      {
        moveMotor(HIGH, motorSpeedCommand);
      }
    }
  }
  else if (systemState == WAIT_FOR_NEW_TARGET)
  {
    // Check for a new height command from Serial2 (or another controller).
    if (Serial2.available())
    {
      String input = Serial2.readStringUntil('\n');
      input.trim();
      if (input.startsWith("HEIGHT:"))
      {
        float newTarget = input.substring(7).toFloat();
        if (newTarget >= LOWER_HEIGHT_LIMIT && newTarget <= UPPER_HEIGHT_LIMIT)
        {
          targetHeightMM = newTarget;
          String msg = "Received new target height: " + String(targetHeightMM, 2) +
                       " mm. Moving...";
          sendMessage(msg);
          systemState = MOVING_TO_TARGET;
          // Reset PID variables before starting motion.
          integral = 0.0;
          lastError = 0.0;
          lastPIDTime = millis();
        }
        else
        {
          sendMessage("Invalid height received. Please send a value between 100mm and 900mm.");
        }
      }
      else
      {
        sendMessage("Invalid format. Expected HEIGHT:<value>");
      }
    }
  }

  // ---------------------- Debug Output ---------------------- //
  unsigned long currentMillis = millis();
  if (currentMillis - lastPrintTime >= PRINT_INTERVAL_MS)
  {
    lastPrintTime = currentMillis;
    String debugMsg = "Angle = " + String(currentAngle, 2) + " deg, Height = " +
                      String(currentHeightMM, 2) + " mm, Revolutions = " +
                      String(revolutionCount);
    Serial.println(debugMsg);
    Serial2.println(debugMsg);
  }
}

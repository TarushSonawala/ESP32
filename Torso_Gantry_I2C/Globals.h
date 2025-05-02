#ifndef GLOBALS_H
#define GLOBALS_H

#include "config.h"

// Global variable declarations
extern bool homed;
extern float oldAngle;
extern float totalAngle;
extern long revolutionCount;
extern unsigned long lastPrintTime;
extern const unsigned long PRINT_INTERVAL_MS;
extern SystemState systemState;
extern float targetHeightMM;

#endif // GLOBALS_H

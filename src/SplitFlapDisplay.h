#pragma once

#include "JsonSettings.h"
#include "SplitFlapModule.h"

#include <Arduino.h>

#define MAX_MODULES 8 // for memory allocation, update if more modules
#define MAX_RPM 15.0f

// Timing constants for motor control
#define HALL_EFFECT_CHECK_INTERVAL_US  (20 * 1000)  // 20ms minimum to avoid sensor bouncing
#define MOTOR_START_STOP_DELAY_MS      200          // Time for motor to align to magnetic field
#define WATCHDOG_FEED_INTERVAL_MS      100          // Feed watchdog every 100ms during operations

class SplitFlapMqtt;

class SplitFlapDisplay {
  public:
    SplitFlapDisplay(JsonSettings &settings);

    void init();
    void updateOffsets();  // Update offsets without full reinit
    void writeString(
        String inputString, float speed = MAX_RPM,
        bool centering = true
    );                                     // Move all modules at once to show a specific string
    void writeChar(char inputChar,
                   float speed = MAX_RPM); // sets all modules to a single char
    void moveTo(int targetPositions[], float speed = MAX_RPM, bool releaseMotors = true, bool isHoming = false);
    void home(float speed = MAX_RPM);      // move home
    void homeToString(
        String homeString, float speed = MAX_RPM,
        bool centering = true
    );                                      // moves home and then writes a string
    void homeToChar(char homeChar,
                    float speed = MAX_RPM); // moves home and then sets all modules to a char
    void testAll();
    void testCount();
    void testRandom(float speed = MAX_RPM);
    void testModule(int moduleIndex, float speed = MAX_RPM); // Test single module: A -> 0 -> blank
    int getNumModules() { return numModules; }
    int getCharsetSize() const { return charSetSize; }
    void setMqtt(SplitFlapMqtt *mqttHandler);
    SplitFlapModule* getModules() { return modules; }       // Get access to modules array for testing

  private:
    JsonSettings &settings;

    bool checkAllFalse(bool array[], int size);
    void stopMotors();
    void startMotors();
    void performHomingSequence(float speed);  // Shared homing logic

    int numModules;
    uint8_t moduleAddresses[MAX_MODULES];
    SplitFlapModule modules[MAX_MODULES];
    int moduleOffsets[MAX_MODULES];
    int displayOffset;

    float maxVel;       // Max Velocity In RPM
    int charSetSize;    // 37 for standard, 48 for extended
    int stepsPerRot;    // number of motor steps per full rotation of character
                        // drum
    int magnetPosition; // position of drum wheel when magnet is detected
    int SDAPin;         // SDA pin
    int SCLPin;         // SCL pin

    SplitFlapMqtt *mqtt = nullptr;
};

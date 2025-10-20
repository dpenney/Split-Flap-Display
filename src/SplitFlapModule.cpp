#include "SplitFlapModule.h"

// Array of characters, in order, the first item is located on the magnet on the
// character drum
const char SplitFlapModule::StandardChars[37] = {' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
                                                 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
                                                 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

const char SplitFlapModule::ExtendedChars[48] = {
    ' ', 'A', 'B', 'C', 'D', 'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U',  'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4',
    '5', '6', '7', '8', '9', '\'', ':', '?', '!', '.', '-', '/', '$', '@', '#', '%',
};

bool hasErrored = false;

// Default Constructor
SplitFlapModule::SplitFlapModule()
    : address(0), position(0), stepNumber(0), stepsPerRot(0), chars(StandardChars), numChars(37), charSetSize(37) {
    baseMagnetPosition = 710;
    magnetPosition = 710;
}

// Constructor implementation
SplitFlapModule::SplitFlapModule(
    uint8_t I2Caddress, int stepsPerFullRotation, int stepOffset, int magnetPos, int charsetSize
)
    : address(I2Caddress), position(0), stepNumber(0), stepsPerRot(stepsPerFullRotation), charSetSize(charsetSize) {
    baseMagnetPosition = magnetPos;
    magnetPosition = magnetPos + stepOffset;

    chars = (charsetSize == 48) ? ExtendedChars : StandardChars;
    numChars = (charsetSize == 48) ? 48 : 37;
}

void SplitFlapModule::updateOffset(int newOffset) {
    magnetPosition = baseMagnetPosition + newOffset;
}

void SplitFlapModule::writeIO(uint16_t data) {
    Wire.beginTransmission(address);
    Wire.write(data & 0xFF);        // Send lower byte
    Wire.write((data >> 8) & 0xFF); // Send upper byte

    byte error = Wire.endTransmission();

    if (error > 0) {
        consecutiveErrors++;
        if (!hasErrored || consecutiveErrors == 1) {
            hasErrored = true;
            Serial.print("Error writing data to module ");
            Serial.print(address);
            Serial.print(", error code: ");
            Serial.println(error);
            // Error codes:
            // 0 = success
            // 1 = data too long to fit in transmit buffer
            // 2 = received NACK on transmit of address
            // 3 = received NACK on transmit of data
            // 4 = other error
        }

        // Attempt recovery after multiple errors
        if (consecutiveErrors >= 3) {
            Serial.print("Module ");
            Serial.print(address);
            Serial.println(" has persistent I2C errors, attempting recovery...");
            delay(10);
            consecutiveErrors = 0; // Reset counter after recovery attempt
        }
    } else {
        // Communication successful - reset error tracking
        if (consecutiveErrors > 0) {
            Serial.print("Module ");
            Serial.print(address);
            Serial.println(" communication recovered");
            consecutiveErrors = 0;
            hasErrored = false;
        }
    }
}

// Init Module, Setup IO Board
void SplitFlapModule::init() {
    float stepSize = (float) stepsPerRot / (float) numChars;
    float currentPosition = 0;
    for (int i = 0; i < numChars; i++) {
        charPositions[i] = (int) currentPosition;
        currentPosition += stepSize;
    }

    uint16_t initState = 0b1111111111100001; // Pin 15 (17) as INPUT, Pins 1-4 as OUTPUT
    writeIO(initState);

    stop();                                  // Write all motor coil inputs LOW

    int initDelay = 100;

    delay(initDelay);
    step();
    delay(initDelay);
    step();
    delay(initDelay);
    step();
    delay(initDelay);
    step();
    delay(initDelay);

    stop();
}

int SplitFlapModule::getCharPosition(char inputChar) {
    inputChar = toupper(inputChar);
    for (int i = 0; i < charSetSize; i++) {
        if (chars[i] == inputChar) {
            return charPositions[i];
        }
    }
    return 0; // Character not found, return blank
}

void SplitFlapModule::stop() {
    uint16_t stepState = 0b1111111111100001;
    writeIO(stepState);
}

void SplitFlapModule::start() {
    stepNumber = (stepNumber + 3) % 4; // effectively take one off stepNumber
    step(false);                       // write the "previous" step high again, in case turned off
}

void SplitFlapModule::step(bool updatePosition) {
    uint16_t stepState;
    switch (stepNumber) {
        case 0:
            stepState = 0b1111111111100111;
            writeIO(stepState);
            break;
        case 1:
            stepState = 0b1111111111110011;
            writeIO(stepState);
            break;
        case 2:
            stepState = 0b1111111111111001;
            writeIO(stepState);
            break;
        case 3:
            stepState = 0b1111111111101101;
            writeIO(stepState);
            break;
    }
    if (updatePosition) {
        position = (position + 1) % stepsPerRot;
        stepNumber = (stepNumber + 1) % 4;
    }
}

bool SplitFlapModule::readHallEffectSensor() {
    if (hasErrored) {
        return false;
    }

    uint8_t requestBytes = 2;
    Wire.requestFrom(address, requestBytes);
    // Make sure the data is available
    if (Wire.available() == 2) {
        uint16_t inputState = 0;

        // Read the two bytes and combine them into a 16-bit value
        inputState = Wire.read();             // Read the lower byte
        inputState |= (Wire.read() << 8);     // Read the upper byte and shift it left

        return (inputState & (1 << 15)) != 0; // If bit is 15, return HIGH, else LOW
    }
    return false;
}

bool SplitFlapModule::testI2CConnectivity() {
    // Try to communicate with the module by requesting 2 bytes
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    // error codes:
    // 0: success
    // 1: data too long to fit in transmit buffer
    // 2: received NACK on transmit of address
    // 3: received NACK on transmit of data
    // 4: other error
    // 5: timeout

    if (error == 0) {
        // Module responded, try reading some data to verify full communication
        Wire.requestFrom(address, (uint8_t)2);
        if (Wire.available() == 2) {
            Wire.read(); // Read and discard
            Wire.read(); // Read and discard
            return true;
        }
    }

    return false;
}

void SplitFlapModule::wakeUp() {
    // Gentle wake-up sequence before any movement
    // This helps overcome static friction and ensures coils are properly energized

    // Step 1: Gradually energize coils with micro-steps
    // This helps overcome stiction without jerking the mechanism
    for (int i = 0; i < 4; i++) {
        step(false); // Step without updating position
        delay(50);   // Longer delay for gentle energizing
        yield();     // Allow other tasks to run
    }

    // Step 2: Small oscillation to break static friction
    // Move forward slightly then back to original position
    int originalStepNumber = stepNumber;
    for (int i = 0; i < 2; i++) {
        step(false);
        delay(30);
        yield();
    }

    // Return to original step position
    for (int i = 0; i < 2; i++) {
        stepNumber = (stepNumber + 3) % 4; // Step backwards
        step(false);
        delay(30);
        yield();
    }

    stepNumber = originalStepNumber;

    // Step 3: Full power holding
    // Ensure current coil is at full strength
    step(false);
    delay(20);
}

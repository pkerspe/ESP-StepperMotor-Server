
#ifndef ESPStepperMotorServer_PositionSwitch_h
#define ESPStepperMotorServer_PositionSwitch_h

#include <Arduino.h>

typedef struct positionSwitch
{
    byte stepperIndex;
    byte ioPinNumber = 255;
    byte switchType;
    String positionName;
    long switchPosition;
} positionSwitch;

#endif
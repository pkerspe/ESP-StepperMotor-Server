//      *********************************************************************
//      *                                                                   *
//      *  ESP8266 and ESP32 Stepper Motor Server  - Stepper Config class   *
//      * a wrapper class to decouple the FlexyStepper class a bit better   *
//      *            Copyright (c) Paul Kerspe, 2019                        *
//      *                                                                   *
//      *********************************************************************
//
// MIT License
//
// Copyright (c) 2019 Paul Kerspe
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is furnished
// to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "ESPStepperMotorServer_StepperConfiguration.h"

//
// constructor for the stepper wrapper class
//
ESPStepperMotorServer_StepperConfiguration::ESPStepperMotorServer_StepperConfiguration(byte stepIoPin, byte directionIoPin)
{
    this->_stepIoPin = stepIoPin;
    this->_directionIoPin = directionIoPin;
    this->_flexyStepper = new FlexyStepper();
    this->_flexyStepper->connectToPins(this->_stepIoPin, this->_directionIoPin);
}

// ---------------------------------------------------------------------------------
//                                  Getters / Setters
// ---------------------------------------------------------------------------------
FlexyStepper *ESPStepperMotorServer_StepperConfiguration::getFlexyStepper()
{
    return this->_flexyStepper;
}

void ESPStepperMotorServer_StepperConfiguration::setId(byte id)
{
    this->_stepperIndex = id;
}

byte ESPStepperMotorServer_StepperConfiguration::getId()
{
    return this->_stepperIndex;
}

String ESPStepperMotorServer_StepperConfiguration::getDisplayName()
{
    return this->_displayName;
}
void ESPStepperMotorServer_StepperConfiguration::setDisplayName(String displayName)
{
    if (displayName.length() > ESPSMS_Stepper_DisplayName_MaxLength)
    {
        char logString[160];
        sprintf(logString, "ESPStepperMotorServer_StepperConfiguration::setDisplayName: The display name for stepper with id %i is to long. Max length is %i characters. Name will be trimmed", this->getId(), ESPSMS_Stepper_DisplayName_MaxLength);
        ESPStepperMotorServer_Logger::logWarning(logString);
        this->_displayName = displayName.substring(0, ESPSMS_Stepper_DisplayName_MaxLength);
    }
    else
    {
        this->_displayName = displayName;
    }
}

byte ESPStepperMotorServer_StepperConfiguration::getStepIoPin()
{
    return this->_stepIoPin;
}
byte ESPStepperMotorServer_StepperConfiguration::getDirectionIoPin()
{
    return this->_directionIoPin;
}

void ESPStepperMotorServer_StepperConfiguration::setStepsPerRev(unsigned int stepsPerRev){
    this->stepsPerRev = stepsPerRev;
}

unsigned int ESPStepperMotorServer_StepperConfiguration::getStepsPerRev(){
    return this->stepsPerRev;
}

void ESPStepperMotorServer_StepperConfiguration::setMicrostepsPerStep(unsigned int microstepsPerStep){
    //check for power of two value, since others are not allowed in micro step sizes
    if(microstepsPerStep && !(microstepsPerStep & (microstepsPerStep-1)) == 0){
        this->microsteppingDivisor = microstepsPerStep;
    } else {
    ESPStepperMotorServer_Logger::logWarning("ESPStepperMotorServer_StepperConfiguration::setMicrostepsPerStep: Invalid microstepping value given. Only values which are power of two are allowed");
    }
}

unsigned int ESPStepperMotorServer_StepperConfiguration::getMicrostepsPerStep(){
    return this->microsteppingDivisor;
}

void ESPStepperMotorServer_StepperConfiguration::setRpmLimit(unsigned int rpmLimit){
if(rpmLimit > ESPSMS_MAX_UPPER_RPM_LMIT){
    char logString[170];
    sprintf(logString, "ESPStepperMotorServer_StepperConfiguration::setRpmLimit: The given rpm limit value %i exceeds the allowed maximum rpm limit of %i, will set to %i", rpmLimit, ESPSMS_MAX_UPPER_RPM_LMIT, ESPSMS_MAX_UPPER_RPM_LMIT);
    ESPStepperMotorServer_Logger::logWarning(logString);
    this->rpmLimit = ESPSMS_MAX_UPPER_RPM_LMIT;
} else {
    this->rpmLimit = rpmLimit;
}
}
  
  unsigned int ESPStepperMotorServer_StepperConfiguration::getRpmLimit(){
    return this->rpmLimit;
  }
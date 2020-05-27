//      *******************************************************************
//      *                                                                 *
//      * ESP8266 and ESP32 Stepper Motor Server  - Position Switch class *
//      *            Copyright (c) Paul Kerspe, 2019                      *
//      *                                                                 *
//      *******************************************************************
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
#include <ESPStepperMotorServer_PositionSwitch.h>

byte _stepperIndex;
byte _ioPinNumber = 255;
byte _switchType; //bit mask for active state and the general type of switch
String _positionName = "";
long _switchPosition;
ESPStepperMotorServer_Logger _logger = ESPStepperMotorServer_Logger((String) "ESPStepperMotorServer_PositionSwitch");

ESPStepperMotorServer_PositionSwitch::ESPStepperMotorServer_PositionSwitch()
{
}

ESPStepperMotorServer_PositionSwitch::ESPStepperMotorServer_PositionSwitch(byte ioPin, byte stepperIndex, byte switchType, String name, long switchPosition)
{
    this->_ioPinNumber = ioPin;
    this->_stepperIndex = stepperIndex;
    this->_switchType = switchType; //this is a bit mask representing the active state (bit 1 and 2) and the general type (homing/limit/position or emergency stop switch) in one byte
    this->_positionName = name;
    this->_switchPosition = switchPosition;
}

void ESPStepperMotorServer_PositionSwitch::setId(byte id)
{
    this->_switchIndex = id;
}

byte ESPStepperMotorServer_PositionSwitch::getId()
{
    return this->_switchIndex;
}

byte ESPStepperMotorServer_PositionSwitch::getStepperIndex(void)
{
    return this->_stepperIndex;
}

byte ESPStepperMotorServer_PositionSwitch::getIoPinNumber(void)
{
    return this->_ioPinNumber;
}

byte ESPStepperMotorServer_PositionSwitch::getSwitchType(void)
{
    return this->_switchType;
}

String ESPStepperMotorServer_PositionSwitch::getPositionName(void)
{
    return this->_positionName;
}
void ESPStepperMotorServer_PositionSwitch::setPositionName(String name)
{
    this->_positionName = name;
}

long ESPStepperMotorServer_PositionSwitch::getSwitchPosition(void)
{
    return this->_switchPosition;
}
void ESPStepperMotorServer_PositionSwitch::setSwitchPosition(long position)
{
    this->_switchPosition = position;
}

bool ESPStepperMotorServer_PositionSwitch::isActiveHigh()
{
    return this->isTypeBitSet(SWITCHTYPE_STATE_ACTIVE_HIGH_BIT);
}

bool ESPStepperMotorServer_PositionSwitch::isTypeBitSet(byte bitToCheck)
{
    return this->_switchType & (1 << (bitToCheck - 1));
}
//      *******************************************************************
//      *                                                                 *
//      *  ESP8266 and ESP32 Stepper Motor Server  - Stepper class        *
//      * a wrapper class to decouple the FlexyStepper class a bit better *
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

#include "ESPStepperMotorServer_Stepper.h"
// ---------------------------------------------------------------------------------
//                                  Getters / Setters
// ---------------------------------------------------------------------------------
//
// constructor for the stepper wrapper class
//
ESPStepperMotorServer_Stepper::ESPStepperMotorServer_Stepper(FlexyStepper *flexyStepper)
{
    this->_flexyStepper = flexyStepper;
}

FlexyStepper *ESPStepperMotorServer_Stepper::getFlexyStepper() {
    return this->_flexyStepper;
}

void ESPStepperMotorServer_Stepper::setId(byte id) {
    this->_stepperIndex = id;
}

byte ESPStepperMotorServer_Stepper::getId() {
    return this->_stepperIndex;
}
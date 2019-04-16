
//      ************************************************************
//      *                                                          *
//      *     Header file for ESPStepperMotorServer_Stepper.cpp    *
//      *                                                          *
//      *              Copyright (c) Paul Kerspe, 2019             *
//      *                                                          *
//      ************************************************************

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

#ifndef ESPStepperMotorServer_Stepper_h
#define ESPStepperMotorServer_Stepper_h

#include <ESPStepperMotorServer_Logger.h>
#include <FlexyStepper.h>

#define ESPStepperMotorServer_Stepper_DisplayName_MaxLength 20

class ESPStepperMotorServer_Stepper
{
public:
  ESPStepperMotorServer_Stepper(byte stepIoPin, byte directionIoPin);
  FlexyStepper *getFlexyStepper();
  void setId(byte id);
  byte getId();
  String getDisplayName();
  void setDisplayName(String displayName);
  byte getStepIoPin();
  byte getDirectionIoPin();

  const static byte ESPServerStepperUnsetIoPinNumber = 255;

private:
  //
  // private member variables
  //
  FlexyStepper *_flexyStepper;
  byte _stepperIndex = 0;
  String _displayName;
  byte _stepIoPin = ESPServerStepperUnsetIoPinNumber;
  byte _directionIoPin = ESPServerStepperUnsetIoPinNumber;
};
// ------------------------------------ End ---------------------------------
#endif
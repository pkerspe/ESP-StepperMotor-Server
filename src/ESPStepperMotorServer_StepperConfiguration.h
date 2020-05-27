//      *************************************************************************
//      *                                                                       *
//      *     Header file for ESPStepperMotorServer_StepperConfiguration.cpp    *
//      *                                                                       *
//      *              Copyright (c) Paul Kerspe, 2019                          *
//      *                                                                       *
//      *************************************************************************

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

#ifndef ESPStepperMotorServer_StepperConfiguration_h
#define ESPStepperMotorServer_StepperConfiguration_h

#include <ESPStepperMotorServer_Logger.h>
#include <FlexyStepper.h>

#define ESPSMS_MICROSTEPS_OFF 1
#define ESPSMS_MICROSTEPS_2 2
#define ESPSMS_MICROSTEPS_4 4
#define ESPSMS_MICROSTEPS_8 8
#define ESPSMS_MICROSTEPS_16 16
#define ESPSMS_MICROSTEPS_32 32
#define ESPSMS_MICROSTEPS_128 128
#define ESPSMS_MICROSTEPS_256 256

#define ESPSMS_MAX_UPPER_RPM_LMIT 3000

#define ESPSMS_Stepper_DisplayName_MaxLength 20

//size calculated using https://arduinojson.org/v6/assistant/
#define RESERVED_JSON_SIZE_ESPStepperMotorServer_StepperConfiguration 210

class ESPStepperMotorServer_StepperConfiguration
{
public:
  ESPStepperMotorServer_StepperConfiguration(byte stepIoPin, byte directionIoPin);
  ESPStepperMotorServer_StepperConfiguration(byte stepIoPin, byte directionIoPin, String displayName, unsigned int stepsPerRev, unsigned int microsteppingDivisor, unsigned int rpmLimit);

  FlexyStepper *getFlexyStepper();

  /**
   * Internally used setter to set the id of this stepper motor.
   * Only use this if you know what you are doing
   */
  void setId(byte id);
  /**
   * get the internal id of this stepper motor configuration within the stepper server
   */
  byte getId();

  /**
   * Set the display name of the stepper motor to be shown in the user intefaces
   */
  void setDisplayName(String displayName);
  /**
   * Get the currently configured display name for the stepper motor
   */
  String getDisplayName();

  /**
   * Get the currently configured IO pin that is used to send step pulses to the stepper driver
   */
  byte getStepIoPin();

  /**
   * Get the currently configured IO pin that is used to send the direction signale to the stepper driver
   */
  byte getDirectionIoPin();

  /**
   * Set the number of full steps the stepper motor itself needs to perform for a full revolution.
   * Most stepper motors perform 1.8 degree turn per step, thus resulting in 200 full steps per revolution.
   * Other somewhat common values are 3.6 degreee (100 steps/rev), 3.75 degree (96 steps/rev) and 7.5 degree (48 steps/rev) per full step.
   * Geared stepper motors may have much smaller values, resutling in a much higher steps/rev value. See the datasheet of your stepper motor for the correct value.
   * The default value is 200 steps/rev since this is the most common value.
   */
  void setStepsPerRev(unsigned int);
  /**
   * Get the currently configured steps/rev value for this steppe motor
   * The default value is 200 steps/rev
   **/
  unsigned int getStepsPerRev();

  /**
   * Set the number of microsteps you configured in the stepper driver (usually one with DIP switches on the driver board) for this stepper motor.
   * Common values are 1 (no micro stepping), 2 (half step), 4, 8, 16, 32, 64, 128 and sometimes 256 microsteps per step.
   * This setting is needed to calculate the proper amount of pulses that need to be send to the stepper driver.
   * If this value does not match the configured micro step setting on your driver board, 
   * the number of pulses need to travel a certain distance in mm or to perform a certain amount of revolutions with the stepper motor, will not be correct.
   * The default value is 1 (ESPSMS_MICROSTEPS_OFF).
   * Allowed values: ESPSMS_MICROSTEPS_OFF (full stepping), ESPSMS_MICROSTEPS_2 (half stepping), ESPSMS_MICROSTEPS_4, ESPSMS_MICROSTEPS_8, ESPSMS_MICROSTEPS_16, ESPSMS_MICROSTEPS_32, ESPSMS_MICROSTEPS_64, ESPSMS_MICROSTEPS_128, ESPSMS_MICROSTEPS_256
   */
  void setMicrostepsPerStep(unsigned int);
  /**
   * Get the currently configured number of microsteps per step for this stepper motor.
   * The default value is 1 (ESPSMS_MICROSTEPS_OFF)
   **/
  unsigned int getMicrostepsPerStep();

  /**
   * Set the maximum revolutions per minute for this stepper.
   * This limit will only be used to limit the allowed values in the rest api enpoints / user interfaces and to calculate the maximum step pulse frequncy needed.
   * If the step pulse frequnency is higher than the one your motor can handle, you might lose steps or the motor might stall.
   * See your stepper motors datasheet for the torque curve an chose the limit that fits your needs (some datasheets specify the PPS (=Pulses per second) rather then revs/minutem, so make sure you get the right unit when settings this value).
   * The default setting is 1200 revs/minute, which might be already to high for some steppers (especially geared ones or steppers driven with a low voltage)
   **/
  void setRpmLimit(unsigned int);
  /**
   * Get the currently configured Revolutions per Minute limit for this stepper. 
   * Default is 1200 revs/minute.
   */
  unsigned int getRpmLimit();

  const static byte ESPServerStepperUnsetIoPinNumber = 255;

private:
  //
  // private member variables
  //
  FlexyStepper *_flexyStepper;
  String _displayName;
  byte _stepperIndex = 0;
  byte _stepIoPin = ESPServerStepperUnsetIoPinNumber;
  byte _directionIoPin = ESPServerStepperUnsetIoPinNumber;
  unsigned int _stepsPerRev = 200;
  unsigned int _microsteppingDivisor = ESPSMS_MICROSTEPS_OFF;
  unsigned int _rpmLimit = 1200;
};
// ------------------------------------ End ---------------------------------
#endif
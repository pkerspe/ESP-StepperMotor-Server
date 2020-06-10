//      ******************************************************************
//      *                                                                *
//      *    Header file for ESPStepperMotorServer_RotaryEncoder.cpp     *
//      *                                                                *
//      *               Copyright (c) Paul Kerspe, 2019                  *
//      *                                                                *
//      ******************************************************************
//
// This is a class to support rotary encoders a input controllers for the ESPStepperMotorServer
//
// It is based on the work Ben Buxton's Rotary Encodr Library, which was licensed under the following conditions:
// Copyright 2011 Ben Buxton.Licenced under the GNU GPL Version 3. Contact : bb @cactii.net
// Licenced under the GNU GPL Version 3. Contact: bb@cactii.net
// https://github.com/buxtronix/arduino/tree/master/libraries/Rotary
//
// I included the sources here to reduce the complexity of setting up the required libraries that need to be installed on top of the ESPSMS Library
// by the user in the Arduino UI, since it can not currently deal with automatic dependency management like platformIO.
// Ben's code is beautiful in its simplicy and therefore does not create a noticable overhead in the code size of the ESPStepperMotorServer.
//
//
// ESPStepperMotorServer is licensed under the following conditions:
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

#ifndef ESPStepperMotorServer_RotaryEncoder_h
#define ESPStepperMotorServer_RotaryEncoder_h

#include "Arduino.h"

// Enable this to emit codes twice per step.
//#define HALF_STEP

// Enable weak pullups
#define ENABLE_PULLUPS

// Values returned by 'process'
// No complete step yet.
#define DIR_NONE 0x0
// Clockwise step.
#define DIR_CW 0x10
// Anti-clockwise step.
#define DIR_CCW 0x20

//size calculated using https://arduinojson.org/v6/assistant/
#define RESERVED_JSON_SIZE_ESPStepperMotorServer_RotaryEncoder 170

class ESPStepperMotorServer_RotaryEncoder
{
   friend class ESPStepperMotorServer;

public:
   /**
     * Constructor for the rotary encoder entity.
     * The arguments define the PGIO Pins to be used to connect the Rotary encoder to
     * The displayName defines the human readable name for thi encoder in the User Interface and logs
     */
   ESPStepperMotorServer_RotaryEncoder(char pinA, char pinB, String displayName, int stepMultiplier, byte stepperIndex);

   /**
   * setter to set the id of this encoder.
   * Only use this if you know what you are doing
   */
   void setId(byte id);
   /**
    * get the id of the rotary encoder
    */
   byte getId();
   /**
     * process the input states of the io pins to determine the current rotary encoder step status
     */
   unsigned char process();
   /**
     * return the configured GPIO pin number that is connected to pin A of the rotary encoder
     */
   unsigned char getPinAIOPin();
   /**
     * return the configured GPIO pin number that is connected to pin B of the rotary encoder
     */
   unsigned char getPinBIOPin();
   /**
     * get the configured display name of the rotary encoder    
     */
   const String getDisplayName();
   /**
     * set the stepper motor id that should be linked to this rotary encoder
     */
   void setStepperIndex(byte stepperMotorIndex);
   /**
     * get the configured id of the stepper motor that is linked to this rotary encoder
     */
   byte getStepperIndex();
   /**
   * set a multiplication factor used to calculate the ammount of pulses send to the stepper motor for one step of the rotary encoder.
   * Default is factor of 1, so if one step in the rotatry encoder will be convertd into on puls to the steppr motor driver.
   * If microstpping is configured in the stepper driver board, one pulse will be one microstep, so it might be needed to st this multiplier accordingly to the microstepp setting of the stepper drivr board.     * e.g. if you configured 32 microsteps in your stepper driver board and you want the stepper motor to perform one full step per rotary encoder step, you need to set this mulitplier to 32
   */
   void setStepMultiplier(unsigned int stepMultiplier);
   /**
   * get the configured step multiplier value for this rotary encoder 
   */
   unsigned int getStepMultiplier(void);

private:
   unsigned char _state;
   unsigned char _pinA;
   unsigned char _pinB;
   unsigned char _encoderIndex;
   byte _stepperIndex;
   String _displayName;
   // step multiplier is used to define how many pulses should be sen to the stepper for one step from the rotary encoder
   unsigned int _stepMultiplier;
};

#endif

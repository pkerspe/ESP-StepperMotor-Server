//      ******************************************************************
//     //                                                               *
//     //   Header file for ESPStepperMotorServer_RotaryEncoder.cpp     *
//     //                                                               *
//     //              Copyright (c) Paul Kerspe, 2019                  *
//     //                                                               *
//      ******************************************************************
//
// This is a class to support rotary encoders a input controllers for the ESPStepperMotorServer
//
// It is based on the work Ben Buxton's Rotary Encodr Library, which was licensed under the following conditions:
// Copyright 2011 Ben Buxton.Licenced under the GNU GPL Version 3. Contact : bb @cactii.net
// Licenced under the GNU GPL Version 3. Contact: bb@cactii.net
// https://github.com/buxtronix/arduino/tree/master/libraries/Rotary
//
// BEGIN OF BENS COMMENT
// A typical mechanical rotary encoder emits a two bit gray code
// on 3 output pins. Every step in the output (often accompanied
// by a physical 'click') generates a specific sequence of output
// codes on the pins.
// There are 3 pins used for the rotary encoding - one common and
// two 'bit' pins.
// The following is the typical sequence of code on the output when
// moving from one step to the next:
//  Position   Bit1   Bit2
//  ----------------------
//    Step1     0      0
//     1/4      1      0
//     1/2      1      1
//     3/4      0      1
//    Step2     0      0
//
// From this table, we can see that when moving from one 'click' to
// the next, there are 4 changes in the output code.
//
// - From an initial 0 - 0, Bit1 goes high, Bit0 stays low.
// - Then both bits are high, halfway through the step.
// - Then Bit1 goes low, but Bit2 stays high.
// - Finally at the end of the step, both bits return to 0.
//
// Detecting the direction is easy - the table simply goes in the other
// direction (read up instead of down).
//
// To decode this, we use a simple state machine. Every time the output
// code changes, it follows state, until finally a full steps worth of
// code is received (in the correct order). At the final 0-0, it returns
// a value indicating a step in one direction or the other.
//
// If an invalid state happens (for example we go from '0-1' straight
// to '1-0'), the state machine resets to the start until 0-0 and the
// next valid codes occur.
//
// The biggest advantage of using a state machine over other algorithms
// is that this has inherent debounce built in. Other algorithms emit spurious
// output with switch bounce, but this one will simply flip between
// sub-states until the bounce settles, then continue along the state
// machine.
// A side effect of debounce is that fast rotations can cause steps to
// be skipped. By not requiring debounce, fast rotations can be accurately
// measured.
// Another advantage is the ability to properly handle bad state, such
// as due to EMI, etc.
// It is also a lot simpler than others - a static state table and less
// than 10 lines of logic.
// END OF BENS COMMENT
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

#include "Arduino.h"
#include "ESPStepperMotorServer_Logger.h"
#include "ESPStepperMotorServer_RotaryEncoder.h"
#include "ESPStepperMotorServer.h"

// The below state table has, for each state (row), the new state
// to set based on the next encoder output. From left to right in,
// the table, the encoder outputs are 00, 01, 10, 11, and the value
// in that position is the new state to set.
#define R_START 0x0

// NOTE regarind HALF STEP support in the original Rotray Encoder library by BenBuxton:
// Half Step support has been removed to reduce complexity

// Use the full-step state table (emits a code at 00 only)
#define R_CW_FINAL 0x1
#define R_CW_BEGIN 0x2
#define R_CW_NEXT 0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT 0x6

const unsigned char ttable[7][4] = {
    // R_START
    {R_START, R_CW_BEGIN, R_CCW_BEGIN, R_START},
    // R_CW_FINAL
    {R_CW_NEXT, R_START, R_CW_FINAL, R_START | DIR_CW},
    // R_CW_BEGIN
    {R_CW_NEXT, R_CW_BEGIN, R_START, R_START},
    // R_CW_NEXT
    {R_CW_NEXT, R_CW_BEGIN, R_CW_FINAL, R_START},
    // R_CCW_BEGIN
    {R_CCW_NEXT, R_START, R_CCW_BEGIN, R_START},
    // R_CCW_FINAL
    {R_CCW_NEXT, R_CCW_FINAL, R_START, R_START | DIR_CCW},
    // R_CCW_NEXT
    {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};

ESPStepperMotorServer_RotaryEncoder::ESPStepperMotorServer_RotaryEncoder(char pinA, char pinB, String displayName, int stepMultiplier, byte stepperIndex)
{
    // Assign variables.
    this->_pinA = pinA;
    this->_pinB = pinB;
    this->_displayName = displayName;
    this->_stepMultiplier = stepMultiplier;
    this->_stepperIndex = stepperIndex;
    this->_encoderIndex = -1;
    // Set pins to input and enable pullup.
    pinMode(this->_pinA, INPUT_PULLUP);
    pinMode(this->_pinB, INPUT_PULLUP);
    // Initialise state.
    this->_state = R_START;
}

unsigned char ESPStepperMotorServer_RotaryEncoder::process()
{
    // Grab state of input pins.
    unsigned char pinstate = (digitalRead(this->_pinB) << 1) | digitalRead(this->_pinA);
    // Determine new state from the pins and state table.
    this->_state = ttable[this->_state & 0xf][pinstate];
    // Return emit bits, ie the generated event.
    return this->_state & 0x30;
}

void ESPStepperMotorServer_RotaryEncoder::setId(byte id)
{
    this->_encoderIndex = id;
}

byte ESPStepperMotorServer_RotaryEncoder::getId()
{
    return this->_encoderIndex;
}

unsigned char ESPStepperMotorServer_RotaryEncoder::getPinAIOPin()
{
    return this->_pinA;
}

unsigned char ESPStepperMotorServer_RotaryEncoder::getPinBIOPin()
{
    return this->_pinB;
}

void ESPStepperMotorServer_RotaryEncoder::setStepperIndex(byte stepperMotorIndex)
{
    if (stepperMotorIndex >= 0 && stepperMotorIndex <= ESPStepperHighestAllowedIoPin)
    {
        this->_stepperIndex = stepperMotorIndex;
    }
    else
    {
        ESPStepperMotorServer_Logger::logWarning("ESPStepperMotorServer_RotaryEncoder::setStepperIndex: Invalid stepper motor index value given, must be within he allowed range of 0 >= value <= ESPStepperHighestAllowedIoPin");
    }
}

byte ESPStepperMotorServer_RotaryEncoder::getStepperIndex()
{
    return this->_stepperIndex;
}

const String ESPStepperMotorServer_RotaryEncoder::getDisplayName()
{
    return this->_displayName;
}

void ESPStepperMotorServer_RotaryEncoder::setStepMultiplier(unsigned int stepMultiplier)
{
    this->_stepMultiplier = stepMultiplier;
}

unsigned int ESPStepperMotorServer_RotaryEncoder::getStepMultiplier()
{
    return this->_stepMultiplier;
}
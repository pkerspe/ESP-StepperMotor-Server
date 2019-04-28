//      ******************************************************************
//      *                                                                *
//      *       Header file for ESPStepperMotorServer_Logger.cpp         *
//      *                                                                *
//      *               Copyright (c) Paul Kerspe, 2019                  *
//      *                                                                *
//      ******************************************************************

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

#ifndef ESPStepperMotorServer_Logger_h
#define ESPStepperMotorServer_Logger_h

#include <Arduino.h>

#define ESPServerLogLevel_ALL 4
#define ESPServerLogLevel_DEBUG 3
#define ESPServerLogLevel_INFO 2
#define ESPServerLogLevel_WARNING 1

//
// the ESPStepperMotorServer_Logger class
class ESPStepperMotorServer_Logger
{
  public:
    ESPStepperMotorServer_Logger();
    ESPStepperMotorServer_Logger(String logName);
    static void setLogLevel(byte);
    static byte getLogLevel(void);
    static void logDebug(const char *msg, boolean newLine = true, boolean ommitLogLevel = false);
    static void logDebug(String msg, boolean newLine = true, boolean ommitLogLevel = false);
    static void logInfo(const char *msg, boolean newLine = true, boolean ommitLogLevel = false);
    static void logWarning(const char *msg, boolean newLine = true, boolean ommitLogLevel = false);
    char logString[400];

  private:
    static void log(const char *level, const char *msg, boolean newLine, boolean ommitLogLevel);
    static void printBinaryWithLeaingZeros(char *result, byte var);
    static byte _logLevel;
    String _loggerName;
};

#endif

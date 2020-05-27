//      *******************************************************************
//      *                                                                 *
//      *  ESP8266 and ESP32 Stepper Motor Server  - Logging class        *
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

#include <ESPStepperMotorServer_Logger.h>

byte ESPStepperMotorServer_Logger::_logLevel = ESPServerLogLevel_INFO;

char logString[400];
const char *LEVEL_STRING_DEBUG = "DEBUG";
const char *LEVEL_STRING_INFO = "INFO";
const char *LEVEL_STRING_WARNING = "WARNING";

ESPStepperMotorServer_Logger::ESPStepperMotorServer_Logger(String loggerName)
{
    this->_loggerName = loggerName;
}

ESPStepperMotorServer_Logger::ESPStepperMotorServer_Logger()
{
    this->_loggerName = "root";
}

void ESPStepperMotorServer_Logger::printBinaryWithLeaingZeros(char *result, byte var)
{
    int charIndex = 0;
    for (byte test = 0x80; test; test >>= 1)
    {
        result[charIndex] = (var & test ? '1' : '0');
        charIndex++;
        // Serial.write(var & test ? '1' : '0');
    }
    result[charIndex] = '\0';
}

void ESPStepperMotorServer_Logger::setLogLevel(byte logLevel)
{
    switch (logLevel)
    {
    case ESPServerLogLevel_ALL:
    case ESPServerLogLevel_DEBUG:
    case ESPServerLogLevel_INFO:
    case ESPServerLogLevel_WARNING:
        ESPStepperMotorServer_Logger::_logLevel = logLevel;
        break;
    default:
        ESPStepperMotorServer_Logger::logWarning("Invalid log level given, log level will be set to info");
        ESPStepperMotorServer_Logger::_logLevel = ESPServerLogLevel_INFO;
        break;
    }
}

byte ESPStepperMotorServer_Logger::getLogLevel()
{
    return ESPStepperMotorServer_Logger::_logLevel;
}

void ESPStepperMotorServer_Logger::logf(const char *level, const char *format, va_list args)
{
    char buf[1000];
    vsnprintf(buf, sizeof(buf), format, args);
    ESPStepperMotorServer_Logger::log(level, buf, false, false);
}

void ESPStepperMotorServer_Logger::log(const char *level, const char *msg, boolean newLine, boolean ommitLogLevel)
{
    if (!ommitLogLevel)
    {
        Serial.printf("[%s] ", level);
    }
    if (newLine == true)
    {
        Serial.println(msg);
    }
    else
    {
        Serial.print(msg);
    }
}

void ESPStepperMotorServer_Logger::logDebug(const char *msg, boolean newLine, boolean ommitLogLevel)
{
    if (getLogLevel() >= ESPServerLogLevel_DEBUG)
    {
        ESPStepperMotorServer_Logger::log(LEVEL_STRING_DEBUG, msg, newLine, ommitLogLevel);
    }
}

void ESPStepperMotorServer_Logger::logDebugf(const char *format, ...)
{
    if (getLogLevel() >= ESPServerLogLevel_DEBUG)
    {
        va_list args;
        va_start(args, format);
        ESPStepperMotorServer_Logger::logf(LEVEL_STRING_DEBUG, format, args);
        va_end(args);
    }
}

void ESPStepperMotorServer_Logger::logDebug(String msg, boolean newLine, boolean ommitLogLevel)
{
    ESPStepperMotorServer_Logger::logDebug(msg.c_str(), newLine, ommitLogLevel);
}

void ESPStepperMotorServer_Logger::logInfo(const char *msg, boolean newLine, boolean ommitLogLevel)
{
    if (getLogLevel() >= ESPServerLogLevel_INFO)
    {
        ESPStepperMotorServer_Logger::log(LEVEL_STRING_INFO, msg, newLine, ommitLogLevel);
    }
}
void ESPStepperMotorServer_Logger::logInfof(const char *format, ...)
{
    if (getLogLevel() >= ESPServerLogLevel_INFO)
    {
        va_list args;
        va_start(args, format);
        ESPStepperMotorServer_Logger::logf(LEVEL_STRING_INFO, format, args);
        va_end(args);
    }
}

void ESPStepperMotorServer_Logger::logWarning(const char *msg, boolean newLine, boolean ommitLogLevel)
{
    ESPStepperMotorServer_Logger::log(LEVEL_STRING_WARNING, msg, newLine, ommitLogLevel);
}

void ESPStepperMotorServer_Logger::logWarningf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    ESPStepperMotorServer_Logger::logf(LEVEL_STRING_WARNING, format, args);
    va_end(args);
}
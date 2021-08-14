//      ******************************************************************
//      *                                                                *
//      *       Header file for ESPStepperMotorServer_RestAPI.cpp        *
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

#ifndef ESPStepperMotorServer_RestAPI_h
#define ESPStepperMotorServer_RestAPI_h

#include <Arduino.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ESPStepperMotorServer_PositionSwitch.h>
#include <ESPStepperMotorServer_Logger.h>
#include <ESPStepperMotorServer.h>

//just declare class here for compiler, since we have a circular dependency
class ESPStepperMotorServer;

class ESPStepperMotorServer_RestAPI
{
public:
  ESPStepperMotorServer_RestAPI(ESPStepperMotorServer *stepperMotorServer);
  /**
     * register all rest endpoint handlers with the given ESP AsyncWebServer instance reference
     */
  void registerRestEndpoints(AsyncWebServer *server);

private:
  String version;
  ESPStepperMotorServer_Logger *logger;
  ESPStepperMotorServer *_stepperMotorServer;
  void populateStepperDetailsToJsonObject(JsonObject &detailsObjecToPopulate, ESPStepperMotorServer_StepperConfiguration *stepper, int index);
  void populateSwitchDetailsToJsonObject(JsonObject &detailsObjecToPopulate, ESPStepperMotorServer_PositionSwitch *positionSwitch, int index);
  void populateRotaryEncoderDetailsToJsonObject(JsonObject &detailsObjecToPopulate, ESPStepperMotorServer_RotaryEncoder *rotaryEncoder, int index);
  
  void logDebugRequestUrl(AsyncWebServerRequest *request);

  //movement related endpoints
  void handleHomingRequest(AsyncWebServerRequest *request);
  //for other endpoints see ESPStepperMotorServer_RestAPI.cpp in function registerRestEndpoints

  // SWITCH CONFIGURATION ENDPOINT HANDLER
  void handlePostSwitchRequest(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total, int switchIndex = -1);
  int handleDeleteSwitchRequest(AsyncWebServerRequest *request, boolean sendReponse);
  // STEPPER CONFIGURATION ENDPOINT HANDLER
  void handlePostStepperRequest(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total, int switchIndex = -1);
  int handleDeleteStepperRequest(AsyncWebServerRequest *request, boolean sendReponse);
  // ROTARY ENCODER CONFIGURATION ENDPOINT HANDLER
  void handlePostRotaryEncoderRequest(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total, int encoderIndex = -1);
  int handleDeleteRotaryEncoderRequest(AsyncWebServerRequest *request, boolean sendReponse);
};

#endif

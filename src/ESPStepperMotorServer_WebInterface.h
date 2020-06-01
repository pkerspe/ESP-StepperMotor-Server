//      ******************************************************************
//      *                                                                *
//      *  Header file for ESPStepperMotorServer_WebInterface.cpp        *
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

#ifndef ESPStepperMotorServer_WebInterface_h
#define ESPStepperMotorServer_WebInterface_h

#include <Arduino.h>
#include <HTTPClient.h>
#include <ESPStepperMotorServer.h>
#include <ESPStepperMotorServer_Logger.h>

class ESPStepperMotorServer_WebInterface
{
public:
  ESPStepperMotorServer_WebInterface(ESPStepperMotorServer *serverRef);
  void registerWebInterfaceUrls(AsyncWebServer *httpServer);

private:
  bool downloadFileToSpiffs(const char *url, const char *targetPath);
  bool checkIfGuiExistsInSpiffs();

  const char *webUiIndexFile = "/index.html";
  const char *webUiJsFile = "/js/app.js.gz";
  const char *webUiLogoFile = "/img/logo.svg";
  const char *webUiEncoderGraphic = "/img/rotaryEncoderWheel.svg";
  const char *webUiStepperGraphic = "/img/stepper.svg";
  const char *webUiSwitchGraphic = "/img/switch.svg";
  const char *webUiFaviconFile = "/favicon.ico";
  const char *webUiRepositoryBasePath = "https://raw.githubusercontent.com/pkerspe/ESP-StepperMotor-Server-UI/master/data";
  ESPStepperMotorServer *_serverRef;
  AsyncWebServer *_httpServer;
};

#endif

//      ******************************************************************
//      *                                                                *
//      *             Header file for ESPStepperMotorServer.cpp          *
//      *                                                                *
//      *               Copyright (c) Paul Kerspe, 2019                  *
//      *                                                                *
//      ******************************************************************

// this project is not supposed to replace a controller of a CNC machine but more of a general approach on working with stepper motors
// for a good Arduino/ESP base Gerber compatible controller Project see:
// https://github.com/gnea/grbl
// and for ESP32: https://github.com/bdring/Grbl_Esp32
// currently no G-Code (http://linuxcnc.org/docs/html/gcode.html) parser is implemented, yet it might be part of a future release
// other usefull informaion when connecting your ESP32 board to your driver boards and you are not sure which pins to use: https://randomnerdtutorials.com/esp32-pinout-reference-gpios/

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

#ifndef ESPStepperMotorServer_h
#define ESPStepperMotorServer_h

#include <Arduino.h>
#include <FlexyStepper.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>

#include <ESPStepperMotorServer_PositionSwitch.h>
#include <ESPStepperMotorServer_StepperConfiguration.h>
#include <ESPStepperMotorServer_RotaryEncoder.h>
#include <ESPStepperMotorServer_Logger.h>
#include <ESPStepperMotorServer_RestAPI.h>

#define ESPServerWifiModeClient 0
#define ESPServerWifiModeAccessPoint 1
#define ESPServerWifiModeDisabled 2

#define ESPServerRestApiEnabled 2
#define ESPServerWebserverEnabled 4
#define ESPServerSerialEnabled 8

#define ESPServerSwitchType_ActiveHigh 1
#define ESPServerSwitchType_ActiveLow 2

#define ESPServerSwitchType_HomingSwitchBegin 4
#define ESPServerSwitchType_HomingSwitchEnd 8
#define ESPServerSwitchType_GeneralPositionSwitch 16
#define ESPServerSwitchType_EmergencyStopSwitch 32

#ifndef ESPServerMaxSwitches
#define ESPServerMaxSwitches 10
#endif

#ifndef ESPServerSwitchStatusRegisterCount
#define ESPServerSwitchStatusRegisterCount 2 //NOTE: this value must be chosen according to the value of ESPServerMaxSwitches: val = ceil(ESPServerMaxSwitches / 8)
#endif

#ifndef ESPServerMaxSteppers
#define ESPServerMaxSteppers 5
#endif

#ifndef ESPServerMaxRotaryEncoders
#define ESPServerMaxRotaryEncoders 5
#endif

#define ESPStepperMotorServer_SwitchDisplayName_MaxLength 20

#define ESPServerPositionSwitchUnsetPinNumber 255
#define ESPStepperHighestAllowedIoPin 50

//just declare class here for compiler, since we have a circular dependency
class ESPStepperMotorServer_RestAPI;

//
// the ESPStepperMotorServer class
// TODO: remove all wifi stuff if not needed using: #if defined(ESPServerWifiModeClient) || defined(ESPServerWifiModeAccessPoint)
class ESPStepperMotorServer
{
public:
  ESPStepperMotorServer(byte serverMode);
  void setHttpPort(int portNumber);
  void setAccessPointName(const char *accessPointSSID);
  void setAccessPointPassword(const char *accessPointPassword);
  void setWifiCredentials(const char *ssid, const char *pwd);
  void setWifiMode(byte wifiMode);
  void printWifiStatus();
  int addStepper(ESPStepperMotorServer_StepperConfiguration *stepper);
  int addPositionSwitch(byte stepperIndex, byte ioPinNumber, byte switchType, String positionName, long switchPosition = -1);
  int addPositionSwitch(positionSwitch posSwitchToAdd, int switchIndex = -1);
  int addRotaryEncoder(ESPStepperMotorServer_RotaryEncoder *rotaryEncoderToAdd);
  void removePositionSwitch(int positionSwitchIndex);
  void removePositionSwitch(positionSwitch *posSwitchToRemove);
  void removeStepper(int stepperConfigurationIndex);
  void removeStepper(ESPStepperMotorServer_StepperConfiguration *stepper);
  void removeRotaryEncoder(int rotaryEncoderConfigurationIndex);
  void removeRotaryEncoder(ESPStepperMotorServer_RotaryEncoder *rotaryEncoderToAdd);
  void printPositionSwitchStatus();
  void start();
  void stop();
  byte getPositionSwitchStatus(int positionSwitchIndex);
  void getButtonStatusRegister(byte buffer[ESPServerSwitchStatusRegisterCount]);

  //delegator functions only
  void setLogLevel(byte);
  void getStatusAsJsonString(String &statusString);
  ESPStepperMotorServer_StepperConfiguration *getConfiguredStepper(byte index);
  positionSwitch *getConfiguredSwitch(byte index);
  bool isIoPinUsed(int);

  //
  // public member variables
  //
  int wifiClientConnectionTimeoutSeconds = 25;
  //this boolean value indicates wether a state change has occurred on one of the position switches or not
  volatile boolean positionSwitchUpdateAvailable = false;
  // a boolean indicating if a position switch that has been configure as emegrency switch, has been triggered
  volatile boolean emergencySwitchIsActive = false;

private:
  void scanWifiNetworks();
  void connectToWifiNetwork();
  void startAccessPoint();
  void startWebserver();
  void registerRestApiEndpoints();
  void registerWebInterfaceUrls();
  void startSPIFFS();
  void printSPIFFSStats();
  int getSPIFFSFreeSpace();
  bool checkIfGuiExistsInSpiffs();
  void printSPIFFSRootFolderContents();
  bool downloadFileToSpiffs(const char *url, const char *targetPath);
  void setupPositionSwitchIOPin(positionSwitch *posSwitchToAdd);
  void detachInterruptForPositionSwitch(positionSwitch *posSwitch);
  void detachInterruptForRotaryEncoder(ESPStepperMotorServer_RotaryEncoder *rotaryEncoder);
  void detachAllInterrupts();
  void attachAllInterrupts();
  void setPositionSwitchStatus(int positionSwitchIndex, byte status);
  // ISR handling
  static void staticPositionSwitchISR();
  void internalPositionSwitchISR();
  static void staticRotaryEncoderISR();
  void internalRotaryEncoderISR();

  //
  // private member variables
  //
  int httpPortNumber;
  byte wifiMode;
  byte enabledServices;
  const char *accessPointSSID;
  const char *accessPointPassword;
  const char *wifiClientSsid;
  const char *wifiPassword;
  const char *version = "0.0.1";
  const char *webUiIndexFile = "/index.html";
  const char *webUiJsFile = "/js/app.js.gz";
  const char *webUiLogoFile = "/img/logo.svg";
  const char *webUiEncoderGraphic = "/img/rotaryEncoderWheel.svg";
  const char *webUiStepperGraphic = "/img/stepper.svg";
  const char *webUiSwitchGraphic = "/img/switch.svg";
  const char *webUiFaviconFile = "/favicon.ico.gz";
  const char *webUiRepositoryBasePath = "https://raw.githubusercontent.com/pkerspe/ESP-StepperMotor-Server/master/data";
  boolean isWebserverEnabled = false;
  boolean isRestApiEnabled = false;
  boolean isServerStarted = false;
  char logString[400];

  ESPStepperMotorServer_RestAPI *restApiHandler;
  ESPStepperMotorServer_StepperConfiguration *configuredSteppers[ESPServerMaxSteppers] = {NULL};
  byte configuredStepperIndex = 0;
  static ESPStepperMotorServer *anchor; //used for self-reference in ISR
  positionSwitch configuredPositionSwitches[ESPServerMaxSwitches];
  // this byte indicated the index on the configuration array for position switches where the next configuration will be stored
  // the last added position switch thus has the index value configuredPositionSwitchIndex-1
  byte configuredPositionSwitchIndex = 0;
  // the button status register for all configured button switches
  volatile byte buttonStatus[ESPServerSwitchStatusRegisterCount] = {0};
  // a helper array the contains only the IO pins of all configured postion switches for faster access in the ISR
  volatile byte configuredPositionSwitchIoPins[ESPServerMaxSwitches];
  // an array of all position switch configuration indexes where a emergency switch is connected to
  volatile byte emergencySwitchIndexes[ESPServerMaxSwitches];
  // an array to hold all configured rotary encoders
  ESPStepperMotorServer_RotaryEncoder *configuredRotaryEncoders[ESPServerMaxRotaryEncoders] = {NULL};

  AsyncWebServer *httpServer;
};

// ------------------------------------ End ---------------------------------
#endif

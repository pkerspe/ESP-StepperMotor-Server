
//      ******************************************************************
//      *                                                                *
//      *    Header file for ESPStepperMotorServer_Configuration.cpp     *
//      *                                                                *
//      *               Copyright (c) Paul Kerspe, 2019                  *
//      *                                                                *
//      ******************************************************************

// this class repesents the complete configuration object and provide
// helper functions to persist and load the configuration form the SPIFFS of the ESP

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

#ifndef ESPStepperMotorServer_Configuration_h
#define ESPStepperMotorServer_Configuration_h

#include <ArduinoJson.h>
#include <FS.h>
#include <ESPStepperMotorServer.h>
#include <ESPStepperMotorServer_Logger.h>
#include <ESPStepperMotorServer_PositionSwitch.h>
#include <ESPStepperMotorServer_RotaryEncoder.h>
#include <ESPStepperMotorServer_StepperConfiguration.h>

// TODO: get IO pin configration rules implemented and helper to provide information about which pins to use and which not
// https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
// Input only pins
// GPIOs 34 to 39 are GPIs – input only pins. These pins don’t have internal pull-ups or pull-down resistors. They can’t be used as outputs, so use these pins only as inputs:
// GPIO 34, GPIO 35, GPIO 36, GPIO 39
// GPIO 12 boot fail if pulled high, better not use as active low

#define DEFAULT_SERVER_PORT 80
#define DEFAULT_WIFI_MODE 1
//
// the ESPStepperMotorServer_Configuration class
class ESPStepperMotorServer_Configuration
{
  friend class ESPStepperMotorServer;

public:
  ESPStepperMotorServer_Configuration(const char *configFilePath);
  String getCurrentConfigurationAsJSONString(bool prettyPrint = true, bool includePasswords = false);
  unsigned int calculateRequiredJsonDocumentSizeForCurrentConfiguration();
  void printCurrentConfigurationAsJsonToSerial();
  bool saveCurrentConfiguationToSpiffs(String filename = "");
  bool loadConfiguationFromSpiffs(String filename = "");
  void serializeServerConfiguration(JsonDocument &doc, bool includePasswords = false);

  byte addStepperConfiguration(ESPStepperMotorServer_StepperConfiguration *stepperConfig);
  byte addSwitch(ESPStepperMotorServer_PositionSwitch *positionSwitch);
  byte addRotaryEncoder(ESPStepperMotorServer_RotaryEncoder *encoder);
  void setStepperConfiguration(ESPStepperMotorServer_StepperConfiguration *stepperConfig, byte id);
  void setSwitch(ESPStepperMotorServer_PositionSwitch *positionSwitch, byte id);
  void setRotaryEncoder(ESPStepperMotorServer_RotaryEncoder *encoder, byte id);
  void removeStepperConfiguration(byte id);
  void removeSwitch(byte id);
  void removeRotaryEncoder(byte id);
  ESPStepperMotorServer_StepperConfiguration *getStepperConfiguration(unsigned char id);
  ESPStepperMotorServer_PositionSwitch *getSwitch(byte id);
  ESPStepperMotorServer_RotaryEncoder *getRotaryEncoder(unsigned char id);
  ESP_FlexyStepper **getConfiguredFlexySteppers();
  // a cache containing all IO pins that are used by switches. The indexes matches the indexes in the configuredSwitches (=switch ID)
  // -1 is used to indicate an emtpy array slot
  signed char allSwitchIoPins[ESPServerMaxSwitches];
  int serverPort = DEFAULT_SERVER_PORT;
  int wifiMode = 1;
  const char *apName = "ESPStepperMotorServer";
  const char *apPassword = "Aa123456";
  const char *wifiSsid;
  const char *wifiPassword;
  //this "cache" should not be private since we need to use it in the ISRs and any getter to retrieve it would slow down processing
  ESPStepperMotorServer_PositionSwitch *configuredEmergencySwitches[ESPServerMaxSwitches] = {NULL};

private:
  //
  // private member variables
  //
  boolean isCurrentConfigurationSaved = false;
  const char *_configFilePath;

  /**** the follwoing variables represent the in-memory configuration settings *******/
  // an array to hold all configured stepper configurations
  ESPStepperMotorServer_StepperConfiguration *configuredSteppers[ESPServerMaxSteppers] = {NULL};

  // this is a shortcut/cache for all configured flexy stepper instances, yet it will not have the same indexes as the configuredSteppers,
  // but solely an array that is filled from the beginnnig without emtpy slots.
  // it is used to have a quick access to configured flexy steppers in time critical functions
  void updateConfiguredFlexyStepperCache(void);
  ESP_FlexyStepper *configuredFlexySteppers[ESPServerMaxSteppers] = {NULL};
  // an array to hold all configured switches
  ESPStepperMotorServer_PositionSwitch *allConfiguredSwitches[ESPServerMaxSwitches] = {NULL};
  // update the caches for emergency and limit switches
  void updateSwitchCaches();
  ESPStepperMotorServer_PositionSwitch *configuredLimitSwitches[ESPServerMaxSwitches] = {NULL};
  // an array to hold all configured rotary encoders
  ESPStepperMotorServer_RotaryEncoder *configuredRotaryEncoders[ESPServerMaxRotaryEncoders] = {NULL};

  /////////////////////////////////////////////////////
  // CONSTANTS FOR JSON CONFIGURATION PROPERTY NAMES //
  /////////////////////////////////////////////////////
  // GENERAL SERVER CONFIGURATION //
  const char *JSON_SECTION_NAME_SERVER_CONFIGURATION = "serverConfiguration";
  const char *JSON_PROPERTY_NAME_PORT_NUMBER = "port";
  const char *JSON_PROPERTY_NAME_WIFI_MODE = "wififMode"; //allowed values are 0 (wifi off = ESPServerWifiModeDisabled),1 (AP mode = ESPServerWifiModeAccessPoint) and 2 (client mode = ESPServerWifiModeClient)
  const char *JSON_PROPERTY_NAME_WIFI_SSID = "wifiSsid";
  const char *JSON_PROPERTY_NAME_WIFI_PASSWORD = "wifiPassword";
  const char *JSON_PROPERTY_NAME_WIFI_AP_NAME = "apName";
  const char *JSON_PROPERTY_NAME_WIFI_AP_PASSWORD = "apPassword";

  // STEPPER SPECIFIC CONFIGURATION //
  const char *JSON_SECTION_NAME_STEPPER_CONFIGURATIONS = "stepperConfigurations";

  // SWITCH SPECIFIC CONFIGURATION //
  const char *JSON_SECTION_NAME_SWITCH_CONFIGURATIONS = "switchConfigurations";

  // ROTARY ENCODER SPECIFIC CONFIGURATION //
  const char *JSON_SECTION_NAME_ROTARY_ENCODER_CONFIGURATIONS = "rotaryEncoderConfigurations";
};
#endif


//      *********************************************************
//      *                                                       *
//      *           Stepper Motor Server Configuration           *
//      *                                                       *
//      *            Copyright (c) Paul Kerspe, 2019            *
//      *                                                       *
//      **********************************************************
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

#include "ESPStepperMotorServer_Configuration.h"

#define RESERVED_JSON_SIZE_ESPStepperMotorServer_Configuration 300

//
// constructor for the stepper server configuration class
//
ESPStepperMotorServer_Configuration::ESPStepperMotorServer_Configuration(const char *configFilePath)
{
  this->_configFilePath = configFilePath;
  this->loadConfiguationFromSpiffs();
  if (ESPStepperMotorServer_Logger::getLogLevel() >= ESPServerLogLevel_DEBUG)
  {
    this->printCurrentConfigurationAsJsonToSerial();
  }
}

unsigned int ESPStepperMotorServer_Configuration::calculateRequiredJsonDocumentSizeForCurrentConfiguration()
{
  unsigned int size = 0;
  size += RESERVED_JSON_SIZE_ESPStepperMotorServer_PositionSwitch * ESPServerMaxSwitches;
  size += RESERVED_JSON_SIZE_ESPStepperMotorServer_RotaryEncoder * ESPServerMaxRotaryEncoders;
  size += RESERVED_JSON_SIZE_ESPStepperMotorServer_StepperConfiguration * ESPServerMaxSteppers;
  size += RESERVED_JSON_SIZE_ESPStepperMotorServer_Configuration;
  return size;
}

void ESPStepperMotorServer_Configuration::printCurrentConfigurationAsJsonToSerial()
{
  DynamicJsonDocument doc(this->calculateRequiredJsonDocumentSizeForCurrentConfiguration());
  this->serializeServerConfiguration(doc, false);
  serializeJsonPretty(doc, Serial);
  Serial.println();
}

String ESPStepperMotorServer_Configuration::getCurrentConfigurationAsJSONString(bool prettyPrint, bool includePasswords)
{
  DynamicJsonDocument doc(this->calculateRequiredJsonDocumentSizeForCurrentConfiguration());
  this->serializeServerConfiguration(doc, includePasswords);
  String output;
  if (prettyPrint)
  {
    serializeJsonPretty(doc, output);
  }
  else
  {
    serializeJson(doc, output);
  }
  return output;
}

void ESPStepperMotorServer_Configuration::serializeServerConfiguration(JsonDocument &doc, bool includePasswords)
{
  // Set the values in the document
  doc[JSON_SECTION_NAME_SERVER_CONFIGURATION][JSON_PROPERTY_NAME_PORT_NUMBER] = this->serverPort;
  doc[JSON_SECTION_NAME_SERVER_CONFIGURATION][JSON_PROPERTY_NAME_WIFI_MODE] = this->wifiMode;
  doc[JSON_SECTION_NAME_SERVER_CONFIGURATION][JSON_PROPERTY_NAME_WIFI_SSID] = this->wifiSsid;
  doc[JSON_SECTION_NAME_SERVER_CONFIGURATION][JSON_PROPERTY_NAME_WIFI_PASSWORD] = (includePasswords) ? this->wifiPassword : "*****";
  doc[JSON_SECTION_NAME_SERVER_CONFIGURATION][JSON_PROPERTY_NAME_WIFI_AP_NAME] = this->apName;
  doc[JSON_SECTION_NAME_SERVER_CONFIGURATION][JSON_PROPERTY_NAME_WIFI_AP_PASSWORD] =(includePasswords) ? this->apPassword : "*****";
  // add all stepper configs
  JsonArray stepperConfigArray = doc.createNestedArray(JSON_SECTION_NAME_STEPPER_CONFIGURATIONS);
  for (byte stepperConfigIndex = 0; stepperConfigIndex < ESPServerMaxSteppers; stepperConfigIndex++)
  {
    ESPStepperMotorServer_StepperConfiguration *stepperConfig = this->getStepperConfiguration(stepperConfigIndex);
    if (stepperConfig)
    {
      JsonObject nestedStepperConfig = stepperConfigArray.createNestedObject();
      nestedStepperConfig["id"] = stepperConfig->getId();
      nestedStepperConfig["name"] = stepperConfig->getDisplayName();
      nestedStepperConfig["stepPin"] = stepperConfig->getStepIoPin();
      nestedStepperConfig["directionPin"] = stepperConfig->getDirectionIoPin();
      nestedStepperConfig["stepsPerRev"] = stepperConfig->getStepsPerRev();
      nestedStepperConfig["stepsPerMM"] = stepperConfig->getStepsPerMM();
      nestedStepperConfig["microsteppingDivisor"] = stepperConfig->getMicrostepsPerStep();
      nestedStepperConfig["rpmLimit"] = stepperConfig->getRpmLimit();
    }
  }

  // add all switch configs
  JsonArray switchConfigArray = doc.createNestedArray(JSON_SECTION_NAME_SWITCH_CONFIGURATIONS);
  for (byte switchConfigIndex = 0; switchConfigIndex < ESPServerMaxSwitches; switchConfigIndex++)
  {
    ESPStepperMotorServer_PositionSwitch *switchConfig = this->getSwitch(switchConfigIndex);
    if (switchConfig)
    {
      JsonObject nestedSwitchConfig = switchConfigArray.createNestedObject();
      nestedSwitchConfig["id"] = switchConfig->getId();
      nestedSwitchConfig["name"] = switchConfig->getPositionName();
      nestedSwitchConfig["ioPin"] = switchConfig->getIoPinNumber();
      nestedSwitchConfig["stepperIndex"] = switchConfig->getStepperIndex();
      nestedSwitchConfig["switchType"] = switchConfig->getSwitchType();
      nestedSwitchConfig["switchPosition"] = switchConfig->getSwitchPosition();
    }
  }

  // add all rotary encoder configs
  JsonArray encoderConfigArray = doc.createNestedArray(JSON_SECTION_NAME_ROTARY_ENCODER_CONFIGURATIONS);
  for (byte encoderConfigIndex = 0; encoderConfigIndex < ESPServerMaxRotaryEncoders; encoderConfigIndex++)
  {
    ESPStepperMotorServer_RotaryEncoder *encoderConfig = this->getRotaryEncoder(encoderConfigIndex);
    if (encoderConfig)
    {
      JsonObject nestedEncoderConfig = encoderConfigArray.createNestedObject();
      nestedEncoderConfig["id"] = encoderConfig->getId();
      nestedEncoderConfig["name"] = encoderConfig->getDisplayName();
      nestedEncoderConfig["pinA"] = encoderConfig->getPinAIOPin();
      nestedEncoderConfig["pinB"] = encoderConfig->getPinBIOPin();
      nestedEncoderConfig["stepMultiplier"] = encoderConfig->getStepMultiplier();
      nestedEncoderConfig["stepperIndex"] = encoderConfig->getStepperIndex();
    }
  }
}

bool ESPStepperMotorServer_Configuration::saveCurrentConfiguationToSpiffs(String filename)
{
  if (filename == "")
  {
    filename = this->_configFilePath;
  }
  // assemble the json object first, to check if all goes well
  // Allocate a temporary JsonDocument
  DynamicJsonDocument doc(this->calculateRequiredJsonDocumentSizeForCurrentConfiguration());
  this->serializeServerConfiguration(doc, true);

  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(filename);
  // Open file for writing
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file)
  {
    ESPStepperMotorServer_Logger::logWarningf("Failed to create new configuration file '%s' in SPIFFS\n", filename.c_str());
    return false;
  }
  bool success = false;
  // Serialize JSON to file
  if (serializeJson(doc, file) == 0)
  {
    ESPStepperMotorServer_Logger::logWarningf("Failed to write new configuration to file '%s' in SPIFFS\n", filename.c_str());
  }
  else
  {
    ESPStepperMotorServer_Logger::logInfof("New configuration file written in SPIFFS to '%s'\n", filename.c_str());
    success = true;
  }

  // Close the file
  file.close();
  return success;
}

bool ESPStepperMotorServer_Configuration::loadConfiguationFromSpiffs(String filename)
{
  filename = (filename == "") ? this->_configFilePath : filename;
  filename = (filename.startsWith("/")) ? filename : "/" + filename;

  if (SPIFFS.exists(filename))
  {
    ESPStepperMotorServer_Logger::logInfof("Loading configuration file %s from SPIFFS\n", filename.c_str());
    File configFile = SPIFFS.open(filename, FILE_READ);
    DynamicJsonDocument doc(this->calculateRequiredJsonDocumentSizeForCurrentConfiguration());
    this->serializeServerConfiguration(doc);
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, configFile);
    if (error)
      ESPStepperMotorServer_Logger::logWarningf("Failed to read configuration file %s. Will use fallback default configuration\n", filename.c_str());
    else
    {
      ESPStepperMotorServer_Logger::logDebug("File loaded and deserialized");
    }
    // Copy values from the JsonDocument to the Config

    // SERVER CONFIG
    this->serverPort = doc[JSON_SECTION_NAME_SERVER_CONFIGURATION][JSON_PROPERTY_NAME_PORT_NUMBER] | DEFAULT_SERVER_PORT;
    this->wifiMode = doc[JSON_SECTION_NAME_SERVER_CONFIGURATION][JSON_PROPERTY_NAME_WIFI_MODE] | DEFAULT_WIFI_MODE;

    JsonVariant value = doc[JSON_SECTION_NAME_SERVER_CONFIGURATION][JSON_PROPERTY_NAME_WIFI_AP_NAME];
    this->apName = (value) ? value.as<char *>() : "ESP-StepperMotor-Server";

    value = doc[JSON_SECTION_NAME_SERVER_CONFIGURATION][JSON_PROPERTY_NAME_WIFI_AP_PASSWORD];
    this->apPassword = (value) ? value.as<char *>() : "Aa123456";

    this->wifiSsid = doc[JSON_SECTION_NAME_SERVER_CONFIGURATION][JSON_PROPERTY_NAME_WIFI_SSID].as<char *>();
    this->wifiPassword = doc[JSON_SECTION_NAME_SERVER_CONFIGURATION][JSON_PROPERTY_NAME_WIFI_PASSWORD].as<char *>();

    // STEPPER CONFIG
    JsonArray configs = doc[JSON_SECTION_NAME_STEPPER_CONFIGURATIONS].as<JsonArray>();
    byte configCounter = 0;
    if (!configs)
    {
      ESPStepperMotorServer_Logger::logInfo("No stepper configuration present in config file");
    }
    else
    {
      for (JsonVariant stepperConfigEntry : configs)
      {
        const char *value = stepperConfigEntry["name"].as<char *>();
        ESPStepperMotorServer_StepperConfiguration *stepperConfig = new ESPStepperMotorServer_StepperConfiguration(
            (stepperConfigEntry["stepPin"] | 255),
            (stepperConfigEntry["directionPin"] | 255),
            ((value) ? value : "undefined"),
            (stepperConfigEntry["stepsPerRev"] | 200),
            (stepperConfigEntry["stepsPerMM"] | 100),
            (stepperConfigEntry["microsteppingDivisor"] | ESPSMS_MICROSTEPS_OFF),
            (stepperConfigEntry["rpmLimit"] | 1000));
        if (stepperConfigEntry["id"])
        {
          this->setStepperConfiguration(stepperConfig, stepperConfigEntry["id"]);
        }
        else
        {
          this->addStepperConfiguration(stepperConfig);
        }
        configCounter++;
      }
      ESPStepperMotorServer_Logger::logInfof("%i stepper configuration entr%s loaded from config file\n", configCounter, (configCounter == 1) ? "y" : "ies");
    }

    // SWITCH CONFIG
    configCounter = 0;
    configs = doc[JSON_SECTION_NAME_SWITCH_CONFIGURATIONS].as<JsonArray>();
    if (!configs)
    {
      ESPStepperMotorServer_Logger::logInfo("No switch configuration present in config file");
    }
    else
    {
      for (JsonVariant switchConfigEntry : configs)
      {
        const char *value = switchConfigEntry["name"].as<char *>();
        ESPStepperMotorServer_PositionSwitch *switchConfig = new ESPStepperMotorServer_PositionSwitch(
            (switchConfigEntry["ioPin"] | 255),
            (switchConfigEntry["stepperIndex"] | 255),
            (switchConfigEntry["switchType"] | 255),
            ((value) ? value : "undefined"),
            (switchConfigEntry["switchPosition"] | 0));
        if (switchConfigEntry["id"])
        {
          this->setSwitch(switchConfig, switchConfigEntry["id"]);
        }
        else
        {
          this->addSwitch(switchConfig);
        }
        configCounter++;
      }
      ESPStepperMotorServer_Logger::logInfof("%i switch configuration entr%s loaded from config file\n", configCounter, (configCounter == 1) ? "y" : "ies");
    }

    // ENCODER CONFIG
    configCounter = 0;
    configs = doc[JSON_SECTION_NAME_ROTARY_ENCODER_CONFIGURATIONS].as<JsonArray>();
    if (!configs)
    {
      ESPStepperMotorServer_Logger::logInfo("No rotary encoder configuration present in config file");
    }
    else
    {
      for (JsonVariant encoderConfigEntry : configs)
      {
        const char *value = encoderConfigEntry["name"].as<char *>();
        //char pinA, char pinB, String displayName, int stepMultiplier, byte stepperIndex
        ESPStepperMotorServer_RotaryEncoder *encoderConfig = new ESPStepperMotorServer_RotaryEncoder(
            (encoderConfigEntry["pinA"] | 255),
            (encoderConfigEntry["pinB"] | 255),
            ((value) ? value : "undefined"),
            (encoderConfigEntry["stepMultiplier"] | 255),
            (encoderConfigEntry["stepperIndex"] | 255));
        if (encoderConfigEntry["id"])
        {
          this->setRotaryEncoder(encoderConfig, encoderConfigEntry["id"]);
        }
        else
        {
          this->addRotaryEncoder(encoderConfig);
        }
        configCounter++;
      }
      ESPStepperMotorServer_Logger::logInfof("%i rotary encoder configuration entr%s loaded from config file\n", configCounter, (configCounter == 1) ? "y" : "ies");
    }

    // Close the file
    configFile.close();
    return true;
  }
  else
  {
    ESPStepperMotorServer_Logger::logWarningf("Failed to load configuration file from SPIFFS. File %s not found\n", filename);
    return false;
  }
}

byte ESPStepperMotorServer_Configuration::addStepperConfiguration(ESPStepperMotorServer_StepperConfiguration *stepperConfig)
{
  //find first index that is not NULL and use as id
  byte id = 0;
  for (id = 0; id < ESPServerMaxSteppers; id++)
  {
    if (this->configuredSteppers[id] == NULL)
    {
      stepperConfig->setId(id);
      this->configuredSteppers[id] = stepperConfig;
      this->updateConfiguredFlexyStepperCache();
      return id;
    }
  }
  ESPStepperMotorServer_Logger::logWarningf("The maximum amount of stepper configurations (%i) that can be configured has been reached, no more stepper configs can be added\n", ESPServerMaxSteppers);
  return 255;
}

byte ESPStepperMotorServer_Configuration::addSwitch(ESPStepperMotorServer_PositionSwitch *positionSwitch)
{
  //find first index that is not NULL and use as id
  byte id = 0;
  for (id = 0; id < ESPServerMaxSwitches; id++)
  {
    if (this->allConfiguredSwitches[id] == NULL)
    {
      positionSwitch->setId(id);
      this->allConfiguredSwitches[id] = positionSwitch;
      this->updateSwitchCaches();
      return id;
    }
  }
  ESPStepperMotorServer_Logger::logWarningf("The maximum amount of switches (%i) that can be configured has been reached, no more switches can be added\n", ESPServerMaxSwitches);
  return 255;
}

byte ESPStepperMotorServer_Configuration::addRotaryEncoder(ESPStepperMotorServer_RotaryEncoder *encoder)
{
  //find first index that is not NULL and use as id
  byte id = 0;
  for (id = 0; id < ESPServerMaxRotaryEncoders; id++)
  {
    if (this->configuredRotaryEncoders[id] == NULL)
    {
      encoder->setId(id);
      this->configuredRotaryEncoders[id] = encoder;
      return id;
    }
  }
  ESPStepperMotorServer_Logger::logWarningf("The maximum amount of rotary encoders (%i) that can be configured has been reached, no more encoders can be added\n", ESPServerMaxRotaryEncoders);
  return 255;
}

void ESPStepperMotorServer_Configuration::setStepperConfiguration(ESPStepperMotorServer_StepperConfiguration *stepperConfig, byte id)
{
  if (id >= ESPServerMaxSteppers)
  {
    ESPStepperMotorServer_Logger::logWarningf("The given stepper id/index (%i) exceeds the allowed max amount of %i. Stepper config will not be set\n", id, ESPServerMaxSteppers);
  }
  else
  {
    stepperConfig->setId(id);
    this->configuredSteppers[id] = stepperConfig;
  }
  this->updateConfiguredFlexyStepperCache();
}

void ESPStepperMotorServer_Configuration::setSwitch(ESPStepperMotorServer_PositionSwitch *positionSwitch, byte id)
{
  if (id >= ESPServerMaxSwitches)
  {
    ESPStepperMotorServer_Logger::logWarningf("The given switch id/index (%i) exceeds the allowed max amount of %i. Switch config will not be set\n", id, ESPServerMaxSteppers);
  }
  else
  {
    positionSwitch->setId(id);
    this->allConfiguredSwitches[id] = positionSwitch;
    this->updateSwitchCaches();
  }
}

void ESPStepperMotorServer_Configuration::setRotaryEncoder(ESPStepperMotorServer_RotaryEncoder *encoder, byte id)
{
  if (id >= ESPServerMaxRotaryEncoders)
  {
    ESPStepperMotorServer_Logger::logWarningf("The given rotray encoder id/index (%i) exceeds the allowed max amount of %i. Rotray encoder config will not be set\n", id, ESPServerMaxSteppers);
  }
  else
  {
    encoder->setId(id);
    this->configuredRotaryEncoders[id] = encoder;
  }
}

ESPStepperMotorServer_StepperConfiguration *ESPStepperMotorServer_Configuration::getStepperConfiguration(unsigned char id)
{
  if (id >= ESPServerMaxSteppers)
  {
    ESPStepperMotorServer_Logger::logWarningf("Invalid stepper config requested with id %i. Will retun NULL\n", id);
    return NULL;
  }
  return this->configuredSteppers[id];
}

void ESPStepperMotorServer_Configuration::updateConfiguredFlexyStepperCache()
{
  byte flexyStepperCounter = 0;
  ESPStepperMotorServer_StepperConfiguration *stepper;

  //clear list first
  for (byte i = 0; i < ESPServerMaxSteppers; i++)
  {
    //TODO: check if this delete call is appropriate, currently it casues kernel panic
    //delete (this->configuredFlexySteppers[i]);
    this->configuredFlexySteppers[i] = NULL;
  }

  for (byte i = 0; i < ESPServerMaxSteppers; i++)
  {
    stepper = this->getStepperConfiguration(i);
    if (stepper)
    {
      this->configuredFlexySteppers[flexyStepperCounter] = stepper->getFlexyStepper();
      flexyStepperCounter++;
    }
  }
}

ESP_FlexyStepper **ESPStepperMotorServer_Configuration::getConfiguredFlexySteppers()
{
  return this->configuredFlexySteppers;
}

ESPStepperMotorServer_PositionSwitch *ESPStepperMotorServer_Configuration::getSwitch(byte id)
{
  if (id < 0 || id > ESPServerMaxSwitches-1)
  {
    ESPStepperMotorServer_Logger::logWarningf("Invalid switch config requested with id %i. Will retun NULL\n", id);
    return NULL;
  }
  return this->allConfiguredSwitches[id];
}

ESPStepperMotorServer_RotaryEncoder *ESPStepperMotorServer_Configuration::getRotaryEncoder(unsigned char id)
{
  if (id >= ESPServerMaxRotaryEncoders)
  {
    ESPStepperMotorServer_Logger::logWarningf("Invalid rotary encoder config requested with id %i. Will retun NULL\n", id);
    return NULL;
  }
  return this->configuredRotaryEncoders[id];
}

void ESPStepperMotorServer_Configuration::removeStepperConfiguration(byte id)
{
  //check if any switches are connected to this stepper and delete those
  for (byte switchIndex = 0; switchIndex < ESPServerMaxSwitches; switchIndex++)
  {
    ESPStepperMotorServer_PositionSwitch *switchConfig = this->getSwitch(switchIndex);
    if (switchConfig && switchConfig->getStepperIndex() == id)
    {
      ESPStepperMotorServer_Logger::logDebugf("Found switch configuration (id=%i) that is linked to stepper config (id=%i) to be deleted. Will delete switch config as well\n", switchConfig->getId(), id);
      this->removeSwitch(switchIndex);
    }
  }
  //check if any switches are connected to this stepper and delete those
  for (byte encoderIndex = 0; encoderIndex < ESPServerMaxRotaryEncoders; encoderIndex++)
  {
    ESPStepperMotorServer_RotaryEncoder *encoderConfig = this->getRotaryEncoder(encoderIndex);
    if (encoderConfig && encoderConfig->getStepperIndex() == id)
    {
      ESPStepperMotorServer_Logger::logDebugf("Found encoder configuration (id=%i) that is linked to stepper config (id=%i) to be deleted. Will delete encoder config as well\n", encoderConfig->getId(), id);
      this->removeRotaryEncoder(encoderIndex);
    }
  }
  //finally delete the stepper config itself
  //TODO: check if this delete call is appropriate, currently it casues kernel panic
  //delete (this->configuredSteppers[id]);
  this->configuredSteppers[id] = NULL;
  this->updateConfiguredFlexyStepperCache();
}

void ESPStepperMotorServer_Configuration::removeSwitch(byte id)
{
  //TODO: check if this delete call is appropriate, currently it casues kernel panic
  //delete (this->allConfiguredSwitches[id]);
  this->allConfiguredSwitches[id] = NULL;
  this->updateSwitchCaches();
}

void ESPStepperMotorServer_Configuration::updateSwitchCaches()
{
  //reset all caches first
  for (byte i = 0; i < ESPServerMaxSwitches; i++)
  {
    //TODO: check if this is appropriate, currently it casues kernel panic
    //delete (this->configuredEmergencySwitches[i]);
    this->configuredEmergencySwitches[i] = NULL;
    //TODO: check if this delete call is appropriate, currently it casues kernel panic
    //delete (this->configuredLimitSwitches[i]);
    this->configuredLimitSwitches[i] = NULL;
    this->allSwitchIoPins[i] = (signed char)-1;
  }

  //now rebuild the caches
  byte emergencySwitchCacheIndex = 0;
  byte limitSwitchCacheIndex = 0;

  for (byte i = 0; i < ESPServerMaxSwitches; i++)
  {
    if (this->allConfiguredSwitches[i])
    {
      this->allSwitchIoPins[i] = this->allConfiguredSwitches[i]->getIoPinNumber();

      if (this->allConfiguredSwitches[i]->isEmergencySwitch())
      {
        this->configuredEmergencySwitches[emergencySwitchCacheIndex] = this->allConfiguredSwitches[i];
        emergencySwitchCacheIndex++;
      }
      else if (this->allConfiguredSwitches[i]->isLimitSwitch())
      {
        this->configuredLimitSwitches[limitSwitchCacheIndex] = this->allConfiguredSwitches[i];
        limitSwitchCacheIndex++;
      }
    }
  }
}

void ESPStepperMotorServer_Configuration::removeRotaryEncoder(byte id)
{
  //TODO: check if this delete call is appropriate, currently it casues kernel panic
  //delete (this->configuredRotaryEncoders[id]);
  this->configuredRotaryEncoders[id] = NULL;
}
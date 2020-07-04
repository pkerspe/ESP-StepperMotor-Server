//      *********************************************************
//      *                                                       *
//      *     ESP8266 and ESP32 Stepper Motor Server            *
//      *                                                       *
//      *            Copyright (c) Paul Kerspe, 2019            *
//      *                                                       *
//      **********************************************************

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
// This library is used to start a server (Webserver, REST API or via serial port) to control and configure one or more stepper motors via a stepper driver module with step and direction input as well as optional homing switches
//
// Usage:
//    Near the top of the program, add:
//        include "ESPStepperMotorServer.h"
//
//    e.g. to start the server and enable the web based uster interface, the REST API and the serial server use:
//        ESPStepperMotorServer server(ESPServerRestApiEnabled|ESPServerWebserverEnabled|ESPServerSerialEnabled);
//    eg. to only start the web user interface and disable the rest API and serial server use:
//        ESPStepperMotorServer server(ESPServerWebserverEnabled);
//
//    if the server is started with the ESPServerWebserverEnabled or ESPServerRestApiEnabled flag, you can specify a http port (default is port 80), for the server to listen for connections (e.g. port 80 in this example) if only starting in serial mode, you can ommit this step
//        server.setHttpPort(80);
//
//    if you want the server to connect to an existing wifi network, you need to set the wifi ssid, username and password (replace "somessid", "someUser" and "somePwd" with the appropriate values for you network)
//        server.setWifiCredentials("someSsid","someUser","somePwd");
//    OR if you do NOT want to connect to an EXISTING wifi network, then omit the setWifiCredentials call and instead configure the server to stat a separate Access Point by setting the Access Point name of your choice
//    NOTE: if you do not setWifiCredentials or setAccessPointName the server will start in access point mode with an ssid of "ESPStepperMotorServer"
//        server.setAccessPointName("myStepperServer");
//
//    after configuring the basic server, start it up as the last required step in the setup function:
//        server.start();
//

#include <ESPStepperMotorServer.h>

// ---------------------------------------------------------------------------------
//                                  Setup functions
// ---------------------------------------------------------------------------------

ESPStepperMotorServer *ESPStepperMotorServer::anchor = NULL;

ESPStepperMotorServer::ESPStepperMotorServer(const ESPStepperMotorServer &espStepperMotorServer)
{
  this->serverConfiguration = new ESPStepperMotorServer_Configuration(espStepperMotorServer.serverConfiguration->_configFilePath);
  *this->serverConfiguration = *espStepperMotorServer.serverConfiguration;

  if (espStepperMotorServer.webInterfaceHandler)
  {
    this->webInterfaceHandler = new ESPStepperMotorServer_WebInterface(this);
    *this->webInterfaceHandler = *espStepperMotorServer.webInterfaceHandler;
  }

  if (espStepperMotorServer.restApiHandler)
  {
    this->restApiHandler = new ESPStepperMotorServer_RestAPI(this);
    *this->restApiHandler = *espStepperMotorServer.restApiHandler;
  }

  if (espStepperMotorServer.cliHandler)
  {
    this->cliHandler = new ESPStepperMotorServer_CLI(this);
    *this->cliHandler = *espStepperMotorServer.cliHandler;
  }

  if (espStepperMotorServer.motionControllerHandler)
  {
    this->motionControllerHandler = new ESPStepperMotorServer_MotionController(this);
    *this->motionControllerHandler = *espStepperMotorServer.motionControllerHandler;
  }

  if (espStepperMotorServer.httpServer)
  {
    this->httpServer = new AsyncWebServer(this->serverConfiguration->serverPort);
    *this->httpServer = *espStepperMotorServer.httpServer;
  }

  if (espStepperMotorServer.webSockerServer)
  {
    this->webSockerServer = new AsyncWebSocket("/ws");
    *this->webSockerServer = *espStepperMotorServer.webSockerServer;
  }
}

ESPStepperMotorServer::~ESPStepperMotorServer()
{
  delete this->serverConfiguration;
  delete this->webInterfaceHandler;
  delete this->restApiHandler;
  delete this->cliHandler;
  delete this->motionControllerHandler;
}

//
// constructor for the stepper server class
//
ESPStepperMotorServer::ESPStepperMotorServer(byte serverMode, byte logLevel)
{
  ESPStepperMotorServer_Logger::setLogLevel(logLevel);
  startSPIFFS();
  //get config instance which tries to load config from SPIFFS by default
  this->serverConfiguration = new ESPStepperMotorServer_Configuration(this->defaultConfigurationFilename);

  this->enabledServices = serverMode;
  if ((this->enabledServices & ESPServerWebserverEnabled) == ESPServerWebserverEnabled)
  {
    this->isWebserverEnabled = true;
    this->webInterfaceHandler = new ESPStepperMotorServer_WebInterface(this);
  }
  //rest api needs to be started either if web UI is enabled (which uses the REST API itself) or if REST API is enabled
  if ((this->enabledServices & ESPServerRestApiEnabled) == ESPServerRestApiEnabled || this->isWebserverEnabled)
  {
    this->isRestApiEnabled = true;
    this->restApiHandler = new ESPStepperMotorServer_RestAPI(this);
  }
  if ((this->enabledServices & ESPServerSerialEnabled) == ESPServerSerialEnabled)
  {
    this->isCLIEnabled = true;
    this->cliHandler = new ESPStepperMotorServer_CLI(this);
  }

  this->motionControllerHandler = new ESPStepperMotorServer_MotionController(this);

  if (ESPStepperMotorServer::anchor != NULL)
  {
    ESPStepperMotorServer_Logger::logWarning("ESPStepperMotorServer must be used as a singleton, do not instanciate more than one server in your project");
  }
  else
  {
    ESPStepperMotorServer::anchor = this;
  }
}

// ---------------------------------------------------------------------------------
//                     general service control functions
// ---------------------------------------------------------------------------------

void ESPStepperMotorServer::start()
{
  ESPStepperMotorServer_Logger::logInfof("Starting ESP-StepperMotor-Server (v. %s)\n", this->version);

  if (this->serverConfiguration->wifiMode == ESPServerWifiModeAccessPoint)
  {
    this->startAccessPoint();
    if (ESPStepperMotorServer_Logger::getLogLevel() >= ESPServerLogLevel_DEBUG)
    {
      this->printWifiStatus();
    }
  }
  else if (this->serverConfiguration->wifiMode == ESPServerWifiModeClient)
  {
    this->connectToWifiNetwork();
    if (ESPStepperMotorServer_Logger::getLogLevel() >= ESPServerLogLevel_DEBUG)
    {
      this->printWifiStatus();
    }
  }
  else if (this->serverConfiguration->wifiMode == ESPServerWifiModeDisabled)
  {
    ESPStepperMotorServer_Logger::logInfo("WiFi mode is disabled, only serial control interface will be used for controls");
  }
  this->startWebserver();
  this->setupAllIOPins();
  this->attachAllInterrupts();

  if (this->isCLIEnabled)
  {
    this->cliHandler->start();
  }
  this->motionControllerHandler->start();
  this->isServerStarted = true;
}

void ESPStepperMotorServer::stop()
{
  ESPStepperMotorServer_Logger::logInfo("Stopping ESP-StepperMotor-Server");
  this->motionControllerHandler->stop();
  this->detachAllInterrupts();
  ESPStepperMotorServer_Logger::logInfo("detached interrupt handlers");

  if (isWebserverEnabled || isRestApiEnabled)
  {
    this->httpServer->end();
    ESPStepperMotorServer_Logger::logInfo("stopped web server");
  }

  if (this->isCLIEnabled)
  {
    this->cliHandler->stop();
  }
  this->isServerStarted = false;
  ESPStepperMotorServer_Logger::logInfo("ESP-StepperMotor-Server stopped");
}

// ---------------------------------------------------------------------------------
//                                  Configuration Functions
// ---------------------------------------------------------------------------------

void ESPStepperMotorServer::setHttpPort(int portNumber)
{
  this->serverConfiguration->serverPort = portNumber;
}

ESPStepperMotorServer_Configuration *ESPStepperMotorServer::getCurrentServerConfiguration()
{
  return this->serverConfiguration;
}

int ESPStepperMotorServer::addOrUpdateRotaryEncoder(ESPStepperMotorServer_RotaryEncoder *encoder, int encoderIndex)
{
  //add encoder to configuration
  if (encoderIndex > -1)
  {
    this->serverConfiguration->setRotaryEncoder(encoder, encoderIndex);
  }
  else
  {
    encoderIndex = (unsigned int)this->serverConfiguration->addRotaryEncoder(encoder);
  }
  this->setupRotaryEncoderIOPin(encoder);
  return encoderIndex;
}

/**
 * add or updated an existing stepper configuration 
 * return the id of the stepper config (in case a new one has been added, or the given one in case of an update)
 * -1 is returned in case of an error (e.g. maximum number of configs reached for this entity)
 */
int ESPStepperMotorServer::addOrUpdateStepper(ESPStepperMotorServer_StepperConfiguration *stepper, int stepperIndex)
{
  if (stepper->getStepIoPin() == ESPStepperMotorServer_StepperConfiguration::ESPServerStepperUnsetIoPinNumber ||
      stepper->getDirectionIoPin() == ESPStepperMotorServer_StepperConfiguration::ESPServerStepperUnsetIoPinNumber)
  {
    ESPStepperMotorServer_Logger::logWarningf("Either the step IO pin (%i) or direction IO (%i) pin, or both, are not set correctly. Use a valid IO Pin value between 0 and the highest available IO Pin on your ESP\n", stepper->getStepIoPin(), stepper->getDirectionIoPin());
    return -1;
  }
  //set IO Pins for stepper
  pinMode(stepper->getDirectionIoPin(), OUTPUT);
  digitalWrite(stepper->getDirectionIoPin(), LOW);
  pinMode(stepper->getStepIoPin(), OUTPUT);
  digitalWrite(stepper->getStepIoPin(), LOW);
  //add stepper to configuration or update existing one
  if (stepperIndex > -1)
  {
    this->serverConfiguration->setStepperConfiguration(stepper, stepperIndex);
  }
  else
  {
    stepperIndex = this->serverConfiguration->addStepperConfiguration(stepper);
  }

  return stepperIndex;
}

/**
 * remove the stepper configuration with the given index/id
 */
void ESPStepperMotorServer::removeStepper(byte id)
{
  if (this->serverConfiguration->getStepperConfiguration(id))
  {
    this->serverConfiguration->removeStepperConfiguration(id);
  }
  else
  {
    ESPStepperMotorServer_Logger::logWarningf("Stepper configuration index %i is invalid, no entry found or stepper IDs do not match, removeStepper() canceled\n", id);
  }
}

int ESPStepperMotorServer::addOrUpdatePositionSwitch(ESPStepperMotorServer_PositionSwitch *posSwitchToAdd, int switchIndex)
{
  if (switchIndex > -1)
  {
    this->serverConfiguration->setSwitch(posSwitchToAdd, switchIndex);
  }
  else
  {
    switchIndex = (unsigned int)this->serverConfiguration->addSwitch(posSwitchToAdd);
  }
  //Setup IO Pin
  this->setupPositionSwitchIOPin(posSwitchToAdd);

  ESPStepperMotorServer_Logger::logInfof("Added switch '%s' for IO pin %i at configuration index %i\n", this->serverConfiguration->getSwitch(switchIndex)->getPositionName().c_str(), this->serverConfiguration->getSwitch(switchIndex)->getIoPinNumber(), switchIndex);
  return switchIndex;
}

void ESPStepperMotorServer::removePositionSwitch(int positionSwitchIndex)
{
  ESPStepperMotorServer_PositionSwitch *posSwitch = this->serverConfiguration->getSwitch(positionSwitchIndex);
  if (posSwitch)
  {
    this->detachInterruptForPositionSwitch(posSwitch);
    ESPStepperMotorServer_Logger::logDebugf("Removing position switch '%s' (id: %i) from configured switches\n", posSwitch->getPositionName().c_str(), positionSwitchIndex);
    this->serverConfiguration->removeSwitch(positionSwitchIndex);
  }
  else
  {
    ESPStepperMotorServer_Logger::logWarning("Switch index %i is invalid, no switch configuration present at this index, removePositionSwitch() canceled\n", positionSwitchIndex);
  }
}

void ESPStepperMotorServer::removeRotaryEncoder(byte id)
{
  if (this->serverConfiguration->getRotaryEncoder(id))
  {
    this->serverConfiguration->removeRotaryEncoder(id);
  }
  else
  {
    ESPStepperMotorServer_Logger::logWarningf("rotary encoder index %i is invalid, no rotary encoder pointer present at this configuration index or rotary encoder IDs do not match, removeRotaryEncoder() canceled\n", id);
  }
}

// ---------------------------------------------------------------------------------
//                                  Status and Service Functions
// ---------------------------------------------------------------------------------

void ESPStepperMotorServer::getFormattedPositionSwitchStatusRegister(byte registerIndex, String &output)
{
  String binary = String(this->buttonStatus[registerIndex], BIN);
  for (int i = 0; i < 8 - binary.length(); i++)
  {
    output += "0";
  }
  output += binary;
}

void ESPStepperMotorServer::printPositionSwitchStatus()
{
  // TODO reuse code in REST API
  const int docSize = 150 * ESPServerMaxSwitches;
  StaticJsonDocument<docSize> doc;
  JsonObject root = doc.to<JsonObject>();
  JsonObject settings = root.createNestedObject("settings");
  settings["positionSwitchCounterLimit"] = ESPServerMaxSwitches;
  settings["statusRegisterCounter"] = ESPServerSwitchStatusRegisterCount;

  JsonArray data = root.createNestedArray("switchStatusRegister");

  for (byte i = 0; i < ESPServerSwitchStatusRegisterCount; i++)
  {
    JsonObject positionSwitchRegister = data.createNestedObject();
    positionSwitchRegister["statusRegisterIndex"] = i;

    String binaryPadded = "";
    this->getFormattedPositionSwitchStatusRegister(i, binaryPadded);
    positionSwitchRegister["status"] = binaryPadded;
  }

  JsonArray switches = root.createNestedArray("positionSwitches");
  for (int i = 0; i < ESPServerMaxSwitches; i++)
  {
    if (this->serverConfiguration->getSwitch(i))
    {
      JsonObject positionSwitch = switches.createNestedObject();
      positionSwitch["id"] = i;
      positionSwitch["name"] = this->serverConfiguration->getSwitch(i)->getPositionName();
      positionSwitch["ioPin"] = this->serverConfiguration->getSwitch(i)->getIoPinNumber();
      positionSwitch["position"] = this->serverConfiguration->getSwitch(i)->getSwitchPosition();
      positionSwitch["stepperId"] = this->serverConfiguration->getSwitch(i)->getStepperIndex();
      positionSwitch["active"] = this->getPositionSwitchStatus(i);

      JsonObject positionSwichType = positionSwitch.createNestedObject("type");
      if ((this->serverConfiguration->getSwitch(i)->getSwitchType() & ESPServerSwitchType_ActiveHigh) == ESPServerSwitchType_ActiveHigh)
      {
        positionSwichType["pinMode"] = "Active High";
      }
      else
      {
        positionSwichType["pinMode"] = "Active Low";
      }
      if ((this->serverConfiguration->getSwitch(i)->getSwitchType() & ESPServerSwitchType_HomingSwitchBegin) == ESPServerSwitchType_HomingSwitchBegin)
      {
        positionSwichType["switchType"] = "Homing switch (start-position)";
      }
      else if ((this->serverConfiguration->getSwitch(i)->getSwitchType() & ESPServerSwitchType_HomingSwitchEnd) == ESPServerSwitchType_HomingSwitchEnd)
      {
        positionSwichType["switchType"] = "Homing switch (end-position)";
      }
      else if ((this->serverConfiguration->getSwitch(i)->getSwitchType() & ESPServerSwitchType_GeneralPositionSwitch) == ESPServerSwitchType_GeneralPositionSwitch)
      {
        positionSwichType["switchType"] = "General position switch";
      }
      else if ((this->serverConfiguration->getSwitch(i)->getSwitchType() & ESPServerSwitchType_EmergencyStopSwitch) == ESPServerSwitchType_EmergencyStopSwitch)
      {
        positionSwichType["switchType"] = "Emergency shut down switch";
      }
    }
  }

  serializeJson(root, Serial);
  Serial.println();
}

/**
 * checks if the configured position switch at the given configuration index is triggered (active) or not
 * This function takes the configured type ESPServerSwitchType_ActiveHigh or ESPServerSwitchType_ActiveLow into account when determining the current active state
 * e.g. if a switch is configured to be ESPServerSwitchType_ActiveLow the function will return 1 when the switch is triggered (low signal on IO pin).
 * For a switch that is configured ESPServerSwitchType_ActiveHigh on the other side, the function will return 0 when a low signal is on the IO pin, and 1 when a high signal is present
 */
byte ESPStepperMotorServer::getPositionSwitchStatus(int positionSwitchIndex)
{
  ESPStepperMotorServer_PositionSwitch *posSwitch = this->serverConfiguration->getSwitch(positionSwitchIndex);
  if (posSwitch)
  {
    byte buttonState = digitalRead(posSwitch->getIoPinNumber());
    if ((posSwitch->getSwitchType() & ESPServerSwitchType_ActiveHigh) == ESPServerSwitchType_ActiveHigh)
    {
      return (buttonState) ? 1 : 0;
    }
    else
    {
      return (buttonState) ? 0 : 1;
    }
  }
  return 0;
}

// ---------------------------------------------------------------------------------
//                          Web Server and REST API functions
// ---------------------------------------------------------------------------------
void ESPStepperMotorServer::startSPIFFS()
{
  ESPStepperMotorServer_Logger::logDebug("Checking SPIFFS for existance and free space");
  if (SPIFFS.begin())
  {
    if (ESPStepperMotorServer_Logger::getLogLevel() >= ESPServerLogLevel_DEBUG)
    {
      ESPStepperMotorServer_Logger::logDebug("SPIFFS started");
      printSPIFFSStats();
    }
  }
  else
  {
    if (this->isWebserverEnabled)
    {
      ESPStepperMotorServer_Logger::logWarning("Unable to activate SPIFFS. Files for web interface cannot be loaded");
    }
  }
}

void ESPStepperMotorServer::printSPIFFSStats()
{
  Serial.println("SPIFFS stats:");
  Serial.printf("Total bytes: %i\n", (int)SPIFFS.totalBytes());
  Serial.printf("bytes used: %i\n", (int)SPIFFS.usedBytes());
  Serial.printf("bytes free: %i\n", getSPIFFSFreeSpace());
}

void ESPStepperMotorServer::printSPIFFSRootFolderContents()
{
  File root = SPIFFS.open("/");

  if (!root)
  {
    ESPStepperMotorServer_Logger::logWarning("Failed to open root folder on SPIFFS for reading");
  }
  if (root.isDirectory())
  {
    ESPStepperMotorServer_Logger::logInfo("Listing files in root folder of SPIFFS:");
    File file = root.openNextFile();
    while (file)
    {
      ESPStepperMotorServer_Logger::logInfof("File: %s (%i) %ld\n", file.name(), file.size(), file.getLastWrite());
      file = root.openNextFile();
    }
    root.close();
  }
}

int ESPStepperMotorServer::getSPIFFSFreeSpace()
{
  return ((int)SPIFFS.totalBytes() - (int)SPIFFS.usedBytes());
}

//TODO: test this part and implement sending of status updates
void ESPStepperMotorServer::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    ESPStepperMotorServer_Logger::logInfof("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    ESPStepperMotorServer_Logger::logInfof("ws[%s][%i] disconnect: %i\n", server->url(), client->id(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    ESPStepperMotorServer_Logger::logWarningf("ws[%s][%u] error(%i): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    ESPStepperMotorServer_Logger::logInfof("ws[%s][%i] pong[%i]: %s\n", server->url(), client->id(), (int)len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len)
    {
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
        {
          msg += (char)data[i];
        }
        String commandFromClient = String((char *)data);
        if (commandFromClient.equals("status"))
        {
          client->text("Here is your status: OK");
        }
      }
      else
      {
        char buff[10];
        for (size_t i = 0; i < info->len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t)data[i]);
          msg += buff;
        }
      }
      Serial.printf("%s\n", msg.c_str());

      if (info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    }
    else
    {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if (info->index == 0)
      {
        if (info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
        {
          msg += (char)data[i];
        }
      }
      else
      {
        char buff[10];
        for (size_t i = 0; i < info->len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t)data[i]);
          msg += buff;
        }
      }
      Serial.printf("%s\n", msg.c_str());

      if ((info->index + len) == info->len)
      {
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if (info->final)
        {
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
          if (info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}

void ESPStepperMotorServer::startWebserver()
{
  if (isWebserverEnabled || isRestApiEnabled)
  {
    printSPIFFSRootFolderContents();

    httpServer = new AsyncWebServer(this->serverConfiguration->serverPort);
    ESPStepperMotorServer_Logger::logInfof("Starting webserver on port %i\n", this->serverConfiguration->serverPort);

    webSockerServer = new AsyncWebSocket("/ws");
    webSockerServer->onEvent(std::bind(&ESPStepperMotorServer::onWebSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
    httpServer->addHandler(webSockerServer);

    if (isWebserverEnabled)
    {
      this->webInterfaceHandler->registerWebInterfaceUrls(this->httpServer);
    }
    if (isRestApiEnabled)
    {
      this->restApiHandler->registerRestEndpoints(this->httpServer);
    }
    // SETUP CORS responses/headers
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Access-Control-Allow-Headers, Origin,Accept, X-Requested-With, Content-Type, Access-Control-Request-Method, Access-Control-Request-Headers");

    httpServer->onNotFound([](AsyncWebServerRequest *request) {
      if (request->method() == HTTP_OPTIONS)
      {
        request->send(200);
      }
      else
      {
        request->send(404);
      }
    });

    httpServer->begin();
    ESPStepperMotorServer_Logger::logInfof("Webserver started, you can now open the user interface on http://%s:%i/\n", this->getIpAddress().c_str(), this->serverConfiguration->serverPort);
  }
}

String ESPStepperMotorServer::getIpAddress()
{
  if (this->serverConfiguration->wifiMode == ESPServerWifiModeAccessPoint)
  {
    return WiFi.softAPIP().toString();
  }
  else if (this->serverConfiguration->wifiMode == ESPServerWifiModeClient)
  {
    return WiFi.localIP().toString();
  }
  else
  {
    return "not connected";
  }
}

/**
 * get some server status information as a JSON formatted string.
 * Returns: version, wifi mode, ip address, spiffs information, enabled server modules
 */
void ESPStepperMotorServer::getServerStatusAsJsonString(String &statusString)
{
  StaticJsonDocument<200> doc;
  JsonObject root = doc.to<JsonObject>();
  root["version"] = this->version;

  JsonObject wifiStatus = root.createNestedObject("wifi");
  wifiStatus["mode"] = (this->serverConfiguration->wifiMode == ESPServerWifiModeAccessPoint) ? "ap" : "client";
  wifiStatus["ip"] = (this->serverConfiguration->wifiMode == ESPServerWifiModeAccessPoint) ? WiFi.dnsIP().toString() : WiFi.localIP().toString();

  JsonObject spiffsStatus = root.createNestedObject("spiffss");
  spiffsStatus["total_space"] = (int)SPIFFS.totalBytes();
  spiffsStatus["free_space"] = this->getSPIFFSFreeSpace();

  JsonObject activeModules = root.createNestedObject("activeModules");
  activeModules["serial_cli"] = (this->cliHandler != NULL);
  activeModules["rest_api"] = (this->isRestApiEnabled);
  activeModules["web_ui"] = (this->isWebserverEnabled);

  serializeJson(root, statusString);
}

// ---------------------------------------------------------------------------------
//             helper functions for stepper communication
// ---------------------------------------------------------------------------------
bool ESPStepperMotorServer::isIoPinUsed(int pinToCheck)
{
  //TODO move to server configuration class
  //check stepper configurations
  for (int i = 0; i < ESPServerMaxSteppers; i++)
  {
    ESPStepperMotorServer_StepperConfiguration *stepperConfig = this->serverConfiguration->getStepperConfiguration(i);
    if (stepperConfig && (stepperConfig->getDirectionIoPin() == pinToCheck || stepperConfig->getStepIoPin() == pinToCheck))
    {
      return true;
    }
  }
  //check switch configurations
  for (int i = 0; i < ESPServerMaxSwitches; i++)
  {
    ESPStepperMotorServer_PositionSwitch *switchConfig = this->serverConfiguration->getSwitch(i);
    if (switchConfig && switchConfig->getIoPinNumber() == pinToCheck)
    {
      return true;
    }
  }
  return false;

  //check encoder configurations
  for (int i = 0; i < ESPServerMaxRotaryEncoders; i++)
  {
    ESPStepperMotorServer_RotaryEncoder *encoderConfig = this->serverConfiguration->getRotaryEncoder(i);
    if (encoderConfig && (encoderConfig->getPinAIOPin() == pinToCheck || encoderConfig->getPinBIOPin() == pinToCheck))
    {
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------------
//                                  WiFi functions
// ---------------------------------------------------------------------------------

void ESPStepperMotorServer::setAccessPointName(const char *accessPointSSID)
{
  this->serverConfiguration->apName = accessPointSSID;
}

void ESPStepperMotorServer::setAccessPointPassword(const char *accessPointPassword)
{
  this->serverConfiguration->apPassword = accessPointPassword;
}

void ESPStepperMotorServer::setWifiMode(byte wifiMode)
{
  switch (wifiMode)
  {
  case ESPServerWifiModeAccessPoint:
    this->serverConfiguration->wifiMode = ESPServerWifiModeAccessPoint;
    break;
  case ESPServerWifiModeClient:
    this->serverConfiguration->wifiMode = ESPServerWifiModeClient;
    break;
  case ESPServerWifiModeDisabled:
    this->serverConfiguration->wifiMode = ESPServerWifiModeDisabled;
    break;
  default:
    ESPStepperMotorServer_Logger::logWarning("Invalid WiFi mode given in setWifiMode");
    break;
  }
}

/**
 * print the wifi status (ssid, IP Address etc.) on the serial port
 */
void ESPStepperMotorServer::printWifiStatus()
{
  ESPStepperMotorServer_Logger::logInfo("ESPStepperMotorServer WiFi details:");

  if (this->serverConfiguration->wifiMode == ESPServerWifiModeClient)
  {
    ESPStepperMotorServer_Logger::logInfo("WiFi status: server acts as wifi client in existing network with DHCP");
    ESPStepperMotorServer_Logger::logInfof("SSID: %s\n", this->getCurrentServerConfiguration()->wifiSsid);
    ESPStepperMotorServer_Logger::logInfof("IP address: %s\n", WiFi.localIP().toString().c_str());
    ESPStepperMotorServer_Logger::logInfof("Strength: %i dBm\n", WiFi.RSSI()); //Received Signal Strength Indicator
  }
  else if (this->serverConfiguration->wifiMode == ESPServerWifiModeAccessPoint)
  {
    ESPStepperMotorServer_Logger::logInfo("WiFi status: access point started");
    ESPStepperMotorServer_Logger::logInfof("SSID: %s\n", this->serverConfiguration->apName);
    ESPStepperMotorServer_Logger::logInfof("IP Address: %s\n", WiFi.softAPIP().toString().c_str());
  }
  else
  {
    ESPStepperMotorServer_Logger::logInfo("WiFi is disabled");
  }
}

void ESPStepperMotorServer::setWifiSSID(const char *ssid)
{
  this->getCurrentServerConfiguration()->wifiSsid = ssid;
}

void ESPStepperMotorServer::setWifiPassword(const char *pwd)
{
  this->getCurrentServerConfiguration()->wifiPassword = pwd;
}

void ESPStepperMotorServer::setWifiCredentials(const char *ssid, const char *pwd)
{
  this->setWifiSSID(ssid);
  this->setWifiPassword(pwd);
}

void ESPStepperMotorServer::startAccessPoint()
{
  WiFi.softAP(this->serverConfiguration->apName, this->serverConfiguration->apPassword);
  ESPStepperMotorServer_Logger::logInfof("Started Access Point with name %s and IP %s\n", this->serverConfiguration->apName, WiFi.softAPIP().toString().c_str());
}

void ESPStepperMotorServer::connectToWifiNetwork()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    ESPStepperMotorServer_Logger::logInfo("Module is already conencted to WiFi network. Will skip WiFi connection procedure");
    return;
  }

  if ((this->getCurrentServerConfiguration()->wifiSsid != NULL) && (this->getCurrentServerConfiguration()->wifiSsid[0] == '\0'))
  {
    ESPStepperMotorServer_Logger::logWarning("No SSID has been configured to connect to. Connection to existing WiFi network aborted");
    return;
  }

  ESPStepperMotorServer_Logger::logInfof("Trying to connect to WiFi with SSID '%s' ...", this->getCurrentServerConfiguration()->wifiSsid);
  WiFi.begin(this->getCurrentServerConfiguration()->wifiSsid, this->getCurrentServerConfiguration()->wifiPassword);
  int retryIntervalMs = 500;
  int timeoutCounter = this->wifiClientConnectionTimeoutSeconds * (1000 / retryIntervalMs);
  while (WiFi.status() != WL_CONNECTED && timeoutCounter > 0)
  {
    delay(retryIntervalMs);
    ESPStepperMotorServer_Logger::logInfo(".", false, true);
    if (timeoutCounter == (this->wifiClientConnectionTimeoutSeconds * 2 - 3))
    {
      WiFi.reconnect();
    }
    timeoutCounter--;
  }
  ESPStepperMotorServer_Logger::logInfo("\n", false, true);

  if (timeoutCounter > 0)
  {
    ESPStepperMotorServer_Logger::logInfof("Connected to network with IP address %s\n", WiFi.localIP().toString().c_str());
  }
  else
  {
    ESPStepperMotorServer_Logger::logWarningf("Connection to WiFi network with SSID '%s' failed with timeout\n", this->getCurrentServerConfiguration()->wifiSsid);
    ESPStepperMotorServer_Logger::logDebugf("Connection timeout is set to %i seconds\n", this->wifiClientConnectionTimeoutSeconds);
    ESPStepperMotorServer_Logger::logWarningf("starting server in access point mode with SSID '%s' and password '%s' as fallback\n", this->serverConfiguration->apName, this->serverConfiguration->apPassword);
    this->setWifiMode(ESPServerWifiModeAccessPoint);
    this->startAccessPoint();
  }
}

void ESPStepperMotorServer::scanWifiNetworks()
{
  int numberOfNetworks = WiFi.scanNetworks();

  Serial.print("Number of networks found:");
  Serial.println(numberOfNetworks);

  for (int i = 0; i < numberOfNetworks; i++)
  {
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID(i));

    Serial.print("Signal strength: ");
    Serial.println(WiFi.RSSI(i));
  }
}

// ---------------------------------------------------------------------------------
//                                 IO Setup and Interrupt functions
// ---------------------------------------------------------------------------------

/**
 * setup the IO Pin to INPUT mode
 */
void ESPStepperMotorServer::setupPositionSwitchIOPin(ESPStepperMotorServer_PositionSwitch *posSwitch)
{
  if (posSwitch)
  {
    if (posSwitch->isActiveHigh())
    {
      ESPStepperMotorServer_Logger::logDebugf("Setting up IO pin %i as input for active high switch '%s' (%i)\n", posSwitch->getIoPinNumber(), posSwitch->getPositionName().c_str(), posSwitch->getId());
      pinMode(posSwitch->getIoPinNumber(), INPUT);
    }
    else
    {
      if (posSwitch->getIoPinNumber() == 34 || posSwitch->getIoPinNumber() == 35 || posSwitch->getIoPinNumber() == 36 || posSwitch->getIoPinNumber() == 39)
      {
        ESPStepperMotorServer_Logger::logWarningf("The configured IO pin %i cannot be used for active low switches unless an external pull up resistor is in place. The ESP does not provide internal pullups on this IO pin. Make sure you have a pull up resistor in place for the switch %s (%i)\n", posSwitch->getIoPinNumber(), posSwitch->getPositionName(), posSwitch->getId());
      }
      ESPStepperMotorServer_Logger::logDebugf("Setting up IO pin %i as input with pullup for active low switch '%s' (%i)\n", posSwitch->getIoPinNumber(), posSwitch->getPositionName().c_str(), posSwitch->getId());
      pinMode(posSwitch->getIoPinNumber(), INPUT_PULLUP);
    }
  }
}

void ESPStepperMotorServer::setupRotaryEncoderIOPin(ESPStepperMotorServer_RotaryEncoder *rotaryEncoder)
{
  //set Pins for encoder
  ESPStepperMotorServer_Logger::logDebugf("Setting up IO pin %i as Pin A input for active high rotary encoder '%s' (%i)\n", rotaryEncoder->getPinAIOPin(), rotaryEncoder->getDisplayName().c_str(), rotaryEncoder->getId());
  pinMode(rotaryEncoder->getPinAIOPin(), INPUT);
  ESPStepperMotorServer_Logger::logDebugf("Setting up IO pin %i as Pin B input for active high rotary encoder '%s' (%i)\n", rotaryEncoder->getPinBIOPin(), rotaryEncoder->getDisplayName().c_str(), rotaryEncoder->getId());
  pinMode(rotaryEncoder->getPinBIOPin(), INPUT);
}

/**
 * setup the IO Pins for all configured switches and encoders
 */
void ESPStepperMotorServer::setupAllIOPins()
{
  //setup IO pins for all switches
  for (byte switchId = 0; switchId < ESPServerMaxSwitches; switchId++)
  {
    ESPStepperMotorServer_PositionSwitch *switchConfig = this->serverConfiguration->getSwitch(switchId);
    if (switchConfig)
    {
      this->setupPositionSwitchIOPin(switchConfig);
    }
  }
  //Setup IO pins for all encoders
  for (byte encoderId = 0; encoderId < ESPServerMaxRotaryEncoders; encoderId++)
  {
    ESPStepperMotorServer_RotaryEncoder *encoderConfig = this->serverConfiguration->getRotaryEncoder(encoderId);
    if (encoderConfig)
    {
      this->setupRotaryEncoderIOPin(encoderConfig);
    }
  }

  this->updateSwitchStatusRegister();
}

/**
 * Register ISR according to switch type (active high or active low) for all configured position switches
 */
void ESPStepperMotorServer::attachAllInterrupts()
{
  for (int i = 0; i < ESPServerMaxSwitches; i++)
  {
    ESPStepperMotorServer_PositionSwitch *posSwitch = this->serverConfiguration->getSwitch(i);
    if (posSwitch)
    {
      char irqNum = digitalPinToInterrupt(posSwitch->getIoPinNumber());
      if (irqNum == NOT_AN_INTERRUPT)
      {
        ESPStepperMotorServer_Logger::logWarningf("Failed to determine IRQ# for given IO pin %i, thus setting up of interrupt for the position switch '%s' failed\n", posSwitch->getIoPinNumber(), posSwitch->getPositionName().c_str());
      }
      else
      {
        _BV(irqNum); // clear potentially pending interrupts
        //register emergency stop switches
        if (posSwitch->isEmergencySwitch())
        {
          ESPStepperMotorServer_Logger::logDebugf("Attaching interrupt service routine for emergency stop switch '%s' on IO pin %i\n", posSwitch->getPositionName().c_str(), posSwitch->getIoPinNumber());
          attachInterrupt(irqNum, staticEmergencySwitchISR, CHANGE);
        }
        //register limit switches
        else if (posSwitch->isLimitSwitch())
        {
          ESPStepperMotorServer_Logger::logDebugf("Attaching interrupt service routine for limit switch '%s' on IO pin %i\n", posSwitch->getPositionName().c_str(), posSwitch->getIoPinNumber());
          attachInterrupt(irqNum, staticLimitSwitchISR, CHANGE);
        }
        //register general position switches & others
        else
        {
          ESPStepperMotorServer_Logger::logDebugf("Attaching interrupt service routine for general position switch '%s' on IO pin %i\n", posSwitch->getPositionName().c_str(), posSwitch->getIoPinNumber());
          attachInterrupt(irqNum, staticPositionSwitchISR, CHANGE);
        }
      }
    }
  }

  for (int i = 0; i < ESPServerMaxRotaryEncoders; i++)
  {
    ESPStepperMotorServer_RotaryEncoder *rotaryEncoder = this->serverConfiguration->getRotaryEncoder(i);
    if (rotaryEncoder != NULL)
    {
      //we do a loop here to save some program memory, could also externalize code block in another function
      const unsigned char pins[2] = {rotaryEncoder->getPinAIOPin(), rotaryEncoder->getPinBIOPin()};
      for (int i = 0; i < 2; i++)
      {
        char irqNum = digitalPinToInterrupt(pins[i]);
        if (irqNum == NOT_AN_INTERRUPT)
        {
          ESPStepperMotorServer_Logger::logWarningf("Failed to determine IRQ# for given IO pin %i, thus setting up of interrupt for the rotary encoder failed for pin %s\n", pins[i], rotaryEncoder->getDisplayName().c_str());
        }

        //ESPStepperMotorServer_Logger::logDebugf("attaching interrupt service routine for rotary encoder '%s' on IO pin %i\n", rotaryEncoder->getDisplayName().c_str(), pins[i]);
        _BV(irqNum); // clear potentially pending interrupts
        attachInterrupt(irqNum, staticRotaryEncoderISR, CHANGE);
      }
    }
  }
}

void ESPStepperMotorServer::detachInterruptForPositionSwitch(ESPStepperMotorServer_PositionSwitch *posSwitch)
{
  ESPStepperMotorServer_Logger::logDebugf("detaching interrupt for position switch %s on IO Pin %i\n", posSwitch->getPositionName().c_str(), posSwitch->getIoPinNumber());
  detachInterrupt(digitalPinToInterrupt(posSwitch->getIoPinNumber()));
}

void ESPStepperMotorServer::detachInterruptForRotaryEncoder(ESPStepperMotorServer_RotaryEncoder *rotaryEncoder)
{
  ESPStepperMotorServer_Logger::logDebugf("detaching interrupts for rotary encoder %s on IO Pins %i and %i\n", rotaryEncoder->getDisplayName().c_str(), rotaryEncoder->getPinAIOPin(), rotaryEncoder->getPinBIOPin());
  //Pin A of rotary encoder
  if (digitalPinToInterrupt(rotaryEncoder->getPinAIOPin()) != NOT_AN_INTERRUPT)
  {
    detachInterrupt(digitalPinToInterrupt(rotaryEncoder->getPinAIOPin()));
  }
  //Pin B of rotary encoder
  if (digitalPinToInterrupt(rotaryEncoder->getPinBIOPin()) != NOT_AN_INTERRUPT)
  {
    detachInterrupt(digitalPinToInterrupt(rotaryEncoder->getPinBIOPin()));
  }
}

/**
 * clear/disable all interrupts for position switches
 **/
void ESPStepperMotorServer::detachAllInterrupts()
{
  for (int i = 0; i < ESPServerMaxSwitches; i++)
  {
    ESPStepperMotorServer_PositionSwitch *posSwitch = this->serverConfiguration->getSwitch(i);
    if (posSwitch)
    {
      this->detachInterruptForPositionSwitch(posSwitch);
    }
  }
  for (int i = 0; i < ESPServerMaxRotaryEncoders; i++)
  {
    ESPStepperMotorServer_RotaryEncoder *rotaryEncoder = this->serverConfiguration->getRotaryEncoder(i);
    if (rotaryEncoder != NULL)
    {
      this->detachInterruptForRotaryEncoder(rotaryEncoder);
    }
  }
}

/**
 * Trigger an emergency stop. Can be called with optional stepper ID to only trigger emergency stop for a specific stepper.
 * If called without the parameter, all configured steppers will be stopped
 **************************************************************************************************************************
 * IMPORTANT NOTE: This function can be called manually, but will also be called from the ISR of the Emegerncy switches,  *
 * so it should be kept as short as possible and not use the CoProcessor (E.g. for floating point arithmetic operations)  *
 ************************************************************************************************************************** 
 */
void ESPStepperMotorServer::performEmergencyStop(int stepperId)
{
  this->emergencySwitchIsActive = true;
  //only perform emergency stop for one stepper
  if (stepperId > -1)
  {
    ESPStepperMotorServer_StepperConfiguration *stepper = this->serverConfiguration->getStepperConfiguration(stepperId);
    if (stepper)
    {
      stepper->getFlexyStepper()->emergencyStop();
    }
  }
  else
  {
    //perform complete stop on all steppers
    ESP_FlexyStepper **configuredFlexySteppers = this->serverConfiguration->getConfiguredFlexySteppers();
    for (byte i = 0; i < ESPServerMaxSteppers; i++)
    {
      if (configuredFlexySteppers[i])
      {
        configuredFlexySteppers[i]->emergencyStop();
      }
      else
      {
        break;
      }
    }
  }
}

void ESPStepperMotorServer::revokeEmergencyStop()
{
  this->emergencySwitchIsActive = false;
}

/**
 * Update the switch status register by reading all configured IO pins.
 * Returns the pin Number of the last IO pin where a change has been detected for since last the update of the register
 */
signed char ESPStepperMotorServer::updateSwitchStatusRegister()
{
  byte registerIndex = 0;
  signed char changedSwitchIndex = -1;
  signed char *allSwitchIoPins = this->serverConfiguration->allSwitchIoPins;
  volatile byte *buttonStatus = this->buttonStatus;
  //iterate over all configured position switch IO pins and read state and write to status registers
  for (int switchIndex = 0; switchIndex < ESPServerMaxSwitches; switchIndex++)
  {
    byte ioPin = allSwitchIoPins[switchIndex];
    if (ioPin > -1)
    {
      if (switchIndex > 7) //write to next register if needed
      {
        registerIndex = (byte)(ceil)((switchIndex + 1) / 8);
      }
      byte previousPinState = ((buttonStatus[registerIndex] & (1 << (switchIndex - 1))) > 0);
      byte currentPinState = digitalRead(ioPin);
      if (currentPinState == HIGH && previousPinState == LOW)
      {
        if (ESPStepperMotorServer_Logger::isDebugEnabled())
          ESPStepperMotorServer_Logger::logDebugf("Setting bit %i to high in register for switch %i with io pin %i\n", (switchIndex % 8), switchIndex, ioPin);

        bitSet(buttonStatus[registerIndex], switchIndex % 8);
        changedSwitchIndex = switchIndex;
      }
      else if (currentPinState == LOW && previousPinState == HIGH)
      {
        if (ESPStepperMotorServer_Logger::isDebugEnabled())
          ESPStepperMotorServer_Logger::logDebugf("Setting bit %i to low in register for switch %i with io pin %i\n", (switchIndex % 8), switchIndex, ioPin);

        bitClear(buttonStatus[registerIndex], switchIndex % 8);
        changedSwitchIndex = switchIndex;
      }
    }
  }
  return changedSwitchIndex;
}

void IRAM_ATTR ESPStepperMotorServer::staticPositionSwitchISR()
{
  anchor->internalSwitchISR(SWITCHTYPE_POSITION_SWITCH_BIT);
}

void IRAM_ATTR ESPStepperMotorServer::staticEmergencySwitchISR()
{
  anchor->internalEmergencySwitchISR();
}

void IRAM_ATTR ESPStepperMotorServer::staticLimitSwitchISR()
{
  anchor->internalSwitchISR(SWITCHTYPE_LIMITSWITCH_POS_BEGIN_BIT);
}

void IRAM_ATTR ESPStepperMotorServer::staticRotaryEncoderISR()
{
  anchor->internalRotaryEncoderISR();
}

void IRAM_ATTR ESPStepperMotorServer::internalEmergencySwitchISR()
{
  ESPStepperMotorServer_PositionSwitch *switchConfig;
  for (byte i = 0; i < ESPServerMaxSwitches; i++)
  {
    switchConfig = this->serverConfiguration->configuredEmergencySwitches[i];
    if (switchConfig != NULL)
    {
      byte registerIndex = 0;
      byte switchId = switchConfig->getId();
      if (switchId > 7) //write to next register if needed
        registerIndex = (byte)(ceil)((switchId + 1) / 8);

      byte pinState = digitalRead(switchConfig->getIoPinNumber());
      if (pinState)
        bitSet(this->buttonStatus[registerIndex], (switchId % 8));
      else
        bitClear(this->buttonStatus[registerIndex], (switchId % 8));

      bool isActiveHigh = switchConfig->isActiveHigh();
      bool switchIsActive = ((pinState && isActiveHigh) || (!pinState && !isActiveHigh));
      if (switchIsActive)
        this->performEmergencyStop(switchConfig->getStepperIndex());
      else
        // TODO: this might cause issues with multiple Emergency Switches connected since it will revoke the global flag
        // so if only the first switch is triggered, the second one will be checked in this loop and revoke the flag
        this->emergencySwitchIsActive = false;
    }
    else
      break;
  }
}

/**
 * ISR for general switch interrupts
 * NOTE: this ISR is not called for emergency switches (since fastest possible processing time is required and we need to avoid all these loops).
 * Look at internalEmergencySwitchISR() instead for emergency switch handling
 */
void IRAM_ATTR ESPStepperMotorServer::internalSwitchISR(byte switchType)
{
  byte changedStausSwitchId = this->updateSwitchStatusRegister();
  if (switchType == SWITCHTYPE_LIMITSWITCH_POS_BEGIN_BIT || switchType == SWITCHTYPE_LIMITSWITCH_POS_END_BIT || switchType == SWITCHTYPE_LIMITSWITCH_COMBINED_BEGIN_END_BIT)
  {
    ESPStepperMotorServer_Configuration *configuration = this->serverConfiguration;
    ESPStepperMotorServer_PositionSwitch *switchConfig = configuration->allConfiguredSwitches[changedStausSwitchId];
    if (switchConfig)
    {
      bool isActiveHigh = switchConfig->_switchType & (1 << (SWITCHTYPE_STATE_ACTIVE_HIGH_BIT - 1)); //we do not use the helper function isActiveHigh due to performance reasons
      bool inputState = digitalRead(switchConfig->_ioPinNumber);
      ESPStepperMotorServer_StepperConfiguration *stepper = configuration->getStepperConfiguration(switchConfig->_stepperIndex);
      if ((inputState && isActiveHigh) || (!inputState && !isActiveHigh))
      {
        if (stepper)
        {
          switch (switchType)
          {
          case SWITCHTYPE_LIMITSWITCH_POS_BEGIN_BIT:
            stepper->_flexyStepper->setLimitSwitchActive(ESP_FlexyStepper::LIMIT_SWITCH_BEGIN);
            break;
          case SWITCHTYPE_LIMITSWITCH_POS_END_BIT:
            stepper->_flexyStepper->setLimitSwitchActive(ESP_FlexyStepper::LIMIT_SWITCH_END);
            break;
          case SWITCHTYPE_LIMITSWITCH_COMBINED_BEGIN_END_BIT:
            stepper->_flexyStepper->setLimitSwitchActive(ESP_FlexyStepper::LIMIT_SWITCH_COMBINED_BEGIN_AND_END);
            break;
          }
        }
        if (ESPStepperMotorServer_Logger::isDebugEnabled())
          ESPStepperMotorServer_Logger::logDebugf("Limit switch '%s' has been triggered (IO pin status is %i)\n", switchConfig->getPositionName().c_str(), inputState);
      }
      else
      {
        if (stepper)
        {
          stepper->_flexyStepper->clearLimitSwitchActive();
        }
        if (ESPStepperMotorServer_Logger::isDebugEnabled())
          ESPStepperMotorServer_Logger::logDebugf("Limit switch '%s' has been released (IO pin status is %i)\n", switchConfig->getPositionName().c_str(), inputState);
      }
    }
    else
    {
      ESPStepperMotorServer_Logger::logWarningf("A IO Pin change has been detected for switch id %i which is not a limit switch, but the ISR was triggered for a switch of type limit switch. It is possible that a limit switch status change has not been detected properly\n", changedStausSwitchId);
    }
  }
}

/**
 * the ISR to handle rotary encoder related pin interrupts and trigger the stepper position change
 */
void IRAM_ATTR ESPStepperMotorServer::internalRotaryEncoderISR()
{
  ESPStepperMotorServer_Configuration *configuration = this->serverConfiguration;
  for (int i = 0; i < ESPServerMaxRotaryEncoders; i++)
  {
    ESPStepperMotorServer_RotaryEncoder *rotaryEncoder = configuration->configuredRotaryEncoders[i];
    if (rotaryEncoder != NULL)
    {
      unsigned char result = rotaryEncoder->process();
      ESPStepperMotorServer_StepperConfiguration *stepperConfig = configuration->configuredSteppers[rotaryEncoder->_stepperIndex];
      if (stepperConfig)
      {
        if (result == DIR_CW)
        {
          stepperConfig->_flexyStepper->setTargetPositionRelativeInSteps(1 * rotaryEncoder->_stepMultiplier);
        }
        else if (result == DIR_CCW)
        {
          signed long newPosition = -1L * rotaryEncoder->_stepMultiplier;
          stepperConfig->_flexyStepper->setTargetPositionRelativeInSteps(newPosition);
        }
      }
      else
      {
        ESPStepperMotorServer_Logger::logWarningf("Invalid (non config found) stepper server id %i in rotary encoder config with id %i\n", rotaryEncoder->_stepperIndex, i);
      }
    }
  }
}

// ----------------- delegator functions to ease API usage -------------------------

void ESPStepperMotorServer::setLogLevel(byte logLevel)
{
  ESPStepperMotorServer_Logger::setLogLevel(logLevel);
}

// -------------------------------------- End --------------------------------------

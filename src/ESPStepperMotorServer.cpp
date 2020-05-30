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
//    For each stepper, declare a global object outside of all functions as
//    follows:
//        FlexyStepper flexyStepper1;
//        FlexyStepper flexyStepper2;
//    and then "wrap" it in a ESPStepperMotorServer_Stepper class:
//        ESPStepperMotorServer_Stepper stepper1(*flexyStepper1);
//        ESPStepperMotorServer_Stepper stepper2(*flexyStepper2);
//    also declare the server instance here along with the required settings:
//    e.g. to start the server and enable the web based uster interface, the REST API and the serial server use:
//        ESPStepperMotorServer server(ESPServerRestApiEnabled|ESPServerWebserverEnabled|ESPServerSerialEnabled);
//    eg. to only start the web user interface and disable the rest API and serial server use:
//        ESPStepperMotorServer server(ESPServerWebserverEnabled);
//
//    In Setup(), assign pin numbers of the ESP where the step and direction inputs of the stepper driver module are connected to ESP:
//        flexyStepper1.connectToPins(10, 11); //bye stepPinNumber, byte directionPinNumber
//        flexyStepper1.connectToPins(12, 14);
//    And then add the steppers to the server and start the server
//        server.addStepper(*stepper1);
//        server.addStepper(*stepper2);
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

//due to circular dependency the follwoing two classes must be inlcuded here, rahter than in the header file (for now, maybe fix this bad api design later)
#include <ESPStepperMotorServer_CLI.h>
#include <ESPStepperMotorServer_MotionController.h>
// ---------------------------------------------------------------------------------
//                                  Setup functions
// ---------------------------------------------------------------------------------

ESPStepperMotorServer *ESPStepperMotorServer::anchor = NULL;

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
  //set Pins for encoder
  pinMode(encoder->getPinAIOPin(), INPUT);
  pinMode(encoder->getPinBIOPin(), INPUT);
  //add encoder to configuration
  if (encoderIndex > -1)
  {
    this->serverConfiguration->setRotaryEncoder(encoder, encoderIndex);
  }
  else
  {
    encoderIndex = (unsigned int)this->serverConfiguration->addRotaryEncoder(encoder);
  }
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
    sprintf(this->logString, "Either the step IO pin (%i) or direction IO (%i) pin, or both, are not set correctly. Use a valid IO Pin value between 0 and the highest available IO Pin on your ESP", stepper->getStepIoPin(), stepper->getDirectionIoPin());
    ESPStepperMotorServer_Logger::logWarning(this->logString);
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
    ESPStepperMotorServer_Logger::logWarningf("stepper configuration index %i is invalid, no entry present at this configuration index or stepper IDs do not match, removeStepper() canceled", id);
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
  //TODO: move switch pin inventory to server config
  this->configuredPositionSwitchIoPins[switchIndex] = posSwitchToAdd->getIoPinNumber();
  this->emergencySwitchIndexes[switchIndex] = ((posSwitchToAdd->getSwitchType() & ESPServerSwitchType_EmergencyStopSwitch) == ESPServerSwitchType_EmergencyStopSwitch);
  //Setup IO Pin
  this->setupPositionSwitchIOPin(posSwitchToAdd);

  ESPStepperMotorServer_Logger::logInfof("added position switch %s for IO pin %i at configuration index %i\n", this->serverConfiguration->getSwitch(switchIndex)->getPositionName().c_str(), this->serverConfiguration->getSwitch(switchIndex)->getIoPinNumber(), switchIndex);
  return switchIndex;
}

void ESPStepperMotorServer::removePositionSwitch(int positionSwitchIndex)
{
  ESPStepperMotorServer_PositionSwitch *posSwitch = this->serverConfiguration->getSwitch(positionSwitchIndex);
  if (posSwitch)
  {
    this->detachInterruptForPositionSwitch(posSwitch);
    ESPStepperMotorServer_Logger::logDebugf("removing position switch %s (idx: %i) from configured position switches\n", posSwitch->getPositionName().c_str(), positionSwitchIndex);
    this->serverConfiguration->removeSwitch(positionSwitchIndex);
    this->emergencySwitchIndexes[positionSwitchIndex] = 0;
  }
  else
  {
    ESPStepperMotorServer_Logger::logWarning("position switch index %i is invalid, no position switch present at this configuration index, removePositionSwitch() canceled\n", positionSwitchIndex);
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
    sprintf(this->logString, "rotary encoder index %i is invalid, no rotary encoder pointer present at this configuration index or rotary encoder IDs do not match, removeRotaryEncoder() canceled", id);
    ESPStepperMotorServer_Logger::logWarning(this->logString);
  }
}

// ---------------------------------------------------------------------------------
//                                  Status and Service Functions
// ---------------------------------------------------------------------------------

/**
 * get the status register value for all configured buttons
 * expects an array as buffer that will be filled with the current status registry values
 */
void ESPStepperMotorServer::getButtonStatusRegister(byte buffer[ESPServerSwitchStatusRegisterCount])
{
  for (int i = 0; i < ESPServerSwitchStatusRegisterCount; i++)
  {
    buffer[i] = this->buttonStatus[i];
  }
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
    String binary = String(this->buttonStatus[i], BIN);
    for (int i = 0; i < 8 - binary.length(); i++)
    {
      binaryPadded += "0";
    }
    binaryPadded += binary;
    positionSwitchRegister["status"] = binaryPadded;
  }

  JsonArray switches = root.createNestedArray("positionSwitches");
  for (int i = 0; i < ESPServerMaxSwitches; i++)
  {
    if (this->serverConfiguration->getSwitch(i) && this->serverConfiguration->getSwitch(i)->getIoPinNumber() != ESPServerPositionSwitchUnsetPinNumber)
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
  byte buttonRegisterIndex = (byte)(ceil)((positionSwitchIndex + 1) / 8);
  ESPStepperMotorServer_PositionSwitch *posSwitch = this->serverConfiguration->getSwitch(positionSwitchIndex);
  if (posSwitch)
  {
    byte bitVal = (1 << positionSwitchIndex % 8);
    byte buttonState = ((this->buttonStatus[buttonRegisterIndex] & bitVal) == bitVal);

    if (posSwitch->getIoPinNumber() != ESPServerPositionSwitchUnsetPinNumber)
    {
      if ((posSwitch->getSwitchType() & ESPServerSwitchType_ActiveHigh) == ESPServerSwitchType_ActiveHigh)
      {
        return (buttonState) ? 1 : 0;
      }
      else
      {
        return (buttonState) ? 0 : 1;
      }
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
      sprintf(this->logString, "File: %s (%i) %ld", file.name(), file.size(), file.getLastWrite());
      ESPStepperMotorServer_Logger::logInfo(this->logString);
      file = root.openNextFile();
    }
    root.close();
  }
}

int ESPStepperMotorServer::getSPIFFSFreeSpace()
{
  return ((int)SPIFFS.totalBytes() - (int)SPIFFS.usedBytes());
}

void ESPStepperMotorServer::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    sprintf(this->logString, "ws[%s][%u] connect\n", server->url(), client->id());
    ESPStepperMotorServer_Logger::logInfo(this->logString);
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    sprintf(this->logString, "ws[%s][%i] disconnect: %i\n", server->url(), client->id(), client->id());
    ESPStepperMotorServer_Logger::logInfo(this->logString);
  }
  else if (type == WS_EVT_ERROR)
  {
    sprintf(this->logString, "ws[%s][%u] error(%i): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
    ESPStepperMotorServer_Logger::logWarning(this->logString);
  }
  else if (type == WS_EVT_PONG)
  {
    sprintf(this->logString, "ws[%s][%i] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
    ESPStepperMotorServer_Logger::logInfo(this->logString);
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
        char buff[3];
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
        char buff[3];
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
      if (checkIfGuiExistsInSpiffs())
      {
        this->registerWebInterfaceUrls();
      }
    }
    if (isRestApiEnabled)
    {
      this->registerRestApiEndpoints();
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

bool ESPStepperMotorServer::checkIfGuiExistsInSpiffs()
{
  ESPStepperMotorServer_Logger::logDebug("Checking if web UI is installed in SPIFFS");
  bool uiComplete = true;
  const char *notPresent = "The file %s could not be found on SPIFFS";
  const char *files[7] = {this->webUiIndexFile, this->webUiJsFile, this->webUiLogoFile, this->webUiFaviconFile, this->webUiEncoderGraphic, this->webUiStepperGraphic, this->webUiSwitchGraphic};

  for (int i = 0; i < 4; i++)
  {
    if (!SPIFFS.exists(files[i]))
    {
      sprintf(this->logString, notPresent, files[i]);
      ESPStepperMotorServer_Logger::logInfo(this->logString);
      if (this->serverConfiguration->wifiMode == ESPServerWifiModeClient && WiFi.isConnected())
      {
        char downloadUrl[200];
        strcpy(downloadUrl, webUiRepositoryBasePath);
        strcat(downloadUrl, files[i]);
        if (!this->downloadFileToSpiffs(downloadUrl, files[i]))
        {
          uiComplete = false;
        }
      }
      else
      {
        uiComplete = false;
      }
    }
  }

  if (!uiComplete && this->serverConfiguration->wifiMode == ESPServerWifiModeAccessPoint)
  {
    ESPStepperMotorServer_Logger::logWarning("The UI does not seem to be installed completely on SPIFFS. Automatic download failed since the server is in Access Point mode and not connected to the internet");
    ESPStepperMotorServer_Logger::logWarning("Start the server in wifi client (STA) mode to enable automatic download of the web interface files to SPIFFS");
  }
  else if (uiComplete)
  {
    ESPStepperMotorServer_Logger::logDebug("Check completed successfully");
  }
  else if (!uiComplete)
  {
    ESPStepperMotorServer_Logger::logDebug("Check failed, one or more UI files are missing and could not be downloaded automatically");
  }
  return uiComplete;
}

HTTPClient http;
// Perform an HTTP GET request to a remote page to download a file to SPIFFS
bool ESPStepperMotorServer::downloadFileToSpiffs(const char *url, const char *targetPath)
{
  sprintf(this->logString, "downloading %s from %s", targetPath, url);
  ESPStepperMotorServer_Logger::logDebug(this->logString);

  if (http.begin(url))
  {
    int httpCode = http.GET();
    sprintf(this->logString, "server responded with %i", httpCode);
    ESPStepperMotorServer_Logger::logDebug(this->logString);

    //////////////////
    // get length of document (is -1 when Server sends no Content-Length header)
    int len = http.getSize();
    uint8_t buff[128] = {0};

    sprintf(this->logString, "starting download stream for file size %i", len);
    ESPStepperMotorServer_Logger::logDebug(this->logString);

    WiFiClient *stream = &http.getStream();

    ESPStepperMotorServer_Logger::logDebug("opening file for writing");
    File f = SPIFFS.open(targetPath, "w+");

    // read all data from server
    while (http.connected() && (len > 0 || len == -1))
    {
      // get available data size
      size_t size = stream->available();

      sprintf(this->logString, "%i bytes available to read from stream", size);
      ESPStepperMotorServer_Logger::logDebug(this->logString);

      if (size)
      {
        // read up to 128 byte
        int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
        // write it to file
        for (int i = 0; i < c; i++)
        {
          f.write(buff[i]);
        }
        if (len > 0)
        {
          len -= c;
        }
      }
      delay(1);
    }
    ESPStepperMotorServer_Logger::logDebug("Closing file handler");
    f.close();
    sprintf(this->logString, "Download of %s completed", targetPath);
    ESPStepperMotorServer_Logger::logInfo(this->logString);
    http.end(); //Close connection
  }

  return SPIFFS.exists(targetPath);
}

void ESPStepperMotorServer::getStatusAsJsonString(String &statusString)
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
  serializeJson(root, statusString);
}

void ESPStepperMotorServer::registerRestApiEndpoints()
{
  this->restApiHandler->registerRestEndpoints(this->httpServer);
}

void ESPStepperMotorServer::registerWebInterfaceUrls()
{
  httpServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(SPIFFS, this->webUiIndexFile);
  });
  httpServer->on(this->webUiIndexFile, HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(SPIFFS, this->webUiIndexFile);
  });
  httpServer->on(this->webUiFaviconFile, HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiFaviconFile, "image/x-icon");
    //response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer->on(this->defaultConfigurationFilename, HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->defaultConfigurationFilename, "application/json", true);
    request->send(response);
  });
  httpServer->on("/js/app.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiJsFile, "text/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer->on(this->webUiJsFile, HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiJsFile, "text/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  // register image paths with caching header present
  httpServer->on(this->webUiLogoFile, HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiLogoFile, "image/svg+xml");
    response->addHeader("Cache-Control", "max-age=36000, public");
    request->send(response);
  });
  httpServer->on(this->webUiStepperGraphic, HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiStepperGraphic, "image/svg+xml");
    response->addHeader("Cache-Control", "max-age=36000, public");
    request->send(response);
  });
  httpServer->on(this->webUiEncoderGraphic, HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiEncoderGraphic, "image/svg+xml");
    response->addHeader("Cache-Control", "max-age=36000, public");
    request->send(response);
  });
  httpServer->on(this->webUiSwitchGraphic, HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiSwitchGraphic, "image/svg+xml");
    response->addHeader("Cache-Control", "max-age=36000, public");
    request->send(response);
  });
}

// ---------------------------------------------------------------------------------
//             helper functions for stepper communication
// ---------------------------------------------------------------------------------

ESPStepperMotorServer_StepperConfiguration *ESPStepperMotorServer::getConfiguredStepper(byte index)
{
  return this->serverConfiguration->getStepperConfiguration(index);
}

ESPStepperMotorServer_PositionSwitch *ESPStepperMotorServer::getConfiguredSwitch(byte index)
{
  if (index < 0 || index >= ESPServerMaxSwitches)
  {
    sprintf(this->logString, "index %i for requested switch is out of allowed range, must be between 0 and %i. Will retrun first entry instead", index, ESPServerMaxSwitches);
    ESPStepperMotorServer_Logger::logWarning(this->logString);
    index = 0;
  }
  return this->serverConfiguration->getSwitch(index);
}

ESPStepperMotorServer_RotaryEncoder *ESPStepperMotorServer::getConfiguredRotaryEncoder(byte index)
{
  return this->serverConfiguration->getRotaryEncoder(index);
}

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
    sprintf(this->logString, "SSID: %s", this->wifiClientSsid);
    ESPStepperMotorServer_Logger::logInfo(logString);

    sprintf(this->logString, "IP address: %s", WiFi.localIP().toString().c_str());
    ESPStepperMotorServer_Logger::logInfo(logString);

    sprintf(this->logString, "Strength: %i dBm", WiFi.RSSI()); //Received Signal Strength Indicator
    ESPStepperMotorServer_Logger::logInfo(logString);
  }
  else if (this->serverConfiguration->wifiMode == ESPServerWifiModeAccessPoint)
  {
    ESPStepperMotorServer_Logger::logInfo("WiFi status: access point started");
    sprintf(this->logString, "SSID: %s", this->serverConfiguration->apName);
    ESPStepperMotorServer_Logger::logInfo(logString);

    sprintf(this->logString, "IP Address: %s", WiFi.softAPIP().toString().c_str());
    ESPStepperMotorServer_Logger::logInfo(logString);
  }
  else
  {
    ESPStepperMotorServer_Logger::logInfo("WiFi is disabled");
  }
}

void ESPStepperMotorServer::setWifiCredentials(const char *ssid, const char *pwd)
{
  this->wifiClientSsid = ssid;
  this->wifiPassword = pwd;
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

  if ((this->wifiClientSsid != NULL) && (this->wifiClientSsid[0] == '\0'))
  {
    ESPStepperMotorServer_Logger::logWarning("No SSID has been configured to connect to. Connection to existing WiFi network aborted");
    return;
  }

  ESPStepperMotorServer_Logger::logInfof("Trying to connect to WiFi with SSID '%s' ...", this->wifiClientSsid);
  WiFi.begin(this->wifiClientSsid, this->wifiPassword);
  int timeoutCounter = this->wifiClientConnectionTimeoutSeconds * 2;
  while (WiFi.status() != WL_CONNECTED && timeoutCounter > 0)
  {
    delay(500); //Do not change this value unless also changing the formula in the declaration of the timeoutCounter integer above
    ESPStepperMotorServer_Logger::logInfo(".", false, true);
    timeoutCounter--;
  }
  ESPStepperMotorServer_Logger::logInfo("\n", false, true);

  if (timeoutCounter > 0)
  {
    ESPStepperMotorServer_Logger::logInfof("Connected to network with IP address %s\n", WiFi.localIP().toString().c_str());
  }
  else
  {
    ESPStepperMotorServer_Logger::logWarningf("Connection to WiFi network with SSID '%s' failed with timeout\n", this->wifiClientSsid);
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
void ESPStepperMotorServer::setupPositionSwitchIOPin(ESPStepperMotorServer_PositionSwitch *posSwitchToAdd)
{
  if (posSwitchToAdd->getSwitchType() & ESPServerSwitchType_ActiveHigh)
  {
    pinMode(posSwitchToAdd->getIoPinNumber(), INPUT);
  }
  else
  {
    pinMode(posSwitchToAdd->getIoPinNumber(), INPUT_PULLUP);
  }
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
      ESPStepperMotorServer_Logger::logDebugf("Attaching interrupt service routine for position switch '%s' on IO pin %i\n", posSwitch->getPositionName().c_str(), posSwitch->getIoPinNumber());
      _BV(irqNum); // clear potentially pending interrupts
      attachInterrupt(irqNum, staticPositionSwitchISR, CHANGE);
    }
  }

  for (int i = 0; i < ESPServerMaxRotaryEncoders; i++)
  {
    ESPStepperMotorServer_RotaryEncoder *rotaryEncoder = this->serverConfiguration->getRotaryEncoder(i);
    if (rotaryEncoder != NULL)
    {
      //we do a loop here to save some pgrogram memory, could also externalize code block in another function
      const unsigned char pins[2] = {rotaryEncoder->getPinAIOPin(), rotaryEncoder->getPinBIOPin()};
      for (int i = 0; i < 2; i++)
      {
        char irqNum = digitalPinToInterrupt(pins[i]);
        if (irqNum == NOT_AN_INTERRUPT)
        {
          ESPStepperMotorServer_Logger::logWarningf("Failed to determine IRQ# for given IO pin %i, thus setting up of interrupt for the rotary encoder failed for pin %s\n", pins[i], rotaryEncoder->getDisplayName().c_str());
        }

        ESPStepperMotorServer_Logger::logDebugf("attaching interrupt service routine for rotary encoder '%s' on IO pin %i\n", rotaryEncoder->getDisplayName().c_str(), pins[i]);
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

void ESPStepperMotorServer::internalPositionSwitchISR()
{
  byte registerIndex = 0;
  byte pinNumber = 0;
  //iterate over all configured position switch IO pins and read state and write to status registers
  for (int switchIndex = 0; switchIndex < ESPServerMaxSwitches; switchIndex++)
  {
    if (this->serverConfiguration->getSwitch(switchIndex))
    {
      if (switchIndex > 7)
      {
        registerIndex = (byte)(ceil)((switchIndex + 1) / 8);
      }
      pinNumber = this->configuredPositionSwitchIoPins[switchIndex];
      boolean isEmergencySwitch = (this->emergencySwitchIndexes[switchIndex] == 1);
      byte pinStateType = 0;
      if (isEmergencySwitch)
      {
        pinStateType = ((this->serverConfiguration->getSwitch(switchIndex)->getSwitchType() & ESPServerSwitchType_ActiveHigh) == ESPServerSwitchType_ActiveHigh) ? ESPServerSwitchType_ActiveHigh : ESPServerSwitchType_ActiveLow;
      }
      if (pinNumber != ESPServerPositionSwitchUnsetPinNumber && digitalRead(pinNumber))
      {
        bitSet(this->buttonStatus[registerIndex], switchIndex % 8);
        if (isEmergencySwitch && pinStateType == ESPServerSwitchType_ActiveHigh)
        {
          this->emergencySwitchIsActive = true;
        }
      }
      else
      {
        bitClear(this->buttonStatus[registerIndex], switchIndex % 8);
        if (isEmergencySwitch && pinStateType == ESPServerSwitchType_ActiveLow)
        {
          this->emergencySwitchIsActive = true;
        }
      }
    }
  }
  this->positionSwitchUpdateAvailable = true;
}

void ESPStepperMotorServer::performEmergencyStop()
{
  this->motionControllerHandler->stop();
  for (byte stepperIndex = 0; stepperIndex < ESPServerMaxSteppers; stepperIndex++)
  {
    ESPStepperMotorServer_StepperConfiguration *stepper = this->serverConfiguration->getStepperConfiguration(stepperIndex);
    if (stepper)
    {
      // TODO: check back on the status of this Pull request and switch to emergencyStop function once done:
      // https://github.com/Stan-Reifel/FlexyStepper/pull/4
      FlexyStepper *flexyStepper = stepper->getFlexyStepper();
      flexyStepper->setAccelerationInStepsPerSecondPerSecond(9999);
      flexyStepper->setTargetPositionToStop();
    }
  }
}

void ESPStepperMotorServer::revokeEmergencyStop(){
  this->motionControllerHandler->start();
}


void ESPStepperMotorServer::internalRotaryEncoderISR()
{
  for (int i = 0; i < ESPServerMaxRotaryEncoders; i++)
  {
    ESPStepperMotorServer_RotaryEncoder *rotaryEncoder = this->serverConfiguration->getRotaryEncoder(i);
    if (rotaryEncoder != NULL)
    {
      unsigned char result = rotaryEncoder->process();
      ESPStepperMotorServer_StepperConfiguration *stepperConfig = this->serverConfiguration->getStepperConfiguration(rotaryEncoder->getStepperIndex());
      if (stepperConfig)
      {
        if (result == DIR_CW)
        {
          stepperConfig->getFlexyStepper()->setTargetPositionRelativeInSteps(1 * rotaryEncoder->getStepMultiplier());
        }
        else if (result == DIR_CCW)
        {
          stepperConfig->getFlexyStepper()->setTargetPositionRelativeInSteps(-1 * rotaryEncoder->getStepMultiplier());
        }
      }
      else
      {
        ESPStepperMotorServer_Logger::logWarningf("Invalid (non config found) stepper server id %i in rotary encoder config with id %i.", rotaryEncoder->getStepperIndex(), i);
      }
    }
  }
}

void ESPStepperMotorServer::staticPositionSwitchISR()
{
  anchor->internalPositionSwitchISR();
}

void ESPStepperMotorServer::staticRotaryEncoderISR()
{
  anchor->internalRotaryEncoderISR();
}

// ----------------- delegator functions to ease API usage -------------------------

void ESPStepperMotorServer::setLogLevel(byte logLevel)
{
  ESPStepperMotorServer_Logger::setLogLevel(logLevel);
}

// -------------------------------------- End --------------------------------------

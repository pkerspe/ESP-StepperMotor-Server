
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

#include "ESPStepperMotorServer.h"
// ---------------------------------------------------------------------------------
//                                  Setup functions
// ---------------------------------------------------------------------------------

ESPStepperMotorServer *ESPStepperMotorServer::anchor = NULL;

//
// constructor for the stepper server class
//
ESPStepperMotorServer::ESPStepperMotorServer(byte serverMode)
{
  this->httpPortNumber = 80;
  this->accessPointSSID = "ESP-StepperMotor-Server";
  this->accessPointPassword = NULL;
  this->wifiMode = ESPServerWifiModeAccessPoint;

  ESPStepperMotorServer_Logger::setLogLevel(ESPServerLogLevel_INFO);
  this->enabledServices = serverMode;
  if ((this->enabledServices & ESPServerWebserverEnabled) == ESPServerWebserverEnabled)
  {
    this->isWebserverEnabled = true;
  }
  if ((this->enabledServices & ESPServerRestApiEnabled) == ESPServerRestApiEnabled)
  {
    this->isRestApiEnabled = true;
    this->restApiHandler = new ESPStepperMotorServer_RestAPI(this);
  }

  if (ESPStepperMotorServer::anchor != NULL)
  {
    ESPStepperMotorServer_Logger::logWarning("ESPStepperMotorServer must be used as a singleton, do not instanciate more than one server in your project");
  }
  else
  {
    ESPStepperMotorServer::anchor = this;
  }
}

void ESPStepperMotorServer::setHttpPort(int portNumber)
{
  this->httpPortNumber = portNumber;
}

int ESPStepperMotorServer::addStepper(ESPStepperMotorServer_StepperConfiguration *stepper)
{
  if (this->configuredStepperIndex >= ESPServerMaxSteppers)
  {
    sprintf(this->logString, "The maximum amount of steppers (%i) that can be configured has been reached, no more steppers can be added", ESPServerMaxSteppers);
    ESPStepperMotorServer_Logger::logWarning(this->logString);
    ESPStepperMotorServer_Logger::logWarning("The value can only be increased during compile time, by modifying the value of ESPServerMaxSteppers in ESPStepperMotorServer.h");
    return -1;
  }
  if (stepper->getStepIoPin() == ESPStepperMotorServer_StepperConfiguration::ESPServerStepperUnsetIoPinNumber ||
      stepper->getDirectionIoPin() == ESPStepperMotorServer_StepperConfiguration::ESPServerStepperUnsetIoPinNumber)
  {
    sprintf(this->logString, "Either the step IO pin (%i) or direction IO (%i) pin, or both, are not set correctly. Use a valid IO Pin value between 0 and the highest available IO Pin on your ESP", stepper->getStepIoPin(), stepper->getDirectionIoPin());
    ESPStepperMotorServer_Logger::logWarning(this->logString);
    return -1;
  }
  stepper->setId(this->configuredStepperIndex);
  //set IO Pins for stepper
  pinMode(stepper->getDirectionIoPin(), OUTPUT);
  digitalWrite(stepper->getDirectionIoPin(), LOW);
  pinMode(stepper->getStepIoPin(), OUTPUT);
  digitalWrite(stepper->getStepIoPin(), LOW);
  //add stepper to configuration
  this->configuredSteppers[this->configuredStepperIndex] = stepper;
  this->configuredStepperIndex++;
  return this->configuredStepperIndex - 1;
}

void ESPStepperMotorServer::removeStepper(int id)
{
  ESPStepperMotorServer_StepperConfiguration *stepperToRemove = this->configuredSteppers[id];
  if (stepperToRemove != NULL && stepperToRemove->getId() == id)
  {
    this->removeStepper(stepperToRemove);
  }
  else
  {
    sprintf(this->logString, "stepper configuration index %i is invalid, no stepper pointer present at this configuration index or stepper IDs do not match, removeStepper() canceled", id);
    ESPStepperMotorServer_Logger::logWarning(this->logString);
  }
}

void ESPStepperMotorServer::removeStepper(ESPStepperMotorServer_StepperConfiguration *stepper)
{
  if (stepper != NULL && this->configuredSteppers[stepper->getId()]->getId() == stepper->getId())
  {
    // check if any switch is configured for the stepper to be removed, if so remove switches first
    for (int i = 0; i < ESPServerMaxSwitches; i++)
    {
      if (this->configuredPositionSwitches[i].getStepperIndex() == stepper->getId())
      {
        this->removePositionSwitch(i);
      }
    }
    //now remove the stepper configuration itself
    this->configuredSteppers[stepper->getId()] = NULL;
  }
  else
  {
    ESPStepperMotorServer_Logger::logInfo("Invalid stepper given. The ID does not match the configured stepper. RemoveStepper failed");
  }
}

int ESPStepperMotorServer::addPositionSwitch(byte stepperIndex, byte ioPinNumber, byte switchType, String positionName, long switchPosition)
{
  ESPStepperMotorServer_PositionSwitch positionSwitchToAdd(ioPinNumber, stepperIndex, switchType, positionName);
  positionSwitchToAdd.setSwitchPosition(switchPosition);
  return this->addPositionSwitch(positionSwitchToAdd);
}

int ESPStepperMotorServer::addPositionSwitch(ESPStepperMotorServer_PositionSwitch posSwitchToAdd, int switchIndex)
{
  if (posSwitchToAdd.getStepperIndex() > this->configuredStepperIndex)
  {
    sprintf(this->logString, "invalid stepperIndex value given. The number of configured steppers is %i but index value of %i was given in addPositionSwitch() call.", this->configuredStepperIndex, posSwitchToAdd.getStepperIndex());
    ESPStepperMotorServer_Logger::logWarning(this->logString);
    ESPStepperMotorServer_Logger::logWarning("the position switch has not been added");
    return -1;
  }
  if (posSwitchToAdd.getPositionName().length() > ESPSMS_Stepper_DisplayName_MaxLength)
  {
    char logString[160];
    sprintf(logString, "ESPStepperMotorServer::addPositionSwitch: The display name for the position switch is to long. Max length is %i characters. Name will be trimmed", ESPStepperMotorServer_SwitchDisplayName_MaxLength);
    ESPStepperMotorServer_Logger::logWarning(logString);
    posSwitchToAdd.setPositionName(posSwitchToAdd.getPositionName().substring(0, ESPSMS_Stepper_DisplayName_MaxLength));
  }
  if (switchIndex == -1)
  {
    //check if we have a blank configuration slot before the actual index (due to possible removal of previously configured position switches that might have been removed in the meantime)
    switchIndex = this->configuredPositionSwitchIndex;
    for (int i = 0; i < this->configuredPositionSwitchIndex; i++)
    {
      if (this->configuredPositionSwitches[i].getIoPinNumber() == ESPServerPositionSwitchUnsetPinNumber)
      {
        switchIndex = i;
        break;
      }
    }
  }

  this->configuredPositionSwitches[switchIndex] = posSwitchToAdd;
  this->configuredPositionSwitchIoPins[switchIndex] = posSwitchToAdd.getIoPinNumber();
  this->emergencySwitchIndexes[switchIndex] = ((posSwitchToAdd.getSwitchType() & ESPServerSwitchType_EmergencyStopSwitch) == ESPServerSwitchType_EmergencyStopSwitch);
  //Setup IO Pin
  this->setupPositionSwitchIOPin(&posSwitchToAdd);

  sprintf(this->logString, "added position switch %s for IO pin %i at configuration index %i", this->configuredPositionSwitches[switchIndex].getPositionName().c_str(), this->configuredPositionSwitches[switchIndex].getIoPinNumber(), switchIndex);
  ESPStepperMotorServer_Logger::logInfo(this->logString);

  if (this->configuredPositionSwitchIndex == switchIndex)
  {
    this->configuredPositionSwitchIndex++;
  }
  return switchIndex;
}

void ESPStepperMotorServer::removePositionSwitch(int positionSwitchIndex)
{
  ESPStepperMotorServer_PositionSwitch *posSwitch = &this->configuredPositionSwitches[positionSwitchIndex];
  if (posSwitch->getIoPinNumber() != ESPServerPositionSwitchUnsetPinNumber)
  {
    this->removePositionSwitch(posSwitch);
  }
  else
  {
    sprintf(this->logString, "position switch index %i is invalid, no position switch present at this configuration index, removePositionSwitch() canceled", positionSwitchIndex);
    ESPStepperMotorServer_Logger::logWarning(this->logString);
  }
}

void ESPStepperMotorServer::removePositionSwitch(ESPStepperMotorServer_PositionSwitch *posSwitchToRemove)
{
  if (posSwitchToRemove->getIoPinNumber() != ESPServerPositionSwitchUnsetPinNumber)
  {
    for (int i = 0; i < ESPServerMaxSwitches; i++)
    {
      if (this->configuredPositionSwitches[i].getIoPinNumber() == posSwitchToRemove->getIoPinNumber())
      {
        ESPStepperMotorServer_PositionSwitch blankPosSwitch;
        this->detachInterruptForPositionSwitch(posSwitchToRemove);
        sprintf(this->logString, "removed position switch %s (idx: %i) from configured position switches", posSwitchToRemove->getPositionName().c_str(), i);
        ESPStepperMotorServer_Logger::logDebug(this->logString);
        this->configuredPositionSwitches[i] = blankPosSwitch;
        this->configuredPositionSwitchIoPins[i] = ESPServerPositionSwitchUnsetPinNumber;
        this->emergencySwitchIndexes[i] = 0;
        return;
      }
    }
  }
  else
  {
    ESPStepperMotorServer_Logger::logInfo("Invalid position switch given, IO pin was not set");
  }
}

// ---------------------------------------------------------------------------------
//                     general service control functions
// ---------------------------------------------------------------------------------

void ESPStepperMotorServer::start()
{
  sprintf(logString, "Starting ESP-StepperMotor-Server (v. %s)", this->version);
  ESPStepperMotorServer_Logger::logInfo(logString);

  if (this->wifiMode == ESPServerWifiModeAccessPoint)
  {
    this->startAccessPoint();
    if (ESPStepperMotorServer_Logger::getLogLevel() >= ESPServerLogLevel_DEBUG)
    {
      this->printWifiStatus();
    }
  }
  else if (this->wifiMode == ESPServerWifiModeClient)
  {
    this->connectToWifiNetwork();
    if (ESPStepperMotorServer_Logger::getLogLevel() >= ESPServerLogLevel_DEBUG)
    {
      this->printWifiStatus();
    }
  }
  else if (this->wifiMode == ESPServerWifiModeDisabled)
  {
    ESPStepperMotorServer_Logger::logInfo("WiFi mode is disabled, will only serial control inerface");
  }

  this->startWebserver();

  this->attachAllInterrupts();

  this->isServerStarted = true;
}

void ESPStepperMotorServer::stop()
{
  ESPStepperMotorServer_Logger::logInfo("Stopping ESP-StepperMotor-Server");
  this->detachAllInterrupts();
  this->isServerStarted = false;
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
  for (int i = 0; i < this->configuredPositionSwitchIndex; i++)
  {
    if (this->configuredPositionSwitches[i].getIoPinNumber() != ESPServerPositionSwitchUnsetPinNumber)
    {
      JsonObject positionSwitch = switches.createNestedObject();
      positionSwitch["id"] = i;
      positionSwitch["name"] = this->configuredPositionSwitches[i].getPositionName();
      positionSwitch["ioPin"] = this->configuredPositionSwitches[i].getIoPinNumber();
      positionSwitch["position"] = this->configuredPositionSwitches[i].getSwitchPosition();
      positionSwitch["stepperId"] = this->configuredPositionSwitches[i].getStepperIndex();
      positionSwitch["active"] = this->getPositionSwitchStatus(i);

      JsonObject positionSwichType = positionSwitch.createNestedObject("type");
      if ((this->configuredPositionSwitches[i].getSwitchType() & ESPServerSwitchType_ActiveHigh) == ESPServerSwitchType_ActiveHigh)
      {
        positionSwichType["pinMode"] = "Active High";
      }
      else
      {
        positionSwichType["pinMode"] = "Active Low";
      }
      if ((this->configuredPositionSwitches[i].getSwitchType() & ESPServerSwitchType_HomingSwitchBegin) == ESPServerSwitchType_HomingSwitchBegin)
      {
        positionSwichType["switchType"] = "Homing switch (start-position)";
      }
      else if ((this->configuredPositionSwitches[i].getSwitchType() & ESPServerSwitchType_HomingSwitchEnd) == ESPServerSwitchType_HomingSwitchEnd)
      {
        positionSwichType["switchType"] = "Homing switch (end-position)";
      }
      else if ((this->configuredPositionSwitches[i].getSwitchType() & ESPServerSwitchType_GeneralPositionSwitch) == ESPServerSwitchType_GeneralPositionSwitch)
      {
        positionSwichType["switchType"] = "General position switch";
      }
      else if ((this->configuredPositionSwitches[i].getSwitchType() & ESPServerSwitchType_EmergencyStopSwitch) == ESPServerSwitchType_EmergencyStopSwitch)
      {
        positionSwichType["switchType"] = "Emergency shut down switch";
      }
    }
  }

  serializeJson(root, Serial);
  Serial.println();
}

/**
 * checks if the configured position switch at the given configuration index is triggere (active) or not
 * This function takes the configured type ESPServerSwitchType_ActiveHigh or ESPServerSwitchType_ActiveLow into account when determining the current active state
 * e.g. if a switch is configured to be ESPServerSwitchType_ActiveLow the function will return 1 when the switch is triggered (low signal on IO pin).
 * For a switch that is configured ESPServerSwitchType_ActiveHigh on the other side, the function will return 0 when a low signal is on the IO pin, and 1 when a high signal is present
 */
byte ESPStepperMotorServer::getPositionSwitchStatus(int positionSwitchIndex)
{
  byte buttonRegisterIndex = (byte)(ceil)((positionSwitchIndex + 1) / 8);
  ESPStepperMotorServer_PositionSwitch posSwitch = this->configuredPositionSwitches[positionSwitchIndex];
  byte bitVal = (1 << positionSwitchIndex % 8);
  byte buttonState = ((this->buttonStatus[buttonRegisterIndex] & bitVal) == bitVal);

  if (posSwitch.getIoPinNumber() != ESPServerPositionSwitchUnsetPinNumber)
  {
    if ((posSwitch.getSwitchType() & ESPServerSwitchType_ActiveHigh) == ESPServerSwitchType_ActiveHigh)
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
    ESPStepperMotorServer_Logger::logDebug("SPIFFS started");
    printSPIFFSStats();
  }
  else
  {
    ESPStepperMotorServer_Logger::logWarning("Unable to activate SPIFFS. Files for WebInterface cannot be loaded");
  }
}

void ESPStepperMotorServer::printSPIFFSStats()
{
  ESPStepperMotorServer_Logger::logDebug("SPIFFS stats:");
  ESPStepperMotorServer_Logger::logDebug("Total bytes: ", false);
  ESPStepperMotorServer_Logger::logDebug(String((int)SPIFFS.totalBytes(), DEC), true, true);
  ESPStepperMotorServer_Logger::logDebug("bytes used in SPIFFS: ", false);
  ESPStepperMotorServer_Logger::logDebug(String((int)SPIFFS.usedBytes(), DEC), true, true);
  ESPStepperMotorServer_Logger::logDebug("bytes available: ", false);
  ESPStepperMotorServer_Logger::logDebug(String(getSPIFFSFreeSpace(), DEC), true, true);
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
    startSPIFFS();
    printSPIFFSRootFolderContents();

    httpServer = new AsyncWebServer(this->httpPortNumber);

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
      if (this->wifiMode == ESPServerWifiModeClient && WiFi.isConnected())
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

  if (!uiComplete && this->wifiMode == ESPServerWifiModeAccessPoint)
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
  wifiStatus["mode"] = (this->wifiMode == ESPServerWifiModeAccessPoint) ? "ap" : "client";
  wifiStatus["ip"] = (this->wifiMode == ESPServerWifiModeAccessPoint) ? WiFi.dnsIP().toString() : WiFi.localIP().toString();

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
  //httpServer->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  httpServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(SPIFFS, this->webUiIndexFile);
  });
  httpServer->on("/index.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(SPIFFS, this->webUiIndexFile);
  });
  httpServer->on("/favicon.ico", HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiFaviconFile, "image/x-icon");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer->on("/js/app.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiJsFile, "text/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer->on("/js/app.js.gz", HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiJsFile, "text/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer->on(this->webUiLogoFile, HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(SPIFFS, this->webUiLogoFile);
  });
  httpServer->on(this->webUiStepperGraphic, HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(SPIFFS, this->webUiStepperGraphic);
  });
  httpServer->on(this->webUiEncoderGraphic, HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(SPIFFS, this->webUiEncoderGraphic);
  });
  httpServer->on(this->webUiSwitchGraphic, HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(SPIFFS, this->webUiSwitchGraphic);
  });
  httpServer->on(this->webUiEncoderGraphic, HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(SPIFFS, this->webUiEncoderGraphic);
  });
}

// ---------------------------------------------------------------------------------
//             helper functions for stepper communication
// ---------------------------------------------------------------------------------

ESPStepperMotorServer_StepperConfiguration *ESPStepperMotorServer::getConfiguredStepper(byte index)
{
  if (index < ESPServerMaxSteppers)
  {
    return this->configuredSteppers[index];
  }
  return NULL;
}

ESPStepperMotorServer_PositionSwitch *ESPStepperMotorServer::getConfiguredSwitch(byte index)
{
  if (index < 0 || index >= ESPServerMaxSwitches)
  {
    sprintf(this->logString, "index %i for requsted switch is out of allowed range, must be between 0 and %i. Will retrun first entry instead", index, ESPServerMaxSwitches);
    ESPStepperMotorServer_Logger::logWarning(this->logString);
    index = 0;
  }
  return &this->configuredPositionSwitches[index];
}

bool ESPStepperMotorServer::isIoPinUsed(int pinToCheck)
{
  //check stepper configurations
  for (int i = 0; i < ESPServerMaxSteppers; i++)
  {
    if (this->configuredSteppers[i] != NULL && (this->configuredSteppers[i]->getDirectionIoPin() == pinToCheck || this->configuredSteppers[i]->getStepIoPin() == pinToCheck))
    {
      return true;
    }
  }
  //check switch configurations
  for (int i = 0; i < ESPServerMaxSwitches; i++)
  {
    if (this->configuredPositionSwitchIoPins[i] == pinToCheck)
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
  this->accessPointSSID = accessPointSSID;
}

void ESPStepperMotorServer::setAccessPointPassword(const char *accessPointPassword)
{
  this->accessPointPassword = accessPointPassword;
}

void ESPStepperMotorServer::setWifiMode(byte wifiMode)
{
  switch (wifiMode)
  {
  case ESPServerWifiModeAccessPoint:
    this->wifiMode = ESPServerWifiModeAccessPoint;
    Serial.println("");
    break;
  case ESPServerWifiModeClient:
    this->wifiMode = ESPServerWifiModeClient;
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

  if (this->wifiMode == ESPServerWifiModeClient)
  {
    ESPStepperMotorServer_Logger::logInfo("WiFi status: server acts as wifi client in existing network with DHCP");
    sprintf(this->logString, "SSID: %s", this->wifiClientSsid);
    ESPStepperMotorServer_Logger::logInfo(logString);

    sprintf(this->logString, "IP address: %s", WiFi.localIP().toString().c_str());
    ESPStepperMotorServer_Logger::logInfo(logString);

    sprintf(this->logString, "Strength: %i dBm", WiFi.RSSI()); //Received Signal Strength Indicator
    ESPStepperMotorServer_Logger::logInfo(logString);
  }
  else if (this->wifiMode == ESPServerWifiModeAccessPoint)
  {
    ESPStepperMotorServer_Logger::logInfo("WiFi status: access point started");
    sprintf(this->logString, "SSID: %s", this->accessPointSSID);
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
  WiFi.softAP(this->accessPointSSID, this->accessPointPassword);
  sprintf(this->logString, "Started Access Point with name %s and IP %s", this->accessPointSSID, WiFi.softAPIP().toString().c_str());
  ESPStepperMotorServer_Logger::logInfo(this->logString);
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
    ESPStepperMotorServer_Logger::logWarning("No ssid has been configured to connect to. Connection to existing WiFi network failed");
    return;
  }

  WiFi.begin(this->wifiClientSsid, this->wifiPassword);

  sprintf(logString, "Trying to connect to WiFi with ssid %s", this->wifiClientSsid);
  ESPStepperMotorServer_Logger::logInfo(logString);
  int timeoutCounter = this->wifiClientConnectionTimeoutSeconds * 2;
  while (WiFi.status() != WL_CONNECTED && timeoutCounter > 0)
  {
    delay(500); //Do not change this value unless also changing the formula in the declaration of the timeoutCounter integer above
    ESPStepperMotorServer_Logger::logInfo(".", false, true);
    timeoutCounter--;
  }
  ESPStepperMotorServer_Logger::logInfo("\n", false);

  if (timeoutCounter > 0)
  {
    sprintf(logString, "Connected to network with IP address %s",  WiFi.localIP().toString().c_str());
    ESPStepperMotorServer_Logger::logInfo(logString);
  }
  else
  {
    sprintf(logString, "Connection to WiFi network with SSID %s failed with timeout", this->wifiClientSsid);
    ESPStepperMotorServer_Logger::logWarning(logString);
    sprintf(logString, "Connection timeout is set to %i seconds", this->wifiClientConnectionTimeoutSeconds);
    ESPStepperMotorServer_Logger::logDebug(logString);

    sprintf(logString, "starting server in access point mode with SSID %s as fallback", this->accessPointSSID);
    ESPStepperMotorServer_Logger::logWarning(logString);
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
    ESPStepperMotorServer_PositionSwitch posSwitch = this->configuredPositionSwitches[i];
    if (posSwitch.getIoPinNumber() != ESPServerPositionSwitchUnsetPinNumber)
    {
      char irqNum = digitalPinToInterrupt(posSwitch.getIoPinNumber());
      if (irqNum == NOT_AN_INTERRUPT)
      {
        sprintf(this->logString, "Failed to determine IRQ# for given IO Pin %i, thus setting up of interrupt for the position switch failed for pin %s", posSwitch.getIoPinNumber(), posSwitch.getPositionName().c_str());
        ESPStepperMotorServer_Logger::logWarning(this->logString);
      }

      sprintf(this->logString, "attaching interrupt service routine for position switch %s on IO Pin %i", posSwitch.getPositionName().c_str(), posSwitch.getIoPinNumber());
      ESPStepperMotorServer_Logger::logDebug(this->logString);
      _BV(irqNum); // clear potentially pending interrupts
      attachInterrupt(irqNum, staticPositionSwitchISR, CHANGE);
    }
  }

  for (int i = 0; i < ESPServerMaxRotaryEncoders; i++)
  {
    ESPStepperMotorServer_RotaryEncoder *rotaryEncoder = this->configuredRotaryEncoders[i];
    if (rotaryEncoder != NULL)
    {
      //we do a loop here to save some pgrogram memory, could also externalize code block in another function
      const unsigned char pins[2] = {rotaryEncoder->getPinAIOPin(), rotaryEncoder->getPinBIOPin()};
      for (int i = 0; i < 2; i++)
      {
        char irqNum = digitalPinToInterrupt(pins[i]);
        if (irqNum == NOT_AN_INTERRUPT)
        {
          sprintf(this->logString, "Failed to determine IRQ# for given IO Pin %i, thus setting up of interrupt for the rotary encoder failed for pin %s", pins[i], rotaryEncoder->getDisplayName().c_str());
          ESPStepperMotorServer_Logger::logWarning(this->logString);
        }

        sprintf(this->logString, "attaching interrupt service routine for rotary encoder %s on IO Pin %i", rotaryEncoder->getDisplayName().c_str(), pins[i]);
        ESPStepperMotorServer_Logger::logDebug(this->logString);
        _BV(irqNum); // clear potentially pending interrupts
        attachInterrupt(irqNum, staticRotaryEncoderISR, CHANGE);
      }
    }
  }
}

void ESPStepperMotorServer::detachInterruptForPositionSwitch(ESPStepperMotorServer_PositionSwitch *posSwitch)
{
  sprintf(this->logString, "detaching interrupt for position switch %s on IO Pin %i", posSwitch->getPositionName().c_str(), posSwitch->getIoPinNumber());
  ESPStepperMotorServer_Logger::logDebug(this->logString);
  detachInterrupt(digitalPinToInterrupt(posSwitch->getIoPinNumber()));
}

void ESPStepperMotorServer::detachInterruptForRotaryEncoder(ESPStepperMotorServer_RotaryEncoder *rotaryEncoder)
{
  sprintf(this->logString, "detaching interrupts for rotary encoder %s on IO Pins %i and %i", rotaryEncoder->getDisplayName().c_str(), rotaryEncoder->getPinAIOPin(), rotaryEncoder->getPinBIOPin());
  ESPStepperMotorServer_Logger::logDebug(this->logString);
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
    ESPStepperMotorServer_PositionSwitch posSwitch = this->configuredPositionSwitches[i];
    if (posSwitch.getIoPinNumber() != ESPServerPositionSwitchUnsetPinNumber)
    {
      this->detachInterruptForPositionSwitch(&posSwitch);
    }
  }
  for (int i = 0; i < ESPServerMaxRotaryEncoders; i++)
  {
    ESPStepperMotorServer_RotaryEncoder *rotaryEncoder = this->configuredRotaryEncoders[i];
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
  for (int i = 0; i < configuredPositionSwitchIndex; i++)
  {
    if (i > 7)
    {
      registerIndex = (byte)(ceil)((i + 1) / 8);
    }
    pinNumber = this->configuredPositionSwitchIoPins[i];
    boolean isEmergencySwitch = (this->emergencySwitchIndexes[i] == 1);
    byte pinStateType = 0;
    if (isEmergencySwitch)
    {
      pinStateType = ((this->configuredPositionSwitches[i].getSwitchType() & ESPServerSwitchType_ActiveHigh) == ESPServerSwitchType_ActiveHigh) ? ESPServerSwitchType_ActiveHigh : ESPServerSwitchType_ActiveLow;
    }
    if (pinNumber != ESPServerPositionSwitchUnsetPinNumber && digitalRead(pinNumber))
    {
      bitSet(this->buttonStatus[registerIndex], i % 8);
      if (isEmergencySwitch && pinStateType == ESPServerSwitchType_ActiveHigh)
      {
        this->emergencySwitchIsActive = true;
      }
    }
    else
    {
      bitClear(this->buttonStatus[registerIndex], i % 8);
      if (isEmergencySwitch && pinStateType == ESPServerSwitchType_ActiveLow)
      {
        this->emergencySwitchIsActive = true;
      }
    }
  }
  this->positionSwitchUpdateAvailable = true;
}

void ESPStepperMotorServer::internalRotaryEncoderISR()
{
  for (int i = 0; i < ESPServerMaxRotaryEncoders; i++)
  {
    ESPStepperMotorServer_RotaryEncoder *rotaryEncoder = this->configuredRotaryEncoders[i];
    if (rotaryEncoder != NULL)
    {
      unsigned char result = rotaryEncoder->process();
      if (result == DIR_CW)
      {
        this->configuredSteppers[rotaryEncoder->getStepperIndex()]->getFlexyStepper()->setTargetPositionRelativeInSteps(1 * rotaryEncoder->getStepMultiplier());
      }
      else if (result == DIR_CCW)
      {
        this->configuredSteppers[rotaryEncoder->getStepperIndex()]->getFlexyStepper()->setTargetPositionRelativeInSteps(-1 * rotaryEncoder->getStepMultiplier());
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

// ----------------- delegator functions to easy API usage -------------------------

void ESPStepperMotorServer::setLogLevel(byte logLevel)
{
  ESPStepperMotorServer_Logger::setLogLevel(logLevel);
}

// -------------------------------------- End --------------------------------------

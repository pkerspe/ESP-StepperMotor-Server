
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
  this->setLogLevel(ESPServerLogLevel_INFO);
  this->enabledServices = serverMode;
  if ((this->enabledServices & ESPServerWebserverEnabled) == ESPServerWebserverEnabled)
  {
    this->isWebserverEnabled = true;
  }
  if ((this->enabledServices & ESPServerRestApiEnabled) == ESPServerRestApiEnabled)
  {
    this->isRestApiEnabled = true;
  }

  if (ESPStepperMotorServer::anchor != NULL)
  {
    logWarning("ESPStepperMotorServer must be used as a singleton, do not instanciate more than one server in your project");
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

int ESPStepperMotorServer::addStepper(ESPStepperMotorServer_Stepper *stepper)
{
  if (this->configuredStepperIndex >= ESPServerMaxSteppers)
  {
    sprintf(this->logString, "The maximum amount of steppers (%i) that can be configured has been reached, no more steppers can be added", ESPServerMaxSteppers);
    this->logWarning(this->logString);
    this->logWarning("The value can only be increased during compile time, by modifying the value of ESPServerMaxSteppers in ESPStepperMotorServer.h");
    return -1;
  }
  stepper->setId(this->configuredStepperIndex);
  this->configuredSteppers[this->configuredStepperIndex] = stepper;
  this->configuredStepperIndex++;
  return this->configuredStepperIndex - 1;
}

void ESPStepperMotorServer::removeStepper(int id)
{
  ESPStepperMotorServer_Stepper *stepperToRemove = this->configuredSteppers[id];
  if (stepperToRemove != NULL && stepperToRemove->getId() == id)
  {
    this->removeStepper(stepperToRemove);
  }
  else
  {
    sprintf(this->logString, "stepper configuration index %i is invalid, no stepper pointer present at this configuration index or stepper IDs do not match, removeStepper() canceled", id);
    logWarning(this->logString);
  }
}

void ESPStepperMotorServer::removeStepper(ESPStepperMotorServer_Stepper *stepper)
{
  if (stepper != NULL && this->configuredSteppers[stepper->getId()]->getId() == stepper->getId())
  {
    // check if any switch is configured for the stepper to be removed, if so remove switches first
    for (int i = 0; i < ESPServerMaxSwitches; i++)
    {
      if (this->configuredPositionSwitches[i].stepperIndex == stepper->getId())
      {
        positionSwitch posSwitchToRemove = this->configuredPositionSwitches[i];
        positionSwitch blankPosSwitch;
        this->detachInterruptForPositionSwitch(&posSwitchToRemove);
        sprintf(this->logString, "removed position switch %s (idx: %i) from configured position switches since related stepper will be removed next", posSwitchToRemove.positionName, i);
        logDebug(this->logString);
        this->configuredPositionSwitches[i] = blankPosSwitch;
        this->configuredPositionSwitchIoPins[i] = ESPServerPositionSwitchUnsetPinNumber;
        //by default null the emergency switch flag, even if swith was not an emergency switch
        this->emergencySwitchIndexes[i] = 0;
      }
    }
    //now remove the stepper configuration itself
    this->configuredSteppers[stepper->getId()] = NULL;
  }
  else
  {
    logInfo("Invalid stepper given. The ID does not match the configured stepper. RemoveStepper failed");
  }
}

int ESPStepperMotorServer::addPositionSwitch(byte stepperIndex, byte ioPinNumber, byte switchType, const char *positionName, long switchPosition)
{
  positionSwitch positionSwitchToAdd;
  positionSwitchToAdd.stepperIndex = stepperIndex;
  positionSwitchToAdd.ioPinNumber = ioPinNumber;
  positionSwitchToAdd.switchType = switchType;
  positionSwitchToAdd.positionName = positionName;
  positionSwitchToAdd.switchPosition = switchPosition;
  return this->addPositionSwitch(positionSwitchToAdd);
}

int ESPStepperMotorServer::addPositionSwitch(positionSwitch posSwitchToAdd)
{
  if (posSwitchToAdd.stepperIndex > this->configuredStepperIndex)
  {
    sprintf(this->logString, "invalid stepperIndex value given. The number of configured steppers is %i but index value of %i was given in addPositionSwitch() call.", this->configuredStepperIndex, posSwitchToAdd.stepperIndex);
    this->logWarning(this->logString);
    this->logWarning("the stepper instance has not been added to the server configuration");
    return -1;
  }
  //check if we have a blank configuration slot before the actual index (due to possible removal of previously configured position switches that might have been removed in the meantime)
  int freePositionSwitchIndex = this->configuredPositionSwitchIndex;
  for (int i = 0; i < this->configuredPositionSwitchIndex; i++)
  {
    if (this->configuredPositionSwitches[i].ioPinNumber == ESPServerPositionSwitchUnsetPinNumber)
    {
      freePositionSwitchIndex = i;
      break;
    }
  }

  this->configuredPositionSwitches[freePositionSwitchIndex] = posSwitchToAdd;
  this->configuredPositionSwitchIoPins[freePositionSwitchIndex] = posSwitchToAdd.ioPinNumber;
  this->emergencySwitchIndexes[freePositionSwitchIndex] = ((posSwitchToAdd.switchType & ESPServerSwitchType_EmergencyStopSwitch) == ESPServerSwitchType_EmergencyStopSwitch);
  //Setup IO Pin
  this->setupPositionSwitchIOPin(&posSwitchToAdd);

  sprintf(this->logString, "added position switch %s for IO pin %i at configuration index %i", this->configuredPositionSwitches[freePositionSwitchIndex].positionName, this->configuredPositionSwitches[freePositionSwitchIndex].ioPinNumber, freePositionSwitchIndex);
  logInfo(this->logString);

  if (this->configuredPositionSwitchIndex == freePositionSwitchIndex)
  {
    this->configuredPositionSwitchIndex++;
  }
  return configuredPositionSwitchIndex - 1;
}

void ESPStepperMotorServer::removePositionSwitch(int positionSwitchIndex)
{
  positionSwitch *posSwitch = &this->configuredPositionSwitches[positionSwitchIndex];
  if (posSwitch->ioPinNumber != ESPServerPositionSwitchUnsetPinNumber)
  {
    this->removePositionSwitch(posSwitch);
  }
  else
  {
    sprintf(this->logString, "position switch index %i is invalid, no position switch present at this configuration index, removePositionSwitch() canceled", positionSwitchIndex);
    logWarning(this->logString);
  }
}

void ESPStepperMotorServer::removePositionSwitch(positionSwitch *posSwitchToRemove)
{
  if (posSwitchToRemove->ioPinNumber != ESPServerPositionSwitchUnsetPinNumber)
  {
    for (int i = 0; i < ESPServerMaxSwitches; i++)
    {
      if (this->configuredPositionSwitches[i].ioPinNumber == posSwitchToRemove->ioPinNumber)
      {
        positionSwitch blankPosSwitch;
        this->detachInterruptForPositionSwitch(posSwitchToRemove);
        sprintf(this->logString, "removed position switch %s (idx: %i) from configured position switches", posSwitchToRemove->positionName, i);
        logDebug(this->logString);
        this->configuredPositionSwitches[i] = blankPosSwitch;
        this->configuredPositionSwitchIoPins[i] = ESPServerPositionSwitchUnsetPinNumber;
        this->emergencySwitchIndexes[i] = 0;
        return;
      }
    }
  }
  else
  {
    logInfo("Invalid position switch given, IO pin was not set");
  }
}

// ---------------------------------------------------------------------------------
//                     general service control functions
// ---------------------------------------------------------------------------------

void ESPStepperMotorServer::start()
{
  sprintf(logString, "Starting ESP-StepperMotor-Server (v. %s)", this->version);
  this->logInfo(logString);

  if (this->wifiMode == ESPServerWifiModeAccessPoint)
  {
    this->startAccessPoint();
    if (this->logLevel >= ESPServerLogLevel_DEBUG)
    {
      this->printWifiStatus();
    }
  }
  else if (this->wifiMode == ESPServerWifiModeClient)
  {
    this->connectToWifiNetwork();
    if (this->logLevel >= ESPServerLogLevel_DEBUG)
    {
      this->printWifiStatus();
    }
  }
  else if (this->wifiMode == ESPServerWifiModeDisabled)
  {
    this->logInfo("WiFi mode is disabled, will only serial control inerface");
  }

  this->startWebserver();

  this->attachAllInterrupts();
}

void ESPStepperMotorServer::stop()
{
  this->logInfo("Stopping ESP-StepperMotor-Server");
  this->detachAllInterrupts();
}

// ---------------------------------------------------------------------------------
//                                  Status and Service Functions
// ---------------------------------------------------------------------------------

void ESPStepperMotorServer::printPositionSwitchStatus()
{
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
    char binaryString[9];
    printBinaryWithLeaingZeros(binaryString, this->buttonStatus[i]);
    positionSwitchRegister["status"] = binaryString;
  }

  JsonArray switches = root.createNestedArray("positionSwitches");
  for (int i = 0; i < this->configuredPositionSwitchIndex; i++)
  {
    if (this->configuredPositionSwitches[i].ioPinNumber != ESPServerPositionSwitchUnsetPinNumber)
    {
      JsonObject positionSwitch = switches.createNestedObject();
      positionSwitch["id"] = i;
      positionSwitch["name"] = this->configuredPositionSwitches[i].positionName;
      positionSwitch["ioPin"] = this->configuredPositionSwitches[i].ioPinNumber;
      positionSwitch["position"] = this->configuredPositionSwitches[i].switchPosition;
      positionSwitch["stepperId"] = this->configuredPositionSwitches[i].stepperIndex;
      positionSwitch["active"] = this->getPositionSwitchStatus(i);

      JsonObject positionSwichType = positionSwitch.createNestedObject("type");
      if ((this->configuredPositionSwitches[i].switchType & ESPServerSwitchType_ActiveHigh) == ESPServerSwitchType_ActiveHigh)
      {
        positionSwichType["pinMode"] = "Active High";
      }
      else
      {
        positionSwichType["pinMode"] = "Active Low";
      }
      if ((this->configuredPositionSwitches[i].switchType & ESPServerSwitchType_HomingSwitchBegin) == ESPServerSwitchType_HomingSwitchBegin)
      {
        positionSwichType["switchType"] = "Homing switch (start-position)";
      }
      else if ((this->configuredPositionSwitches[i].switchType & ESPServerSwitchType_HomingSwitchEnd) == ESPServerSwitchType_HomingSwitchEnd)
      {
        positionSwichType["switchType"] = "Homing switch (end-position)";
      }
      else if ((this->configuredPositionSwitches[i].switchType & ESPServerSwitchType_GeneralPositionSwitch) == ESPServerSwitchType_GeneralPositionSwitch)
      {
        positionSwichType["switchType"] = "General position switch";
      }
      else if ((this->configuredPositionSwitches[i].switchType & ESPServerSwitchType_EmergencyStopSwitch) == ESPServerSwitchType_EmergencyStopSwitch)
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
  positionSwitch posSwitch = this->configuredPositionSwitches[positionSwitchIndex];
  byte bitVal = (1 << positionSwitchIndex % 8);
  byte buttonState = ((this->buttonStatus[buttonRegisterIndex] & bitVal) == bitVal);

  if (posSwitch.ioPinNumber != ESPServerPositionSwitchUnsetPinNumber)
  {
    if ((posSwitch.switchType & ESPServerSwitchType_ActiveHigh) == ESPServerSwitchType_ActiveHigh)
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
  logDebug("Checking SPIFFS for existance and free space");
  if (SPIFFS.begin())
  {
    logDebug("SPIFFS started");
    printSPIFFSStats();
  }
  else
  {
    logWarning("Unable to activate SPIFFS. Files for WebInterface cannot be loaded");
  }
}

void ESPStepperMotorServer::printSPIFFSStats()
{
  logDebug("SPIFFS stats:");
  logDebug("Total bytes: ", false);
  logDebug(String((int)SPIFFS.totalBytes(), DEC), true, true);
  logDebug("bytes used in SPIFFS: ", false);
  logDebug(String((int)SPIFFS.usedBytes(), DEC), true, true);
  logDebug("bytes available: ", false);
  logDebug(String(getSPIFFSFreeSpace(), DEC), true, true);
}

void ESPStepperMotorServer::printSPIFFSRootFolderContents()
{
  File root = SPIFFS.open("/");

  if (!root)
  {
    logWarning("Failed to open root folder on SPIFFS for reading");
  }
  if (root.isDirectory())
  {
    logInfo("Listing files in root folder of SPIFFS:");
    File file = root.openNextFile();
    while (file)
    {
      logInfo("File: ", false, true);
      logInfo(file.name(), true, true);
      file = root.openNextFile();
    }
    root.close();
  }
}

int ESPStepperMotorServer::getSPIFFSFreeSpace()
{
  return ((int)SPIFFS.totalBytes() - (int)SPIFFS.usedBytes());
}

void ESPStepperMotorServer::startWebserver()
{

  if (isWebserverEnabled || isRestApiEnabled)
  {
    startSPIFFS();
    printSPIFFSRootFolderContents();

    httpServer = new AsyncWebServer(this->httpPortNumber);

    if (isWebserverEnabled)
    {
      this->registerWebInterfaceUrls();
    }
    if (isRestApiEnabled)
    {
      this->registerRestApiEndpoints();
    }

    httpServer->begin();
  }
}



// Perform an HTTP GET request to a remote page
bool ESPStepperMotorServer::downloadFileToSpiffs(const char *url, const char *targetPath)
{
  HTTPClient client;
  
  Serial.println("Connecing to server for Web UI Download");
  if (client.begin("https://raw.githubusercontent.com/pkerspe/ESP-StepperMotor-Server/master/LICENSE.txt"))
  {
    int httpCode = client.GET();
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        String payload = client.getString();
        Serial.println(payload);
      }
    }
    else
    {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", client.errorToString(httpCode).c_str());
    }

    client.end();
  }

  Serial.println("done");
  return true;
}

void ESPStepperMotorServer::registerRestApiEndpoints()
{
  // GET /api/status
  httpServer->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    StaticJsonDocument<200> doc;
    JsonObject root = doc.to<JsonObject>();
    root["version"] = this->version;

    JsonObject wifiStatus = root.createNestedObject("wifi");
    wifiStatus["mode"] = (this->wifiMode == ESPServerWifiModeAccessPoint) ? "ap" : "client";
    wifiStatus["ip"] = (this->wifiMode == ESPServerWifiModeAccessPoint) ? WiFi.dnsIP().toString() : WiFi.localIP().toString();

    JsonObject spiffsStatus = root.createNestedObject("spiffss");
    spiffsStatus["total_space"] = (int)SPIFFS.totalBytes();
    spiffsStatus["free_space"] = getSPIFFSFreeSpace();

    String output;
    serializeJson(root, output);
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    request->send(response);
  });

  // GET /api/steppers/position?id=<id>
  httpServer->on("/api/steppers/position", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String output;

    if (request->hasParam("id"))
    {
      int stepperIndex = request->getParam("id")->value().toInt();
      if (stepperIndex < 0 || stepperIndex >= ESPServerMaxSteppers)
      {
        request->send(404);
        return;
      }

      StaticJsonDocument<100> doc;
      JsonObject root = doc.to<JsonObject>();
      ESPStepperMotorServer_Stepper *stepper = this->configuredSteppers[stepperIndex];
      root["mm"] = stepper->getFlexyStepper()->getCurrentPositionInMillimeters();
      root["revs"] = stepper->getFlexyStepper()->getCurrentPositionInRevolutions();
      root["steps"] = stepper->getFlexyStepper()->getCurrentPositionInSteps();
      serializeJson(root, output);
      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
      request->send(response);
    }
    else
    {
      request->send(404);
      return;
    }
  });

  // POST /api/steppers/moveby
  // endpoint to set a new RELATIVE target position for the stepper motor in either mm, revs or steps
  // post parameters: id, unit, value
  httpServer->on("/api/steppers/moveby", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (request->hasParam("id", true))
    {
      int stepperIndex = request->getParam("id", true)->value().toInt();
      if (stepperIndex < 0 || stepperIndex >= ESPServerMaxSteppers || this->configuredSteppers[stepperIndex] == NULL)
      {
        request->send(404);
        return;
      }
      if (request->hasParam("value", true) && request->hasParam("unit", true))
      {
        String unit = request->getParam("unit", true)->value();
        float distance = request->getParam("value", true)->value().toFloat();
        if (unit == "mm")
        {
          this->configuredSteppers[stepperIndex]->getFlexyStepper()->moveRelativeInMillimeters(distance);
        }
        else if (unit == "revs")
        {
          this->configuredSteppers[stepperIndex]->getFlexyStepper()->moveRelativeInRevolutions(distance);
        }
        else if (unit == "steps")
        {
          this->configuredSteppers[stepperIndex]->getFlexyStepper()->moveRelativeInSteps(distance);
        }
        else
        {
          request->send(400);
          return;
        }

        request->send(204);
        return;
      }
    }
    request->send(400);
  });

  // POST /api/steppers/position
  // endpoint to set a new absolute target position for the stepper motor in either mm, revs or steps
  // post parameters: id, unit, value
  httpServer->on("/api/steppers/position", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (request->hasParam("id", true))
    {
      int stepperIndex = request->getParam("id", true)->value().toInt();
      if (stepperIndex < 0 || stepperIndex >= ESPServerMaxSteppers || this->configuredSteppers[stepperIndex] == NULL)
      {
        request->send(404);
        return;
      }
      if (request->hasParam("value", true) && request->hasParam("unit", true))
      {
        String unit = request->getParam("unit", true)->value();
        float position = request->getParam("value", true)->value().toFloat();
        if (unit == "mm")
        {
          this->configuredSteppers[stepperIndex]->getFlexyStepper()->moveToPositionInMillimeters(position);
        }
        else if (unit == "revs")
        {
          this->configuredSteppers[stepperIndex]->getFlexyStepper()->moveToPositionInRevolutions(position);
        }
        else if (unit == "steps")
        {
          this->configuredSteppers[stepperIndex]->getFlexyStepper()->moveToPositionInSteps(position);
        }
        else
        {
          request->send(400);
          return;
        }

        request->send(204);
        return;
      }
    }
    request->send(400);
  });

  // GET /api/steppers
  // GET /api/steppers?id=<id>
  // endpoint to list all configured steppers or a specific one if "id" query parameter is given
  httpServer->on("/api/steppers", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String output;

    if (request->hasParam("id"))
    {
      int stepperIndex = request->getParam("id")->value().toInt();
      if (stepperIndex < 0 || stepperIndex >= ESPServerMaxSteppers)
      {
        request->send(404);
        return;
      }

      StaticJsonDocument<200> doc;
      JsonObject root = doc.to<JsonObject>();
      JsonObject stepperDetails = root.createNestedObject("stepper");
      this->populateStepperDetailsToJsonObject(stepperDetails, this->configuredSteppers[stepperIndex], stepperIndex);
      serializeJson(root, output);
    }
    else
    {
      const int docSize = 200 * ESPServerMaxSteppers;
      StaticJsonDocument<docSize> doc;
      JsonObject root = doc.to<JsonObject>();
      JsonArray steppers = root.createNestedArray("steppers");
      for (int i = 0; i < ESPServerMaxSteppers; i++)
      {
        JsonObject stepperDetails = steppers.createNestedObject();
        this->populateStepperDetailsToJsonObject(stepperDetails, this->configuredSteppers[i], i);
      }
      serializeJson(root, output);
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    request->send(response);
  });

  // DELETE /api/steppers?id=<id>
  httpServer->on("/api/steppers", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
    if (request->params() == 1 && request->getParam(0)->name() == "id")
    {
      int stepperIndex = request->getParam(0)->value().toInt();
      if (stepperIndex < 0 || stepperIndex >= ESPServerMaxSteppers || this->configuredSteppers[stepperIndex] != NULL)
      {
        this->removeStepper(stepperIndex);
        request->send(204);
      }
      else
      {
        request->send(404);
      }
    }
    else
    {
      request->send(404);
    }
  });

  // POST /api/steppers
  /*
  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/api/steppers", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject &jsonObj = json.as<JsonObject>();
  });
  httpServer->addHandler(handler);
  */
  // PUT /api/steppers?id=<id>

  // GET /api/steppers/status?id=<id>

  // GET /api/switches?id=<id>
  // GET /api/switches/status?id=<id>
  // POST /api/switches
  // PUT /api/switches?id=<id>
  // DELETE /api/switches?id=<id>

  // GET /api/outputs
  // GET /api/outputs?id=<id>
  // GET /api/outputs/status?id=<id>
  // PUT /api/outputs/status?id=<id>
  // POST /api/outputs
  // PUT /api/outputs?id=<id>
  // DELETE /api/outputs?id=<id>
}

void ESPStepperMotorServer::registerWebInterfaceUrls()
{
  //httpServer->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  httpServer->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html");
  });
  httpServer->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/favicon.ico.gz", "image/x-icon");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer->on("/dist/build.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/dist/build.js.gz", "text/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer->on("/dist/logo.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/dist/logo.png");
  });
}

// ---------------------------------------------------------------------------------
//             internal helper functions for stepper communication
// ---------------------------------------------------------------------------------

void ESPStepperMotorServer::populateStepperDetailsToJsonObject(JsonObject &stepperDetails, ESPStepperMotorServer_Stepper *stepper, int index)
{
  stepperDetails["id"] = index;
  stepperDetails["configured"] = (stepper == NULL) ? "false" : "true";
  if (stepper != NULL)
  {
    JsonObject position = stepperDetails.createNestedObject("position");
    position["mm"] = stepper->getFlexyStepper()->getCurrentPositionInMillimeters();
    position["revs"] = stepper->getFlexyStepper()->getCurrentPositionInRevolutions();
    position["steps"] = stepper->getFlexyStepper()->getCurrentPositionInSteps();

    JsonObject stepperStatus = stepperDetails.createNestedObject("velocity");
    stepperStatus["rev_s"] = stepper->getFlexyStepper()->getCurrentVelocityInRevolutionsPerSecond();
    stepperStatus["mm_s"] = stepper->getFlexyStepper()->getCurrentVelocityInMillimetersPerSecond();
    stepperStatus["steps_s"] = stepper->getFlexyStepper()->getCurrentVelocityInStepsPerSecond();

    stepperDetails["motionComplete"] = stepper->getFlexyStepper()->motionComplete();
  }
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
    logWarning("Invalid WiFi mode given in setWifiMode");
    break;
  }
}

/**
 * print the wifi status (ssid, IP Address etc.) on the serial port
 */
void ESPStepperMotorServer::printWifiStatus()
{
  this->logInfo("ESPStepperMotorServer WiFi details:");

  if (this->wifiMode == ESPServerWifiModeClient)
  {
    this->logInfo("WiFi status: server acts as wifi client in existing network with DHCP");
    sprintf(this->logString, "SSID: %s", this->wifiClientSsid);
    this->logInfo(logString);

    sprintf(this->logString, "IP address: %s", WiFi.localIP().toString().c_str());
    this->logInfo(logString);

    sprintf(this->logString, "Strength: %i dBm", WiFi.RSSI()); //Received Signal Strength Indicator
    this->logInfo(logString);
  }
  else if (this->wifiMode == ESPServerWifiModeAccessPoint)
  {
    this->logInfo("WiFi status: access point started");
    sprintf(this->logString, "SSID: %s", this->accessPointSSID);
    this->logInfo(logString);

    sprintf(this->logString, "IP Address: %s", WiFi.softAPIP().toString().c_str());
    this->logInfo(logString);
  }
  else
  {
    this->logInfo("WiFi is disabled");
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
  this->logInfo(this->logString);
}

void ESPStepperMotorServer::connectToWifiNetwork()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    this->logInfo("Module is already conencted to WiFi network. Will skip WiFi connection procedure");
    return;
  }

  if ((this->wifiClientSsid != NULL) && (this->wifiClientSsid[0] == '\0'))
  {
    this->logWarning("No ssid has been configured to connect to. Connection to existing WiFi network failed");
    return;
  }

  WiFi.begin(this->wifiClientSsid, this->wifiPassword);

  sprintf(logString, "Trying to connect to WiFi with ssid %s", this->wifiClientSsid);
  this->logInfo(logString);
  int timeoutCounter = this->wifiClientConnectionTimeoutSeconds * 2;
  while (WiFi.status() != WL_CONNECTED && timeoutCounter > 0)
  {
    delay(500); //Do not change this value unless also changing the formula in the declaration of the timeoutCounter integer above
    this->logInfo(".", false, true);
    timeoutCounter--;
  }
  this->logInfo("\n", false);

  if (timeoutCounter > 0)
  {
    this->logInfo("Connected to network");
  }
  else
  {
    sprintf(logString, "Connection to WiFi network with SSID %s failed with timeout", this->wifiClientSsid);
    this->logWarning(logString);
    sprintf(logString, "Connection timeout is set to %i seconds", this->wifiClientConnectionTimeoutSeconds);
    this->logDebug(logString);

    sprintf(logString, "starting server in access point mode with SSID %s as fallback", this->accessPointSSID);
    this->logWarning(logString);
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
void ESPStepperMotorServer::setupPositionSwitchIOPin(positionSwitch *posSwitchToAdd)
{
  if (posSwitchToAdd->switchType & ESPServerSwitchType_ActiveHigh)
  {
    pinMode(posSwitchToAdd->ioPinNumber, INPUT);
  }
  else
  {
    pinMode(posSwitchToAdd->ioPinNumber, INPUT_PULLUP);
  }
}

/**
 * Register ISR according to switch type (active high or active low) for all configured position switches
 */
void ESPStepperMotorServer::attachAllInterrupts()
{
  for (int i = 0; i < ESPServerMaxSwitches; i++)
  {
    positionSwitch posSwitch = this->configuredPositionSwitches[i];
    if (posSwitch.ioPinNumber != ESPServerPositionSwitchUnsetPinNumber)
    {
      char irqNum = digitalPinToInterrupt(posSwitch.ioPinNumber);
      if (irqNum == NOT_AN_INTERRUPT)
      {
        sprintf(this->logString, "Failed to determine IRQ# for given IO Pin %i, thus setting up of interrupt for the position switch failed for pin %s", posSwitch.ioPinNumber, posSwitch.positionName);
        logWarning(this->logString);
      }

      sprintf(this->logString, "attaching interrupt service routine for position switch %s on IO Pin %i", posSwitch.positionName, posSwitch.ioPinNumber);
      logDebug(this->logString);
      _BV(irqNum); // clear potentially pending interrupts
      attachInterrupt(irqNum, staticPositionSwitchISR, CHANGE);
    }
  }
}

void ESPStepperMotorServer::detachInterruptForPositionSwitch(positionSwitch *posSwitch)
{
  sprintf(this->logString, "detaching interrupt for position switch %s on IO Pin %i", posSwitch->positionName, posSwitch->ioPinNumber);
  logDebug(this->logString);
  detachInterrupt(digitalPinToInterrupt(posSwitch->ioPinNumber));
}

/**
 * clear/disable all interrupts for position switches
 **/
void ESPStepperMotorServer::detachAllInterrupts()
{
  for (int i = 0; i < ESPServerMaxSwitches; i++)
  {
    positionSwitch posSwitch = this->configuredPositionSwitches[i];
    if (posSwitch.ioPinNumber != ESPServerPositionSwitchUnsetPinNumber)
    {
      this->detachInterruptForPositionSwitch(&posSwitch);
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
      pinStateType = ((this->configuredPositionSwitches[i].switchType & ESPServerSwitchType_ActiveHigh) == ESPServerSwitchType_ActiveHigh) ? ESPServerSwitchType_ActiveHigh : ESPServerSwitchType_ActiveLow;
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

void ESPStepperMotorServer::staticPositionSwitchISR()
{
  anchor->internalPositionSwitchISR();
}

// ---------------------------------------------------------------------------------
//                                  Logging functions
// ---------------------------------------------------------------------------------

void ESPStepperMotorServer::printBinaryWithLeaingZeros(char *result, byte var)
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

void ESPStepperMotorServer::setLogLevel(byte logLevel)
{
  switch (logLevel)
  {
  case ESPServerLogLevel_ALL:
  case ESPServerLogLevel_DEBUG:
  case ESPServerLogLevel_INFO:
  case ESPServerLogLevel_WARNING:
    this->logLevel = logLevel;
    break;
  default:
    this->logWarning("Invalid log level given, log level will be set to info");
    this->logLevel = ESPServerLogLevel_INFO;
    break;
  }
}

void ESPStepperMotorServer::log(const char *level, const char *msg, boolean newLine, boolean ommitLogLevel)
{
  if (!ommitLogLevel)
  {
    Serial.print("[");
    Serial.print(level);
    Serial.print("] ");
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

void ESPStepperMotorServer::logDebug(const char *msg, boolean newLine, boolean ommitLogLevel)
{
  if (this->logLevel >= ESPServerLogLevel_DEBUG)
  {
    log("DEBUG", msg, newLine, ommitLogLevel);
  }
}

void ESPStepperMotorServer::logDebug(String msg, boolean newLine, boolean ommitLogLevel)
{
  this->logDebug(msg.c_str(), newLine, ommitLogLevel);
}

void ESPStepperMotorServer::logInfo(const char *msg, boolean newLine, boolean ommitLogLevel)
{
  if (this->logLevel >= ESPServerLogLevel_INFO)
  {
    log("INFO", msg, newLine, ommitLogLevel);
  }
}

void ESPStepperMotorServer::logWarning(const char *msg, boolean newLine, boolean ommitLogLevel)
{
  if (this->logLevel >= ESPServerLogLevel_WARNING)
  {
    log("WARNING", msg, newLine, ommitLogLevel);
  }
}

// -------------------------------------- End --------------------------------------


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
      if (this->configuredPositionSwitches[i].stepperIndex == stepper->getId())
      {
        positionSwitch posSwitchToRemove = this->configuredPositionSwitches[i];
        positionSwitch blankPosSwitch;
        this->detachInterruptForPositionSwitch(&posSwitchToRemove);
        sprintf(this->logString, "removed position switch %s (idx: %i) from configured position switches since related stepper will be removed next", posSwitchToRemove.positionName.c_str(), i);
        ESPStepperMotorServer_Logger::logDebug(this->logString);
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
    ESPStepperMotorServer_Logger::logInfo("Invalid stepper given. The ID does not match the configured stepper. RemoveStepper failed");
  }
}

int ESPStepperMotorServer::addPositionSwitch(byte stepperIndex, byte ioPinNumber, byte switchType, String positionName, long switchPosition)
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
    ESPStepperMotorServer_Logger::logWarning(this->logString);
    ESPStepperMotorServer_Logger::logWarning("the stepper instance has not been added to the server configuration");
    return -1;
  }
  if (posSwitchToAdd.positionName.length() > ESPSMS_Stepper_DisplayName_MaxLength)
  {
    char logString[160];
    sprintf(logString, "ESPStepperMotorServer::addPositionSwitch: The display name for the position switch is to long. Max length is %i characters. Name will be trimmed", ESPStepperMotorServer_SwitchDisplayName_MaxLength);
    ESPStepperMotorServer_Logger::logWarning(logString);
    posSwitchToAdd.positionName = posSwitchToAdd.positionName.substring(0, ESPSMS_Stepper_DisplayName_MaxLength);
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

  sprintf(this->logString, "added position switch %s for IO pin %i at configuration index %i", this->configuredPositionSwitches[freePositionSwitchIndex].positionName.c_str(), this->configuredPositionSwitches[freePositionSwitchIndex].ioPinNumber, freePositionSwitchIndex);
  ESPStepperMotorServer_Logger::logInfo(this->logString);

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
    ESPStepperMotorServer_Logger::logWarning(this->logString);
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
        sprintf(this->logString, "removed position switch %s (idx: %i) from configured position switches", posSwitchToRemove->positionName.c_str(), i);
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
}

void ESPStepperMotorServer::stop()
{
  ESPStepperMotorServer_Logger::logInfo("Stopping ESP-StepperMotor-Server");
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

void ESPStepperMotorServer::startWebserver()
{

  if (isWebserverEnabled || isRestApiEnabled)
  {
    startSPIFFS();
    printSPIFFSRootFolderContents();

    httpServer = new AsyncWebServer(this->httpPortNumber);

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
  const char *files[4] = {this->webUiIndexFile, this->webUiJsFile, this->webUiLogoFile, this->webUiFaviconFile};

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
// Perform an HTTP GET request to a remote page to downloa a file to SPIFFS
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
    const int docSize = 60;

    if (request->hasParam("id"))
    {
      const int stepperIndex = request->getParam("id")->value().toInt();
      if (stepperIndex < 0 || stepperIndex >= ESPServerMaxSteppers || this->configuredSteppers[stepperIndex] == NULL)
      {
        request->send(404);
        return;
      }

      StaticJsonDocument<docSize> doc;
      JsonObject root = doc.to<JsonObject>();
      ESPStepperMotorServer_StepperConfiguration *stepper = this->configuredSteppers[stepperIndex];
      root["mm"] = stepper->getFlexyStepper()->getCurrentPositionInMillimeters();
      root["revs"] = stepper->getFlexyStepper()->getCurrentPositionInRevolutions();
      root["steps"] = stepper->getFlexyStepper()->getCurrentPositionInSteps();
      serializeJson(root, output);
      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
      request->send(response);

      sprintf(this->logString, "ArduinoJSON document size uses %i bytes from alocated %i bytes", doc.memoryUsage(), docSize);
      ESPStepperMotorServer_Logger::logDebug(this->logString);
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
    if (request->hasParam("id"))
    {
      int stepperIndex = request->getParam("id")->value().toInt();
      if (stepperIndex < 0 || stepperIndex >= ESPServerMaxSteppers || this->configuredSteppers[stepperIndex] == NULL)
      {
        request->send(404, "application/json", "{\"error\": \"No stepper configuration found for given id\"}");
        return;
      }
      if (request->hasParam("speed"))
      {
        float speed = request->getParam("speed")->value().toFloat();
        if (speed > 0)
        {
          this->configuredSteppers[stepperIndex]->getFlexyStepper()->setSpeedInStepsPerSecond(speed);
        }
      }
      if (request->hasParam("accell"))
      {
        float accell = request->getParam("accell")->value().toFloat();
        if(accell > 0){
          this->configuredSteppers[stepperIndex]->getFlexyStepper()->setAccelerationInStepsPerSecondPerSecond(accell);
        }
      }

      if (request->hasParam("value") && request->hasParam("unit"))
      {
        String unit = request->getParam("unit")->value();
        float distance = request->getParam("value")->value().toFloat();
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
          request->send(400, "application/json", "{\"error\": \"Invalid unit given. Must be one of: revs, steps, mm\"}");
          return;
        }

        request->send(204);
        return;
      }
    }
    request->send(400, "application/json", "{\"error\": \"Missing id paramter for stepper motor to move\"}");
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

      StaticJsonDocument<300> doc;
      JsonObject root = doc.to<JsonObject>();
      JsonObject stepperDetails = root.createNestedObject("stepper");
      this->populateStepperDetailsToJsonObject(stepperDetails, this->configuredSteppers[stepperIndex], stepperIndex);
      serializeJson(root, output);

      //      Serial.println(doc.memoryUsage());
    }
    else
    {
      const int docSize = 300 * ESPServerMaxSteppers;
      StaticJsonDocument<docSize> doc;
      JsonObject root = doc.to<JsonObject>();
      JsonArray steppers = root.createNestedArray("steppers");
      for (int i = 0; i < ESPServerMaxSteppers; i++)
      {
        JsonObject stepperDetails = steppers.createNestedObject();
        this->populateStepperDetailsToJsonObject(stepperDetails, this->configuredSteppers[i], i);
      }
      serializeJson(root, output);

      //      Serial.println(doc.memoryUsage());
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
  httpServer->on("/api/steppers", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    StaticJsonDocument<300> doc;
    DeserializationError error = deserializeJson(doc, (const char *)data);
    if (error)
    {
      request->send(400, "application/json", "{\"error\": \"Invalid JSON request, deserialization failed\"}");
    } 
    else
    {
      if (doc.containsKey("name") && doc.containsKey("stepPin") && doc.containsKey("dirPin"))
      {
        const char *name = doc["name"];
        int stepPin = doc["stepPin"];
        int dirPin = doc["dirPin"];
        if (stepPin >= 0 && stepPin <= ESPStepperHighestAllowedIoPin && dirPin >= 0 && dirPin <= ESPStepperHighestAllowedIoPin && dirPin != stepPin)
        {
          //check if pins are already in use by a stepper or switch configuration
          if (isIoPinUsed(stepPin)) {
            request->send(400, "application/json", "{\"error\": \"The given STEP IO pin is already used by another stepper or a switch configuration\"}");
          } else if (isIoPinUsed(dirPin)) {
            request->send(400, "application/json", "{\"error\": \"The given DIRECTION IO pin is already used by another stepper or a switch configuration\"}");
          } else {
            ESPStepperMotorServer_StepperConfiguration *stepperToAdd = new ESPStepperMotorServer_StepperConfiguration(stepPin, dirPin);
            stepperToAdd->setDisplayName(name);
            int newId = this->addStepper(stepperToAdd);
            sprintf(this->logString, "{\"id\": %i}", newId);
            request->send(200, "application/json", this->logString);
          }
        }
        else
        {
          request->send(400, "application/json", "{\"error\": \"Invalid IO pin number given or step and dir pin are the same\"}");
        }
      }
      else
      {
        request->send(400, "application/json", "{\"error\": \"Invalid request, missing one ore more required parameters: name, stepPin, dirPin\"}");
      }
    } });

  // PUT /api/steppers?id=<id>
  //optional parameter
  //int id = (request->hasParam("id")) ? std::atoi(request->getParam("id")->value().c_str()) : -1;

  // GET /api/switches
  // GET /api/switches?id=<id>
  // endpoint to list all configured position switches or a specific one if "id" query parameter is given
  httpServer->on("/api/switches", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String output;
    const int switchObjectSize = JSON_OBJECT_SIZE(8) + 80; //80 is for Strings names

    if (request->hasParam("id"))
    {
      int switchIndex = request->getParam("id")->value().toInt();
      if (switchIndex < 0 || switchIndex >= ESPServerMaxSwitches || this->configuredPositionSwitches[switchIndex].ioPinNumber == ESPServerPositionSwitchUnsetPinNumber)
      {
        request->send(404);
        return;
      }

      StaticJsonDocument<switchObjectSize> doc;
      JsonObject root = doc.to<JsonObject>();
      this->populateSwitchDetailsToJsonObject(root, this->configuredPositionSwitches[switchIndex], switchIndex);
      serializeJson(root, output);

      sprintf(this->logString, "ArduinoJSON document size uses %i bytes from alocated %i bytes", doc.memoryUsage(), switchObjectSize);
      ESPStepperMotorServer_Logger::logDebug(this->logString);
    }
    else
    {
      const int docSize = switchObjectSize * ESPServerMaxSwitches;
      StaticJsonDocument<docSize> doc;
      JsonObject root = doc.to<JsonObject>();
      JsonArray switches = root.createNestedArray("switches");
      for (int i = 0; i < ESPServerMaxSwitches; i++)
      {
        if (this->configuredPositionSwitches[i].ioPinNumber != ESPServerPositionSwitchUnsetPinNumber)
        {
          JsonObject switchDetails = switches.createNestedObject();
          this->populateSwitchDetailsToJsonObject(switchDetails, this->configuredPositionSwitches[i], i);
        }
      }
      serializeJson(root, output);

      sprintf(this->logString, "ArduinoJSON document size uses %i bytes from alocated %i bytes", doc.memoryUsage(), docSize);
      ESPStepperMotorServer_Logger::logDebug(this->logString);
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    request->send(response);
  });

  // GET /api/switches/status?id=<id>

  // POST /api/switches
  // endpoint to add a new switch
  httpServer->on("/api/switches", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    StaticJsonDocument<300> doc;
    DeserializationError error = deserializeJson(doc, (const char *)data);
    if (error)
    {
      request->send(400, "application/json", "{\"error\": \"Invalid JSON request, deserialization failed\"}");
    } 
    else
    {
      if (doc.containsKey("stepperId") && doc.containsKey("ioPinNumber") && doc.containsKey("positionName") && doc.containsKey("switchPosition") && doc.containsKey("switchType"))
      {
        byte stepperConfigIndex = doc["stepperId"];
        byte ioPinNumber = doc["ioPinNumber"];
        const char *name = doc["positionName"];
        long switchPosition = doc["switchPosition"];
        byte switchType = doc["switchType"];

        if (ioPinNumber >= 0 && ioPinNumber <= ESPStepperHighestAllowedIoPin)
        {
          //check if pins are already in use by a stepper or switch configuration
          if (isIoPinUsed(ioPinNumber))
          {
            request->send(400, "application/json", "{\"error\": \"The given IO pin is already used by another stepper or a switch configuration\"}");
          }
          else if (this->configuredSteppers[stepperConfigIndex] == NULL)
          {
            request->send(400, "application/json", "{\"error\": \"The given stepper id is invalid, no matching stepper configuration could be found\"}");
          } else {
            positionSwitch posSwitchToAdd;
            posSwitchToAdd.ioPinNumber = ioPinNumber;
            posSwitchToAdd.positionName = name;
            posSwitchToAdd.stepperIndex = stepperConfigIndex;
            posSwitchToAdd.switchPosition = switchPosition;
            posSwitchToAdd.switchType = switchType;
            int newId = this->addPositionSwitch(posSwitchToAdd);
            sprintf(this->logString, "{\"id\": %i}", newId);
            request->send(200, "application/json", this->logString);
          }
        }
        else
        {
          request->send(400, "application/json", "{\"error\": \"Invalid IO pin number given\"}");
        }
      }
      else
      {
        request->send(400, "application/json", "{\"error\": \"Invalid request, missing one ore more required parameters: name, stepPin, dirPin\"}");
      }
    } });

  // PUT /api/switches?id=<id>

  // DELETE /api/switches?id=<id>
  httpServer->on("/api/switches", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
    if (request->params() == 1 && request->getParam(0)->name() == "id")
    {
      int switchIndex = request->getParam(0)->value().toInt();
      if (switchIndex < 0 || switchIndex >= ESPServerMaxSwitches || this->configuredPositionSwitches[switchIndex].ioPinNumber != ESPServerPositionSwitchUnsetPinNumber)
      {
        this->removePositionSwitch(switchIndex);
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

  httpServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(SPIFFS, this->webUiIndexFile);
  });
  httpServer->on("/favicon.ico", HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiFaviconFile, "image/x-icon");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer->on("/dist/build.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiJsFile, "text/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer->on("/dist/logo.png", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(SPIFFS, this->webUiLogoFile);
  });
}

// ---------------------------------------------------------------------------------
//             internal helper functions for stepper communication
// ---------------------------------------------------------------------------------

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

void ESPStepperMotorServer::populateSwitchDetailsToJsonObject(JsonObject &switchDetails, positionSwitch positionSwitch, int index)
{
  switchDetails["id"] = index;
  switchDetails["ioPin"] = positionSwitch.ioPinNumber;
  switchDetails["name"] = positionSwitch.positionName;
  switchDetails["stepperId"] = positionSwitch.stepperIndex;
  switchDetails["stepperName"] = this->configuredSteppers[positionSwitch.stepperIndex]->getDisplayName();
  switchDetails["type"] = positionSwitch.switchType;
  switchDetails["position"] = positionSwitch.switchPosition;
}

void ESPStepperMotorServer::populateStepperDetailsToJsonObject(JsonObject &stepperDetails, ESPStepperMotorServer_StepperConfiguration *stepper, int index)
{
  stepperDetails["id"] = index;

  stepperDetails["configured"] = (stepper == NULL) ? "false" : "true";
  if (stepper != NULL)
  {
    stepperDetails["name"] = stepper->getDisplayName();
    stepperDetails["stepPin"] = stepper->getStepIoPin();
    stepperDetails["dirPin"] = stepper->getDirectionIoPin();
    JsonObject position = stepperDetails.createNestedObject("position");

    position["mm"] = stepper->getFlexyStepper()->getCurrentPositionInMillimeters();
    position["revs"] = stepper->getFlexyStepper()->getCurrentPositionInRevolutions();
    position["steps"] = stepper->getFlexyStepper()->getCurrentPositionInSteps();

    JsonObject stepperStatus = stepperDetails.createNestedObject("velocity");
    stepperStatus["rev_s"] = stepper->getFlexyStepper()->getCurrentVelocityInRevolutionsPerSecond();
    stepperStatus["mm_s"] = stepper->getFlexyStepper()->getCurrentVelocityInMillimetersPerSecond();
    stepperStatus["steps_s"] = stepper->getFlexyStepper()->getCurrentVelocityInStepsPerSecond();

    stepperDetails["stopped"] = stepper->getFlexyStepper()->motionComplete();
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
    ESPStepperMotorServer_Logger::logInfo("Connected to network");
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
        sprintf(this->logString, "Failed to determine IRQ# for given IO Pin %i, thus setting up of interrupt for the position switch failed for pin %s", posSwitch.ioPinNumber, posSwitch.positionName.c_str());
        ESPStepperMotorServer_Logger::logWarning(this->logString);
      }

      sprintf(this->logString, "attaching interrupt service routine for position switch %s on IO Pin %i", posSwitch.positionName.c_str(), posSwitch.ioPinNumber);
      ESPStepperMotorServer_Logger::logDebug(this->logString);
      _BV(irqNum); // clear potentially pending interrupts
      attachInterrupt(irqNum, staticPositionSwitchISR, CHANGE);
    }
  }
}

void ESPStepperMotorServer::detachInterruptForPositionSwitch(positionSwitch *posSwitch)
{
  sprintf(this->logString, "detaching interrupt for position switch %s on IO Pin %i", posSwitch->positionName.c_str(), posSwitch->ioPinNumber);
  ESPStepperMotorServer_Logger::logDebug(this->logString);
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

// ----------------- delegator functions to easy API usage -------------------------

void ESPStepperMotorServer::setLogLevel(byte logLevel)
{
  ESPStepperMotorServer_Logger::setLogLevel(logLevel);
}

// -------------------------------------- End --------------------------------------

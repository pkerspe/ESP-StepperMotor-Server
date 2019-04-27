
//      *********************************************************
//      *                                                       *
//      *     ESP8266 and ESP32 Stepper Motor Server Rest API   *
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

#include "ESPStepperMotorServer_RestAPI.h"

bool initialized = false;
String version = "0.0.1";
ESPStepperMotorServer_Logger *logger;
ESPStepperMotorServer *_stepperMotorServer;
// ---------------------------------------------------------------------------------
//                                  Setup functions
// ---------------------------------------------------------------------------------

//
// constructor for the rest endpoint provider
//
ESPStepperMotorServer_RestAPI::ESPStepperMotorServer_RestAPI(ESPStepperMotorServer *stepperMotorServer)
{
  this->_stepperMotorServer = stepperMotorServer;
  this->logger = new ESPStepperMotorServer_Logger("ESPStepperMotorServer_RestAPI");
  this->logger->logInfo("########## ESPStepperMotorServer_RestAPI instance created ###########");
}

/**
 * this function is used to register all handlers for the rest API endpoints of the ESPStepperMotorServer 
 */
void ESPStepperMotorServer_RestAPI::registerRestEndpoints(AsyncWebServer *httpServer)
{
  // GET /api/status
  httpServer->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->logDebugRequestUrl(request);

    String output = String("");
    this->_stepperMotorServer->getStatusAsJsonString(output);
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    request->send(response);
  });

  // GET /api/steppers/position?id=<id>
  httpServer->on("/api/steppers/position", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->logDebugRequestUrl(request);

    if (request->hasParam("id"))
    {
      const byte stepperIndex = request->getParam("id")->value().toInt();
      if (stepperIndex < 0 || stepperIndex >= ESPServerMaxSteppers || this->_stepperMotorServer->getConfiguredStepper(stepperIndex) == NULL)
      {
        request->send(404, "application/json", "{\"error\": \"Invalid stepper id\"}");
        return;
      }
      String output;
      const int docSize = 60;

      StaticJsonDocument<docSize> doc;
      JsonObject root = doc.to<JsonObject>();
      ESPStepperMotorServer_StepperConfiguration *stepper = this->_stepperMotorServer->getConfiguredStepper(stepperIndex);
      root["mm"] = stepper->getFlexyStepper()->getCurrentPositionInMillimeters();
      root["revs"] = stepper->getFlexyStepper()->getCurrentPositionInRevolutions();
      root["steps"] = stepper->getFlexyStepper()->getCurrentPositionInSteps();
      serializeJson(root, output);
      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
      request->send(response);

      sprintf(this->logger->logString, "ArduinoJSON document size uses %i bytes from alocated %i bytes", doc.memoryUsage(), docSize);
      ESPStepperMotorServer_Logger::logDebug(this->logger->logString);
    }
    else
    {
      request->send(400, "application/json", "{\"error\": \"Missing id paramter\"}");
      return;
    }
  });

  // POST /api/steppers/moveby
  // endpoint to set a new RELATIVE target position for the stepper motor in either mm, revs or steps
  // post parameters: id, unit, value
  httpServer->on("/api/steppers/moveby", HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->logDebugRequestUrl(request);

    if (request->hasParam("id"))
    {
      int stepperIndex = request->getParam("id")->value().toInt();
      if (stepperIndex < 0 || stepperIndex >= ESPServerMaxSteppers || this->_stepperMotorServer->getConfiguredStepper(stepperIndex) == NULL)
      {
        request->send(404, "application/json", "{\"error\": \"No stepper configuration found for given id\"}");
        return;
      }
      if (request->hasParam("speed"))
      {
        float speed = request->getParam("speed")->value().toFloat();
        if (speed > 0)
        {
          this->_stepperMotorServer->getConfiguredStepper(stepperIndex)->getFlexyStepper()->setSpeedInStepsPerSecond(speed);
        }
      }
      if (request->hasParam("accell"))
      {
        float accell = request->getParam("accell")->value().toFloat();
        if (accell > 0)
        {
          this->_stepperMotorServer->getConfiguredStepper(stepperIndex)->getFlexyStepper()->setAccelerationInStepsPerSecondPerSecond(accell);
        }
      }

      if (request->hasParam("value") && request->hasParam("unit"))
      {
        String unit = request->getParam("unit")->value();
        float distance = request->getParam("value")->value().toFloat();
        if (unit == "mm")
        {
          this->_stepperMotorServer->getConfiguredStepper(stepperIndex)->getFlexyStepper()->moveRelativeInMillimeters(distance);
        }
        else if (unit == "revs")
        {
          this->_stepperMotorServer->getConfiguredStepper(stepperIndex)->getFlexyStepper()->moveRelativeInRevolutions(distance);
        }
        else if (unit == "steps")
        {
          this->_stepperMotorServer->getConfiguredStepper(stepperIndex)->getFlexyStepper()->moveRelativeInSteps(distance);
        }
        else
        {
          request->send(400, "application/json", "{\"error\": \"Unit must be one of: revs, steps, mm\"}");
          return;
        }
        request->send(204);
        return;
      }
    }
    request->send(400, "application/json", "{\"error\": \"Missing id paramter\"}");
  });

  // POST /api/steppers/position
  // endpoint to set a new absolute target position for the stepper motor in either mm, revs or steps
  // post parameters: id, unit, value
  httpServer->on("/api/steppers/position", HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->logDebugRequestUrl(request);

    if (request->hasParam("id", true))
    {
      int stepperIndex = request->getParam("id", true)->value().toInt();
      if (stepperIndex < 0 || stepperIndex >= ESPServerMaxSteppers || this->_stepperMotorServer->getConfiguredStepper(stepperIndex) == NULL)
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
          this->_stepperMotorServer->getConfiguredStepper(stepperIndex)->getFlexyStepper()->moveToPositionInMillimeters(position);
        }
        else if (unit == "revs")
        {
          this->_stepperMotorServer->getConfiguredStepper(stepperIndex)->getFlexyStepper()->moveToPositionInRevolutions(position);
        }
        else if (unit == "steps")
        {
          this->_stepperMotorServer->getConfiguredStepper(stepperIndex)->getFlexyStepper()->moveToPositionInSteps(position);
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
    this->logDebugRequestUrl(request);

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
      this->populateStepperDetailsToJsonObject(stepperDetails, this->_stepperMotorServer->getConfiguredStepper(stepperIndex), stepperIndex);
      serializeJson(root, output);
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
        this->populateStepperDetailsToJsonObject(stepperDetails, this->_stepperMotorServer->getConfiguredStepper(i), i);
      }
      serializeJson(root, output);
      //Serial.println(doc.memoryUsage());
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    request->send(response);
  });

  // DELETE /api/steppers?id=<id>
  httpServer->on("/api/steppers", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
    this->logDebugRequestUrl(request);

    if (request->params() == 1 && request->getParam(0)->name() == "id")
    {
      int stepperIndex = request->getParam(0)->value().toInt();
      if (stepperIndex < 0 || stepperIndex >= ESPServerMaxSteppers || this->_stepperMotorServer->getConfiguredStepper(stepperIndex) != NULL)
      {
        this->_stepperMotorServer->removeStepper(stepperIndex);
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
    this->logDebugRequestUrl(request);

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
          if (this->_stepperMotorServer->isIoPinUsed(stepPin))
          {
            request->send(400, "application/json", "{\"error\": \"The given STEP IO pin is already used by another stepper or a switch configuration\"}");
          }
          else if (this->_stepperMotorServer->isIoPinUsed(dirPin))
          {
            request->send(400, "application/json", "{\"error\": \"The given DIRECTION IO pin is already used by another stepper or a switch configuration\"}");
          } else {
            ESPStepperMotorServer_StepperConfiguration *stepperToAdd = new ESPStepperMotorServer_StepperConfiguration(stepPin, dirPin);
            stepperToAdd->setDisplayName(name);
            int newId = this->_stepperMotorServer->addStepper(stepperToAdd);
            sprintf(this->logger->logString, "{\"id\": %i}", newId);
            request->send(200, "application/json", this->logger->logString);
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
    this->logDebugRequestUrl(request);

    String output;
    
    const int switchObjectSize = JSON_OBJECT_SIZE(8) + 80; //80 is for Strings names

    if (request->hasParam("id"))
    {
      int switchIndex = request->getParam("id")->value().toInt();
      if (switchIndex < 0 || switchIndex >= ESPServerMaxSwitches || this->_stepperMotorServer->getConfiguredSwitch(switchIndex)->ioPinNumber == ESPServerPositionSwitchUnsetPinNumber)
      {
        request->send(404);
        return;
      }

      StaticJsonDocument<switchObjectSize> doc;
      JsonObject root = doc.to<JsonObject>();
      this->populateSwitchDetailsToJsonObject(root, this->_stepperMotorServer->getConfiguredSwitch(switchIndex), switchIndex);
      serializeJson(root, output);

      sprintf(this->logger->logString, "ArduinoJSON document size uses %i bytes from alocated %i bytes", doc.memoryUsage(), switchObjectSize);
      ESPStepperMotorServer_Logger::logDebug(this->logger->logString);
    }
    else
    {
      const int docSize = switchObjectSize * ESPServerMaxSwitches;
      StaticJsonDocument<docSize> doc;
      JsonObject root = doc.to<JsonObject>();
      JsonArray switches = root.createNestedArray("switches");
      for (int i = 0; i < ESPServerMaxSwitches; i++)
      {
        if (this->_stepperMotorServer->getConfiguredSwitch(i)->ioPinNumber != ESPServerPositionSwitchUnsetPinNumber)
        {
          JsonObject switchDetails = switches.createNestedObject();
          this->populateSwitchDetailsToJsonObject(switchDetails, this->_stepperMotorServer->getConfiguredSwitch(i), i);
        }
      }
      serializeJson(root, output);

      sprintf(this->logger->logString, "ArduinoJSON document size uses %i bytes from alocated %i bytes", doc.memoryUsage(), docSize);
      ESPStepperMotorServer_Logger::logDebug(this->logger->logString);
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    request->send(response);
  });

  // GET /api/switches/status?id=<id>

  // POST /api/switches
  // endpoint to add a new switch
  httpServer->on("/api/switches", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    this->logDebugRequestUrl(request);
    this->handlePostSwitchRequest(request, data, len, index, total); });

  // PUT /api/switches?id=<id>
  // endpoint to update an existing switch (will effectively delete the old switch configuraiton and write a new one at the same position)
  httpServer->on("/api/switches", HTTP_PUT, [](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    this->logDebugRequestUrl(request);
    int deleteRC = this->handleDeleteSwitchRequest(request, false);
    if (deleteRC == 204)
    {
      this->handlePostSwitchRequest(request, data, len, index, total); 
    } else {
      request->send(deleteRC, "application/json", "{\"error\": \"Failed to update position switch\"}");
    } });

  // DELETE /api/switches?id=<id>
  httpServer->on("/api/switches", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
    this->logDebugRequestUrl(request);
    this->handleDeleteSwitchRequest(request, true);
  });

  // GET /api/outputs
  // GET /api/outputs?id=<id>
  // GET /api/outputs/status?id=<id>
  // PUT /api/outputs/status?id=<id>
  // POST /api/outputs
  // PUT /api/outputs?id=<id>
  // DELETE /api/outputs?id=<id>
}

void ESPStepperMotorServer_RestAPI::populateSwitchDetailsToJsonObject(JsonObject &switchDetails, positionSwitch *positionSwitch, int index)
{
  switchDetails["id"] = index;
  switchDetails["ioPin"] = positionSwitch->ioPinNumber;
  switchDetails["name"] = positionSwitch->positionName;
  switchDetails["stepperId"] = positionSwitch->stepperIndex;
  switchDetails["stepperName"] = this->_stepperMotorServer->getConfiguredStepper(positionSwitch->stepperIndex)->getDisplayName();
  switchDetails["type"] = positionSwitch->switchType;
  switchDetails["position"] = positionSwitch->switchPosition;
}

void ESPStepperMotorServer_RestAPI::populateStepperDetailsToJsonObject(JsonObject &stepperDetails, ESPStepperMotorServer_StepperConfiguration *stepper, int index)
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

void ESPStepperMotorServer_RestAPI::logDebugRequestUrl(AsyncWebServerRequest *request)
{
  if (this->logger->getLogLevel() >= ESPServerLogLevel_DEBUG)
  {
    this->logger->logDebug((String)request->methodToString() + " " + request->url(), false, false);
    int params = request->params();
    for (int i = 0; i < params; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      if (!p->isFile() && !p->isPost())
      {
        this->logger->logDebug(p->name() + "=" + p->value(), false, true);
      }
    }
    this->logger->logDebug(" called", true, true);
  }
}

// request handlers
int ESPStepperMotorServer_RestAPI::handleDeleteSwitchRequest(AsyncWebServerRequest *request, boolean sendReponse)
{
  if (request->hasParam("id"))
  {
    int switchIndex = request->getParam(0)->value().toInt();
    if (switchIndex < 0 || switchIndex >= ESPServerMaxSwitches || this->_stepperMotorServer->getConfiguredSwitch(switchIndex)->ioPinNumber != ESPServerPositionSwitchUnsetPinNumber)
    {
      this->_stepperMotorServer->removePositionSwitch(switchIndex);
      if (sendReponse)
      {
        request->send(204);
      }
      return 204;
    }
  }
  if (sendReponse)
  {
    request->send(404, "application/json", "{\"error\": \"Invalid position switch id\"}");
  }
  return 404;
}

void ESPStepperMotorServer_RestAPI::handlePostSwitchRequest(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total, int switchIndex)
{
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
        if (this->_stepperMotorServer->isIoPinUsed(ioPinNumber))
        {
          request->send(400, "application/json", "{\"error\": \"The given IO pin is already used by another element\"}");
        }
        else if (this->_stepperMotorServer->getConfiguredStepper(stepperConfigIndex) == NULL)
        {
          request->send(404, "application/json", "{\"error\": \"The given stepper id is invalid\"}");
        }
        else
        {
          positionSwitch posSwitchToAdd;
          posSwitchToAdd.ioPinNumber = ioPinNumber;
          posSwitchToAdd.positionName = name;
          posSwitchToAdd.stepperIndex = stepperConfigIndex;
          posSwitchToAdd.switchPosition = switchPosition;
          posSwitchToAdd.switchType = switchType;
          if (switchIndex > -1)
          {
            this->_stepperMotorServer->removePositionSwitch(switchIndex);
          }
          switchIndex = this->_stepperMotorServer->addPositionSwitch(posSwitchToAdd, switchIndex);
          sprintf(this->logger->logString, "{\"id\": %i}", switchIndex);
          request->send(200, "application/json", this->logger->logString);
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
  }
}

// -------------------------------------- End --------------------------------------

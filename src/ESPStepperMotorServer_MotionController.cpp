
//      *********************************************************
//      *                                                       *
//      *     ESP32 Stepper Motor Server -  Motion Controller   *
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

#include <ESPStepperMotorServer_MotionController.h>

//
// constructor for the motion controller module
// creates a freeRTOS Task that runs in the background and triggers the motion updates for the stepper driver
//
ESPStepperMotorServer_MotionController::ESPStepperMotorServer_MotionController(ESPStepperMotorServer *serverRef)
{
  this->serverRef = serverRef;
#ifndef ESPStepperMotorServer_COMPILE_NO_DEBUG
  ESPStepperMotorServer_Logger::logDebug("Motor Controller created");
#endif
}

void ESPStepperMotorServer_MotionController::start()
{
  if (this->xHandle == NULL) //prevent multiple starts
  {
    disableCore0WDT();
    xTaskCreate(
        ESPStepperMotorServer_MotionController::processMotionUpdates, /* Task function. */
        "MotionControl",                                              /* String with name of task. */
        10000,                                                        /* Stack size in bytes. */
        this,                                                         /* Parameter passed as input of the task */
        1,                                                            /* Priority of the task. */
        &this->xHandle);                                              /* Task handle. */
    //esp_task_wdt_delete(this->xHandle);
    ESPStepperMotorServer_Logger::logInfo("Motion Controller task started");
  }
}

void ESPStepperMotorServer_MotionController::processMotionUpdates(void *parameter)
{
  ESPStepperMotorServer_MotionController *ref = static_cast<ESPStepperMotorServer_MotionController *>(parameter);
  ESPStepperMotorServer_Configuration *configuration = ref->serverRef->getCurrentServerConfiguration();
  ESP_FlexyStepper **configuredFlexySteppers = configuration->getConfiguredFlexySteppers();
  bool emergencySwitchFlag = false;
  bool allMovementsCompleted = true;
#ifndef ESPStepperMotorServer_COMPILE_NO_WEB
  int updateCounter = 0;
#endif
  while (true)
  {
    allMovementsCompleted = true;
    //update positions of all steppers / trigger stepping if needed
    for (byte i = 0; i < ESPServerMaxSteppers; i++)
    {
      if (configuredFlexySteppers[i])
      {
        if (!configuredFlexySteppers[i]->processMovement())
        {
          allMovementsCompleted = false;
        }
      }
      else
      {
        break;
      }
    }

    if (allMovementsCompleted && ref->serverRef->_isRebootScheduled)
    {
      //going for reboot since all motion is stopped and reboot has been requested
      Serial.println("Rebooting server now");
      ESP.restart();
    }

    //check for emergency switch
    if (ref->serverRef->emergencySwitchIsActive && !emergencySwitchFlag)
    {
      emergencySwitchFlag = true;
      ESPStepperMotorServer_Logger::logInfo("Emergency Switch triggered");
    }
    else if (!ref->serverRef->emergencySwitchIsActive && emergencySwitchFlag)
    {
      emergencySwitchFlag = false;
    }

#ifndef ESPStepperMotorServer_COMPILE_NO_WEB
    //check if we should send updated position information via websocket
    if (ref->serverRef->isWebserverEnabled)
    {
      updateCounter++;
      //we only send sproadically to reduce load and processing times
      if (updateCounter % 200000 == 0 && ref->serverRef->webSockerServer->count() > 0)
      {
        String positionsString = String("{");
        char segmentBuffer[500];
        bool isFirstSegment = true;
        for (byte n = 0; n < ESPServerMaxSteppers; n++)
        {
          if (configuredFlexySteppers[n])
          {
            if (!isFirstSegment)
            {
              positionsString += ",";
            }
            sprintf(segmentBuffer, "\"s%ipos\":%ld, \"s%ivel\":%.3f", n, configuredFlexySteppers[n]->getCurrentPositionInSteps(), n, configuredFlexySteppers[n]->getCurrentVelocityInStepsPerSecond());
            //maybe register as friendly class and access property directly and save some processing time
            positionsString += segmentBuffer;
            isFirstSegment = false;
          }
        }
        positionsString += "}";

        ref->serverRef->sendSocketMessageToAllClients(positionsString.c_str(), positionsString.length());
        updateCounter = 0;
      }
    }
#endif
  }
}

void ESPStepperMotorServer_MotionController::stop()
{
  vTaskDelete(this->xHandle);
  this->xHandle = NULL;
  ESPStepperMotorServer_Logger::logInfo("Motion Controller stopped");
}

// -------------------------------------- End --------------------------------------


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
  ESPStepperMotorServer_Logger::logDebug("Motor Controller created");
}

void ESPStepperMotorServer_MotionController::start()
{
  if (this->xHandle == NULL) //prevent multuple starts
  {
    xTaskCreate(
        ESPStepperMotorServer_MotionController::processMotionUpdates, /* Task function. */
        "MotionControllerTriggerTask",                                /* String with name of task. */
        10000,                                                        /* Stack size in bytes. */
        this,                                                         /* Parameter passed as input of the task */
        1,                                                            /* Priority of the task. */
        &this->xHandle);                                              /* Task handle. */
    ESPStepperMotorServer_Logger::logInfo("Motion Controller task started");
  }
}

void ESPStepperMotorServer_MotionController::processMotionUpdates(void *parameter)
{
  ESPStepperMotorServer_MotionController *ref = (ESPStepperMotorServer_MotionController *)parameter;
  while (true)
  {
    byte updatedSteppers = 0;
    //TODO create function in Configuration class to return all configured steppers in one call or even all flexystepper instances (that need movement) and maybe even in a "cached" way
    for (byte i = 0; i < ESPServerMaxSteppers; i++)
    {
      ESPStepperMotorServer_StepperConfiguration *stepper = ref->serverRef->getConfiguredStepper(i);
      if (stepper)
      {
        if(!stepper->getFlexyStepper()->processMovement()){
          ESPStepperMotorServer_Logger::logDebugf("Stepper %i position updated", stepper->getId());
        }
        updatedSteppers++;
      }
    }
    delay(50);
    yield();
  }
}

void ESPStepperMotorServer_MotionController::stop()
{
  vTaskDelete(this->xHandle);
  this->xHandle = NULL;
  ESPStepperMotorServer_Logger::logInfo("Motion Controller stopped");
}

// -------------------------------------- End --------------------------------------

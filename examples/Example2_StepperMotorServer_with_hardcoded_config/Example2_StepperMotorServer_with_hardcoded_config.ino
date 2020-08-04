//      *****************************************************
//      *     Example how to configure via code             *
//      *            Paul Kerspe                31.5.2020   *
//      *****************************************************
//
// This example shows how to configure the stepper server via code, to create a hardcoded configuation directly in the firmware.
// this could be useful for example if you want to disable the Webinterface and REST API and solely control the steppers via the command line
// and if you want to bundle a static configuration with the actual firmware to burn to multiple devices
// this example will NOT start the webserver and REST API and will also not connect to a WiFi or open an access point.
// If you intend to create a real server that can be controller via a User Interface or REST API see Example 1.
// For this example you do not need SPIFFS and should not have a config.json uploaded to SPIFFS (since it would be loaded otherwise and possibly interfer with the coded config)
//
// This library requires that your stepper motor be connected to the ESP32
// using an external driver that has a "Step and Direction" interface.
//
// For all driver boards, it is VERY important that you set the motor
// current before running the example. This is typically done by adjusting
// a potentiometer on the board or using dip switches.
// Read the driver board's documentation to learn how to configure the driver
//
// for a detailed manual on how to use this library please visit: https://github.com/pkerspe/ESP-StepperMotor-Server/blob/master/README.md
// ***********************************************************************
#include <Arduino.h>
#include <ESPStepperMotorServer.h>
#include <ESPStepperMotorServer_PositionSwitch.h>
#include <ESPStepperMotorServer_StepperConfiguration.h>
#include <ESP_FlexyStepper.h>

ESPStepperMotorServer *stepperMotorServer;
ESPStepperMotorServer_StepperConfiguration *stepperConfiguration;

#define STEP_PIN 16
#define DIRECTION_PIN 17
#define LIMIT_SWITCH_PIN 18

//this function gets called whenever the stepper reaches the target position / ends the current movement
//it is registerd in the line "stepperConfiguration->getFlexyStepper()->registerTargetPositionReachedCallback(targetPositionReachedCallback);"
void targetPositionReachedCallback(long position)
{
  Serial.printf("Stepper reached target position %ld\n", position);
}

void setup()
{
  Serial.begin(115200);
  // now create a new ESPStepperMotorServer instance (this must be done AFTER the Serial interface has been started)
  // In this example We create the server instance with only the serial command line interface enabled
  stepperMotorServer = new ESPStepperMotorServer(ESPServerSerialEnabled, ESPServerLogLevel_DEBUG);
  stepperMotorServer->setWifiMode(ESPServerWifiModeDisabled);

  //create a new configuration for a stepper
  stepperConfiguration = new ESPStepperMotorServer_StepperConfiguration(STEP_PIN, DIRECTION_PIN);
  stepperConfiguration->setDisplayName("X-Axis");
  //configure the step size and microstepping setup of the drive/motor
  stepperConfiguration->setMicrostepsPerStep(1);
  stepperConfiguration->setStepsPerMM(100);
  stepperConfiguration->setStepsPerRev(200);

  //optional: if your stepper has a physical brake, configure the IO Pin of the brake and the behaviour
  // stepperConfiguration->setBrakeIoPin(20, SWITCHTYPE_STATE_ACTIVE_HIGH_BIT); //set the ESP Pin number where the brake control is connected to an confifgure as active high (brake is engaged when pin goes high)
  // stepperConfiguration->setBrakeEngageDelayMs(0); //instantly engage the brake after the stepper stops
  // stepperConfiguration->setBrakeReleaseDelayMs(-1); //-1 = never reease the brake unless stepper moves

  //OPTIONAL: register a callback handler to get informed whenever the stepper reaches the requested target position
  stepperConfiguration->getFlexyStepper()->registerTargetPositionReachedCallback(targetPositionReachedCallback);

  // now add the configuration to the server
  unsigned int stepperId = stepperMotorServer->addOrUpdateStepper(stepperConfiguration);

  //you can now also add switch and rotary encoder configurations to the server and link them to the steppers id if needed
  //here an example for a limit switch connected to the previously created stepper motor configuration (make sure the pin is not floating (use pull up or pull down resistor if needed))
  ESPStepperMotorServer_PositionSwitch *positionSwitch = new ESPStepperMotorServer_PositionSwitch(LIMIT_SWITCH_PIN, stepperId, SWITCHTYPE_LIMITSWITCH_COMBINED_BEGIN_END_BIT, "Limit Switch");
  stepperMotorServer->addOrUpdatePositionSwitch(positionSwitch);

  //start the server
  stepperMotorServer->start();
  // check the serial console for more details and to send control signals
}

void loop()
{
  //put your custom code here or use the following code to let the stepper motor go back and forth in an endless loop...how useful is that? :-)
  //to move the stepper motor we need to get the ESP FlexyStepper instance for this motor like this:
  ESP_FlexyStepper *flexyStepper = stepperConfiguration->getFlexyStepper();
  flexyStepper->moveRelativeInSteps(100);
  delay(2000);
  flexyStepper->moveRelativeInSteps(-100);
  delay(2000);
  //for more details on the functions of ESP_FlexyStepper see here: https://github.com/pkerspe/ESP-FlexyStepper (chapter "function overview")
}

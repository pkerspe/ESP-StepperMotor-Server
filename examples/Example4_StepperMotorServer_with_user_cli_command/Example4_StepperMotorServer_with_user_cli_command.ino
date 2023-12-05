//      *****************************************************
//      *     Example how to register a new CLI command     *
//      *            Tobias Ellinghaus         05.12.2023   *
//      *****************************************************
//
// This examples show how to register a custom command for the CLI interface.
// This could be useful if you want to control specific GPIO pins or trigger some other control logic.
// This example is based on the hardcoded config example, so all remarks of that apply here, too.
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
ESPStepperMotorServer_CLI *cli_handler;

#define STEP_PIN 16
#define DIRECTION_PIN 17
#define ENABLE_PIN 18

void toggle_enable(char *cmd, char *args)
{
  const int stepperid = cli_handler->getValidStepperIdFromArg(args);
#ifndef ESPStepperMotorServer_COMPILE_NO_DEBUG
  ESPStepperMotorServer_Logger::logDebugf("%s called for stepper id %i\n", cmd, stepperid);
#endif
  if (stepperid > -1)
  {
    char value[20];
    cli_handler->getParameterValue(args, "v", value);
    if (value[0] != '\0')
    {
#ifndef ESPStepperMotorServer_COMPILE_NO_DEBUG
      ESPStepperMotorServer_Logger::logDebugf("enabled called with v = %s\n", value);
#endif

      const int on_off = (String(value).toInt());
      digitalWrite(ENABLE_PIN, on_off ? HIGH : LOW);

      Serial.println(cmd);
    }
    else
    {
      Serial.println("error: missing required v parameter");
    }
  }
}

void setup()
{
  // configure our custom pin as an output and initialize it to LOW
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW);

  Serial.begin(115200);
  // now create a new ESPStepperMotorServer instance (this must be done AFTER the Serial interface has been started)
  // In this example we create the server instance with only the serial command line interface enabled
  stepperMotorServer = new ESPStepperMotorServer(ESPServerSerialEnabled, ESPServerLogLevel_DEBUG);
  stepperMotorServer->setWifiMode(ESPServerWifiModeDisabled);

  // register a new CLI command to set the ENABLE pin
  cli_handler = stepperMotorServer->getCLIHandler();
  cli_handler->registerNewUserCommand({String("enable"), String("e"), String("set the ENABLE signal of a specific stepper. requires the id of the stepper and also the value. E.g. e=0&v:1 to enable the ENABLE input on the stepper with id 0"), true}, &toggle_enable);

  //create a new configuration for a stepper
  stepperConfiguration = new ESPStepperMotorServer_StepperConfiguration(STEP_PIN, DIRECTION_PIN);
  stepperConfiguration->setDisplayName("X-Axis");
  //configure the step size and microstepping setup of the drive/motor
  stepperConfiguration->setMicrostepsPerStep(1);
  stepperConfiguration->setStepsPerMM(100);
  stepperConfiguration->setStepsPerRev(200);

  // now add the configuration to the server
  unsigned int stepperId = stepperMotorServer->addOrUpdateStepper(stepperConfiguration);

  //start the server
  stepperMotorServer->start();
  // check the serial console for more details and to send control signals
}

void loop()
{
  //put your custom code here
}

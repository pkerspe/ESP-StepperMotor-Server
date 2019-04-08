#include <Arduino.h>
#include <ESPStepperMotorServer.h>

FlexyStepper stepper;

ESPStepperMotorServer stepperMotorServer(ESPServerRestApiEnabled | ESPServerWebserverEnabled | ESPServerSerialEnabled);

const char *apName = "ESP-StepperMotor-Server";

#define IO_PIN_HOME_1_SWITCH 16
#define IO_PIN_HOME_2_SWITCH 17
#define IO_PIN_EMERGENCY_SWITCH 15

void setup()
{
  Serial.begin(115200);
  stepperMotorServer.setLogLevel(ESPServerLogLevel_DEBUG);

  stepperMotorServer.setWifiCredentials("HITRON-9480", "AF4LAKIJIM1P");
  stepperMotorServer.wifiClientConnectionTimeoutSeconds = 15;
  stepperMotorServer.setWifiMode(ESPServerWifiModeClient);
  //stepperMotorServer.setWifiMode(ESPServerWifiModeAccessPoint);

  int stepperIndex = stepperMotorServer.addStepper(&stepper);
  stepperMotorServer.addPositionSwitch(stepperIndex, IO_PIN_HOME_1_SWITCH, ESPServerSwitchType_HomingSwitchBegin | ESPServerSwitchType_ActiveLow, "Home-1", 0L);
  int positionSwitchIndex2 = stepperMotorServer.addPositionSwitch(stepperIndex, IO_PIN_HOME_2_SWITCH, ESPServerSwitchType_HomingSwitchEnd | ESPServerSwitchType_ActiveLow, "Home-2", 0L);
  stepperMotorServer.removePositionSwitch(positionSwitchIndex2);
  positionSwitchIndex2 = stepperMotorServer.addPositionSwitch(stepperIndex, IO_PIN_HOME_2_SWITCH, ESPServerSwitchType_HomingSwitchEnd | ESPServerSwitchType_ActiveLow, "Home-2", 0L);

  stepperMotorServer.addPositionSwitch(stepperIndex, IO_PIN_EMERGENCY_SWITCH, ESPServerSwitchType_EmergencyStopSwitch | ESPServerSwitchType_ActiveLow, "STOP", 0L);
  //stepperMotorServer.removePositionSwitch(positionSwitchIndex2);

  stepperMotorServer.start();
}

void loop()
{
  if (stepperMotorServer.emergencySwitchIsActive)
  {
    Serial.println("Emergency Switch triggered");
    stepperMotorServer.emergencySwitchIsActive = false;
  }

  if (stepperMotorServer.positionSwitchUpdateAvailable)
  {
    stepperMotorServer.positionSwitchUpdateAvailable = false;
    stepperMotorServer.printPositionSwitchStatus();
  }
  delay(300);
  // put your main code here, to run repeatedly:
}

//      *********************************************************
//      *                                                       *
//      *     ESP32 Stepper Motor Command Line Interface        *
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

#include <ESPStepperMotorServer_CLI.h>

#define CR '\r'
#define LF '\n'
#define BS '\b'
#define NULLCHAR '\0'
#define COMMAND_BUFFER_LENGTH 50 //length of serial buffer for incoming commands

// ---------------------------------------------------------------------------------
//                                  Setup functions
// ---------------------------------------------------------------------------------

//
// constructor for the command line interface module
// creates a freeRTOS Task that runs in the background and polls the serial interface for input to parse
//
ESPStepperMotorServer_CLI::ESPStepperMotorServer_CLI(ESPStepperMotorServer *serverRef)
{
  this->serverRef = serverRef;
}

void ESPStepperMotorServer_CLI::start()
{
  xTaskCreate(
      ESPStepperMotorServer_CLI::processSerialInput, /* Task function. */
      "SerialInterfacePoller",                       /* String with name of task. */
      10000,                                         /* Stack size in bytes. */
      this,                                          /* Parameter passed as input of the task */
      1,                                             /* Priority of the task. */
      &this->xHandle);                               /* Task handle. */
  this->registerCommands();
  ESPStepperMotorServer_Logger::logInfof("Command Line Interface started, registered %i commands. Type 'help' to get a list of all supported commands\n", this->commandCounter);
}

void ESPStepperMotorServer_CLI::stop()
{
  vTaskDelete(this->xHandle);
  this->xHandle = NULL;
  ESPStepperMotorServer_Logger::logInfo("Command Line Interface stopped");
}

void ESPStepperMotorServer_CLI::executeCommand(String cmd)
{
  char cmdCharArray[cmd.length() + 1];
  strcpy(cmdCharArray, cmd.c_str());
  char *pureCommand = strtok(cmdCharArray, _CMD_PARAM_SEPRATOR);
  char *arguments = strtok(NULL, "=");

  for (int i = 0; i < this->commandCounter; i++)
  {
    if (this->allRegisteredCommands[i].command.equals(pureCommand) || this->allRegisteredCommands[i].shortCut.equals(pureCommand))
    {
      (this->*command_functions[i])(pureCommand, arguments);
      return;
    }
  }
  Serial.printf("error: Command '%s' is unknown\n", cmd.c_str());
}

void ESPStepperMotorServer_CLI::processSerialInput(void *parameter)
{
  ESPStepperMotorServer_CLI *ref = static_cast<ESPStepperMotorServer_CLI *>(parameter);
  char commandLine[COMMAND_BUFFER_LENGTH + 1];
  uint8_t charsRead = 0;
  char buffer[2];
  char c;
  while (true)
  {
    while (Serial.available())
    {
      Serial.readBytes(buffer, 1); //using this step to avoid the annoying warnings of code quality check to Check buffer boundaries
      c = buffer[0];
      switch (c)
      {
      case CR: //likely have full command in buffer now, commands are terminated by CR and/or LS
      case LF:
        commandLine[charsRead] = NULLCHAR; //null terminate our command char array
        if (charsRead > 0)
        {
          charsRead = 0; //charsRead behaves like 'static' in this taks, so have to reset
          try
          {
            ref->executeCommand(String(commandLine));
          }
          catch (...)
          {
            ESPStepperMotorServer_Logger::logWarningf("Caught an exception wil trying to execute command line '%s'\n", commandLine);
          }
        }
        break;
      case BS: // handle backspace in input: put a blank in last char
        if (charsRead > 0)
        { //and adjust commandLine and charsRead
          commandLine[--charsRead] = NULLCHAR;
        }
        break;
      default:
        if (charsRead < COMMAND_BUFFER_LENGTH)
        {
          commandLine[charsRead++] = c;
        }
        commandLine[charsRead] = NULLCHAR; //just in case
        break;
      }
    }
    vTaskDelay(10);
  }
}

// ---------------------------------------------------------------------------------
//                          Command interpreter functions
// ---------------------------------------------------------------------------------

void ESPStepperMotorServer_CLI::registerCommands()
{
  commandDetailsStructure helpCommand = {String("help"), String("h"), String("show a list of all available commands"), false};
  this->registerNewCommand(helpCommand, &ESPStepperMotorServer_CLI::cmdHelp);

  commandDetailsStructure moveByCommand = {String("moveby"), String("mb"), String("move by an specified amount of units. requires the id of the stepper to move, the amount pf movement and also optional the unit for the movement (mm, steps, revs). If no unit is specified steps will be assumed as unit. E.g. mb=0&v=-100&u=mm to move the stepper with id 0 by -100 mm"), true};
  this->registerNewCommand(moveByCommand, &ESPStepperMotorServer_CLI::cmdMoveBy);

  commandDetailsStructure moveToCommand = {String("moveto"), String("mt"), String("move to an absolute position. requires the id of the stepper to move, the amount pf movement and also optional the unit for the movement (mm, steps, revs). If no unit is specified steps will be assumed as unit. E.g. mt=0&v:100&u:revs to move the stepper with id 0 to the absolute position at 100 revolutions"), true};
  this->registerNewCommand(moveToCommand, &ESPStepperMotorServer_CLI::cmdMoveTo);

  commandDetailsStructure configCommand = {String("config"), String("c"), String("print the current configuration to the console as JSON formatted string"), false};
  this->registerNewCommand(configCommand, &ESPStepperMotorServer_CLI::cmdPrintConfig);

  commandDetailsStructure emergencystopCommand = {String("emergencystop"), String("es"), String("trigger emergency stop for all connected steppers. This will clear all target positions and stop the motion controller module immediately. In order to proceed normal operation after this command has been issued, you need to call the revokeemergencystop [res] command"), false};
  this->registerNewCommand(emergencystopCommand, &ESPStepperMotorServer_CLI::cmdEmergencyStop);

  commandDetailsStructure revokeemergencystopCommand = {String("revokeemergencystop"), String("res"), String("revoke a previously triggered emergency stop. This must be called before any motions can proceed after a call to the emergencystop command"), false};
  this->registerNewCommand(revokeemergencystopCommand, &ESPStepperMotorServer_CLI::cmdRevokeEmergencyStop);

  commandDetailsStructure positionCommand = {String("position"), String("p"), String("get the current position of a specific stepper or all steppers if no explicit index is given (e.g. by calling 'pos' or 'pos=&u:mm'). If no parameter for the unit is provided, will return the position in steps. Requires the ID of the stepper to get the position for as parameter and optional the unit using 'u:mm'/'u:steps'/'u:revs'. E.g.: p=0&u:steps to return the current position of stepper with id = 0 with unit 'steps'"), true};
  this->registerNewCommand(positionCommand, &ESPStepperMotorServer_CLI::cmdGetPosition);

  commandDetailsStructure velocityCommand = {String("velocity"), String("v"), String("get the current velocity of a specific stepper or all steppers if no explicit index is given (e.g. by calling 'pos' or 'pos=&u:mm'). If no parameter for the unit is provided, will return the position in steps. Requires the ID of the stepper to get the velocity for as parameter and optional the unit using 'u:mm'/'u:steps'/'u:revs'. E.g.: v=0&u:mm to return the velocity in mm per second of stepper with id = 0"), true};
  this->registerNewCommand(velocityCommand, &ESPStepperMotorServer_CLI::cmdGetCurrentVelocity);

  commandDetailsStructure removeswitchCommand = {String("removeswitch"), String("rsw"), String("remove an existing switch configuration. E.g. rsw=0 to remove the switch with the ID 0"), true};
  this->registerNewCommand(removeswitchCommand, &ESPStepperMotorServer_CLI::cmdRemoveSwitch);

  commandDetailsStructure removestepperCommand = {String("removestepper"), String("rs"), String("remove and existing stepper configuration. E.g. rs=0 to remove the stepper config with the ID 0"), true};
  this->registerNewCommand(removestepperCommand, &ESPStepperMotorServer_CLI::cmdRemoveStepper);

  commandDetailsStructure removeencoderCommand = {String("removeencoder"), String("re"), String("remove an existing rotary encoder configuration. E.g. re=0 to remove the encoder with the ID 0"), true};
  this->registerNewCommand(removeencoderCommand, &ESPStepperMotorServer_CLI::cmdRemoveEncoder);

  commandDetailsStructure rebootCommand = {String("reboot"), String("r"), String("reboot the ESP (config changes that have not been saved will be lost)"), false};
  this->registerNewCommand(rebootCommand, &ESPStepperMotorServer_CLI::cmdReboot);

  commandDetailsStructure saveCommand = {String("save"), String("s"), String("save the current configuration to the SPIFFS in config.json"), false};
  this->registerNewCommand(saveCommand, &ESPStepperMotorServer_CLI::cmdSaveConfiguration);

  commandDetailsStructure stopCommand = {String("stop"), String("st"), String("stop the stepper server (also stops the CLI!)"), false};
  this->registerNewCommand(stopCommand, &ESPStepperMotorServer_CLI::cmdStopServer);

  commandDetailsStructure loglevelCommand = {String("loglevel"), String("ll"), String("set or get the current log level for serial output. valid values to set are: 1 (Warning) - 4 (ALL). E.g. to set to log level DEBUG use ll=3 to get the current loglevel call without parameter"), true};
  this->registerNewCommand(loglevelCommand, &ESPStepperMotorServer_CLI::cmdSetLogLevel);

  commandDetailsStructure serverstatusCommand = {String("serverstatus"), String("ss"), String("print status details of the server as JSON formated string"), false};
  this->registerNewCommand(serverstatusCommand, &ESPStepperMotorServer_CLI::cmdServerStatus);

  commandDetailsStructure switchstatusCommand = {String("switchstatus"), String("pss"), String("print the status of all input switches as JSON formated string"), false};
  this->registerNewCommand(switchstatusCommand, &ESPStepperMotorServer_CLI::cmdSwitchStatus);

  commandDetailsStructure setAPNameCommand = {String("setapname"), String("san"), String("set the name of the access point to be opened up by the esp (if in AP mode)"), true};
  this->registerNewCommand(setAPNameCommand, &ESPStepperMotorServer_CLI::cmdSetApName);

  commandDetailsStructure setApPwdCommand = {String("setappwd"), String("sap"), String("set the password for the access point to be opened by the esp"), true};
  this->registerNewCommand(setApPwdCommand, &ESPStepperMotorServer_CLI::cmdSetApPassword);

  commandDetailsStructure setHttpPortCommand = {String("sethttpport"), String("shp"), String("set the http port to listen for for the web interface"), true};
  this->registerNewCommand(setHttpPortCommand, &ESPStepperMotorServer_CLI::cmdSetHttpPort);

  commandDetailsStructure setSSIDCommand = {String("setwifissid"), String("sws"), String("set the SSID of the WiFi to connect to (if in client mode)"), true};
  this->registerNewCommand(setSSIDCommand, &ESPStepperMotorServer_CLI::cmdSetSSID);

  commandDetailsStructure cmdSetWifiPwdCommand = {String("setwifipwd"), String("swp"), String("set the password of the Wifi network to connect to"), true};
  this->registerNewCommand(cmdSetWifiPwdCommand, &ESPStepperMotorServer_CLI::cmdSetWifiPassword);

  //TODO: implement missing cmd functions
  // this->registerNewCommand("addswitch", "asw", 1, "add a new switch configuration", &ESPStepperMotorServer_CLI::cmdAddSwitch);
  // this->registerNewCommand("addstepper", "as", 1, "add a new stepper configuration", &ESPStepperMotorServer_CLI::cmdAddStepper);
  // this->registerNewCommand("addencoder", "ae", 1, "add a new rotary encoder configuration", &ESPStepperMotorServer_CLI::cmdAddEncoder);
  // this->registerNewCommand("listencoders", "le", 0, "list all configured rotary encoders", &ESPStepperMotorServer_CLI::cmdListEncoders);
  // this->registerNewCommand("liststeppers", "ls", 0, "list all configured steppers", &ESPStepperMotorServer_CLI::cmdListSteppers);
  // this->registerNewCommand("listswitches", "lsw", 0, "list all configured input switches", &ESPStepperMotorServer_CLI::cmdListSwitches);
  // this->registerNewCommand("returnhome", "rh", 1, "Return to the home position (only possible if a homing switch is connected). Requires the ID of the stepper to get the position for as parameter. E.g.: rh=0", &ESPStepperMotorServer_CLI::cmdReturnHome);
  // set wifi ssid
  // set wifi pwd
  // set wifi mode
  // set http port
  // set ap name
  // set ap password
}

void ESPStepperMotorServer_CLI::registerNewCommand(commandDetailsStructure commandDetails, void (ESPStepperMotorServer_CLI::*cmdFunction)(char *, char *))
{
  if (this->commandCounter < MAX_CLI_CMD_COUNTER)
  {
    //check if command is already registered
    for (int i = 0; i < this->commandCounter; i++)
    {
      if (this->allRegisteredCommands[i].command.equals(commandDetails.command) || this->allRegisteredCommands[i].shortCut.equals(commandDetails.shortCut))
      {
        ESPStepperMotorServer_Logger::logWarningf("A command with the same name / shortcut is already registered. Will not add the command '%s' [%s] to the list of registered commands", commandDetails.command, commandDetails.shortCut);
        return;
      }
    }
    this->allRegisteredCommands[this->commandCounter] = commandDetails;
    this->command_functions[this->commandCounter] = cmdFunction;
    this->commandCounter++;
  }
  else
  {
    ESPStepperMotorServer_Logger::logWarningf("The maximum number of CLI commands has been exceeded. You need to increase the MAX_CLI_CMD_COUNTER value to add more then %i commands\n", MAX_CLI_CMD_COUNTER);
  }
}

void ESPStepperMotorServer_CLI::cmdPrintConfig(char *cmd, char *args)
{
  this->serverRef->getCurrentServerConfiguration()->printCurrentConfigurationAsJsonToSerial();
}

const char *setterConfirmationTemplate = "%s set to %s (please save and reboot for the changes to take effect)\n";
const char *setterMissingParameterTemplate = "No or invalid value given as parameter. Usage is %s=<value>\n";

void ESPStepperMotorServer_CLI::cmdSetApName(char *cmd, char *args)
{
  if (args != NULL)
  {
    this->serverRef->setAccessPointName(args);
    Serial.printf(setterConfirmationTemplate, "AP name", args);
  }
  else
  {
    Serial.printf(setterMissingParameterTemplate, cmd);
  }
}

void ESPStepperMotorServer_CLI::cmdSetApPassword(char *cmd, char *args)
{
  if (args != NULL)
  {
    this->serverRef->setAccessPointPassword(args);
    Serial.printf(setterConfirmationTemplate, "AP password", args);
  }
  else
  {
    Serial.printf(setterMissingParameterTemplate, cmd);
  }
}

void ESPStepperMotorServer_CLI::cmdSetHttpPort(char *cmd, char *args)
{
  int port = (int)(String(args)).toInt();
  if (port >= 80)
  {
    this->serverRef->setHttpPort(port);
    Serial.printf(setterConfirmationTemplate, "HTTP port", args);
  }
  else
  {
    Serial.printf(setterMissingParameterTemplate, cmd);
  }
}

void ESPStepperMotorServer_CLI::cmdSetSSID(char *cmd, char *args)
{
  if (args != NULL)
  {
    this->serverRef->setWifiSSID(args);
    Serial.printf(setterConfirmationTemplate, "WiFi SSID", args);
  }
  else
  {
    Serial.printf(setterMissingParameterTemplate, cmd);
  }
}

void ESPStepperMotorServer_CLI::cmdSetWifiPassword(char *cmd, char *args)
{
  if (args != NULL)
  {
    this->serverRef->setWifiPassword(args);
    Serial.printf(setterConfirmationTemplate, "WiFi password", args);
  }
  else
  {
    Serial.printf(setterMissingParameterTemplate, cmd);
  }
}

void ESPStepperMotorServer_CLI::cmdHelp(char *cmd, char *args)
{
  Serial.println("\n-------- ESP-StepperMotor-Server-CLI Help -----------\nThe following commands are available:\n");
  Serial.println("<command> [<shortcut>]: <description>");
  for (int i = 0; i < this->commandCounter; i++)
  {
    const char *hint = this->allRegisteredCommands[i].hasParameters ? "*" : "";
    const char *tabString = (this->allRegisteredCommands[i].command.length() + this->allRegisteredCommands[i].shortCut.length() < 12) ? "\t" : "";
    Serial.printf("%s [%s]%s:\t%s%s\n", this->allRegisteredCommands[i].command.c_str(), this->allRegisteredCommands[i].shortCut.c_str(), hint, tabString, this->allRegisteredCommands[i].description.c_str());
  }
  Serial.println("\ncommmands marked with a * require input parameters.\nParameters are provided with the command separarted by a = for the primary parameter.\nSecondary parameters are provided in the format '&<parametername>:<parametervalue>'\n");
  Serial.println("-------------------------------------------------------");
}

void ESPStepperMotorServer_CLI::cmdReboot(char *cmd, char *args)
{
  Serial.println("initiating restart");
  ESP.restart();
}

void ESPStepperMotorServer_CLI::cmdSwitchStatus(char *cmd, char *args)
{
  this->serverRef->printPositionSwitchStatus();
}

void ESPStepperMotorServer_CLI::cmdServerStatus(char *cmd, char *args)
{
  String result;
  this->serverRef->getServerStatusAsJsonString(result);
  Serial.println(result);
}

void ESPStepperMotorServer_CLI::cmdStopServer(char *cmd, char *args)
{
  this->serverRef->stop();
  Serial.println(cmd);
}

void ESPStepperMotorServer_CLI::cmdEmergencyStop(char *cmd, char *args)
{
  this->serverRef->performEmergencyStop();
  Serial.println(cmd);
}

void ESPStepperMotorServer_CLI::cmdRevokeEmergencyStop(char *cmd, char *args)
{
  this->serverRef->revokeEmergencyStop();
  Serial.println(cmd);
}

void ESPStepperMotorServer_CLI::cmdRemoveSwitch(char *cmd, char *args)
{
  unsigned int id = (String(args)).toInt();
  if (id > ESPServerMaxSwitches || !this->serverRef->getCurrentServerConfiguration()->getSwitch((byte)id))
  {
    Serial.println("error: invalid switch id given");
  }
  else
  {
    this->serverRef->removePositionSwitch((byte)id);
    Serial.println(cmd);
  }
}

void ESPStepperMotorServer_CLI::cmdRemoveStepper(char *cmd, char *args)
{
  int stepperid = this->getValidStepperIdFromArg(args);
  if (stepperid > -1)
  {
    this->serverRef->removeStepper((byte)stepperid);
    Serial.println(cmd);
  }
}

void ESPStepperMotorServer_CLI::cmdRemoveEncoder(char *cmd, char *args)
{
  unsigned int id = (String(args)).toInt();
  if (id > ESPServerMaxRotaryEncoders || !this->serverRef->getCurrentServerConfiguration()->getRotaryEncoder((byte)id))
  {
    Serial.println("error: invalid encoder id given");
  }
  else
  {
    this->serverRef->removeRotaryEncoder((byte)id);
    Serial.println(cmd);
  }
}

void ESPStepperMotorServer_CLI::cmdGetCurrentVelocity(char *cmd, char *args)
{
  ESPStepperMotorServer_Configuration *config = this->serverRef->getCurrentServerConfiguration();
  int stepperid = this->getValidStepperIdFromArg(args);
  char unit[10];
  this->getUnitWithFallback(args, unit);

  if (stepperid > -1)
  {
    if (strcmp(unit, "mm") == 0)
      Serial.printf("%f mm/s\n", config->getStepperConfiguration((byte)stepperid)->getFlexyStepper()->getCurrentVelocityInMillimetersPerSecond());
    else if (strcmp(unit, "revs") == 0)
      Serial.printf("%f revs/s\n", config->getStepperConfiguration((byte)stepperid)->getFlexyStepper()->getCurrentVelocityInRevolutionsPerSecond());
    else
      Serial.printf("%f steps/s\n", config->getStepperConfiguration((byte)stepperid)->getFlexyStepper()->getCurrentVelocityInStepsPerSecond());
  }
  else
  {
    ESPStepperMotorServer_Logger::logDebugf("%s called without parameter for stepper index\n", cmd);
    for (stepperid = 0; stepperid < ESPServerMaxSteppers; stepperid++)
    {
      ESPStepperMotorServer_StepperConfiguration *stepper = this->serverRef->getCurrentServerConfiguration()->getStepperConfiguration(stepperid);
      if (stepper)
      {
        if (strcmp(unit, "mm") == 0)
          Serial.printf("%i:%f mm/s\n", stepperid, stepper->getFlexyStepper()->getCurrentVelocityInMillimetersPerSecond());
        else if (strcmp(unit, "revs") == 0)
          Serial.printf("%i:%f revs/s\n", stepperid, stepper->getFlexyStepper()->getCurrentVelocityInRevolutionsPerSecond());
        else
          Serial.printf("%i:%f steps/s\n", stepperid, stepper->getFlexyStepper()->getCurrentVelocityInStepsPerSecond());
      }
    }
  }
}

void ESPStepperMotorServer_CLI::cmdGetPosition(char *cmd, char *args)
{
  ESPStepperMotorServer_Configuration *config = this->serverRef->getCurrentServerConfiguration();
  int stepperid = this->getValidStepperIdFromArg(args);
  char unit[10];
  this->getUnitWithFallback(args, unit);

  if (stepperid > -1)
  {
    if (strcmp(unit, "mm") == 0)
      Serial.printf("%f mm\n", config->getStepperConfiguration((byte)stepperid)->getFlexyStepper()->getCurrentPositionInMillimeters());
    else if (strcmp(unit, "revs") == 0)
      Serial.printf("%f revs\n", config->getStepperConfiguration((byte)stepperid)->getFlexyStepper()->getCurrentPositionInRevolutions());
    else
      Serial.printf("%ld steps\n", config->getStepperConfiguration((byte)stepperid)->getFlexyStepper()->getCurrentPositionInSteps());
  }
  else
  {
    ESPStepperMotorServer_Logger::logDebugf("%s called without parameter for stepper index\n", cmd);
    for (stepperid = 0; stepperid < ESPServerMaxSteppers; stepperid++)
    {
      ESPStepperMotorServer_StepperConfiguration *stepper = config->getStepperConfiguration(stepperid);
      if (stepper)
      {
        if (strcmp(unit, "mm") == 0)
          Serial.printf("%i:%f mm\n", stepperid, stepper->getFlexyStepper()->getCurrentPositionInMillimeters());
        else if (strcmp(unit, "revs") == 0)
          Serial.printf("%i:%f revs\n", stepperid, stepper->getFlexyStepper()->getCurrentPositionInRevolutions());
        else
          Serial.printf("%i:%ld steps\n", stepperid, stepper->getFlexyStepper()->getCurrentPositionInSteps());
      }
    }
  }
}

void ESPStepperMotorServer_CLI::cmdMoveTo(char *cmd, char *args)
{
  int stepperid = this->getValidStepperIdFromArg(args);
  if (stepperid > -1)
  {
    char value[20];
    this->getParameterValue(args, "v", value);
    if (value[0] != NULLCHAR)
    {
      char unit[10];
      this->getParameterValue(args, "u", unit);
      if (unit[0] == NULLCHAR || strcmp(unit, "steps") == 0)
      {
        if (unit[0] == NULLCHAR)
        {
          Serial.println("no unit provided, will use 'steps' as default");
        }
        this->serverRef->getCurrentServerConfiguration()->getStepperConfiguration(stepperid)->getFlexyStepper()->setTargetPositionInSteps((String(value).toInt()));
      }
      else if (strcmp(unit, "revs") == 0)
      {
        this->serverRef->getCurrentServerConfiguration()->getStepperConfiguration(stepperid)->getFlexyStepper()->setTargetPositionInRevolutions((String(value).toFloat()));
      }
      else if (strcmp(unit, "mm") == 0)
      {
        this->serverRef->getCurrentServerConfiguration()->getStepperConfiguration(stepperid)->getFlexyStepper()->setTargetPositionInMillimeters((String(value).toFloat()));
      }
      else
      {
        Serial.println("error: provided unit not supported. Must be one of mm, steps or revs");
        return;
      }
      Serial.println(cmd);
    }
    else
    {
      Serial.println("error: missing required v parameter");
    }
  }
}

void ESPStepperMotorServer_CLI::cmdMoveBy(char *cmd, char *args)
{
  int stepperid = this->getValidStepperIdFromArg(args);
  ESPStepperMotorServer_Logger::logDebugf("%s called for stepper id %i\n", cmd, stepperid);
  if (stepperid > -1)
  {
    char value[20];
    this->getParameterValue(args, "v", value);
    if (value[0] != NULLCHAR)
    {
      ESPStepperMotorServer_Logger::logDebugf("cmdMoveBy called with v = %s\n", value);
      char unit[10];
      this->getParameterValue(args, "u", unit);
      if (unit[0] == NULLCHAR || strcmp(unit, "steps") == 0)
      {
        if (unit[0] == NULLCHAR)
        {
          Serial.println("no unit provided, will use 'steps' as default");
        }
        int targetPosition = (String(value).toInt());
        ESPStepperMotorServer_Logger::logDebugf("Setting target position to %i steps\n", targetPosition);
        this->serverRef->getCurrentServerConfiguration()->getStepperConfiguration(stepperid)->getFlexyStepper()->setTargetPositionRelativeInSteps(targetPosition);
      }
      else if (strcmp(unit, "revs") == 0)
      {
        float targetPosition = (String(value).toFloat());
        ESPStepperMotorServer_Logger::logDebugf("Setting target position to %f revs\n", targetPosition);
        this->serverRef->getCurrentServerConfiguration()->getStepperConfiguration(stepperid)->getFlexyStepper()->setTargetPositionRelativeInRevolutions(targetPosition);
      }
      else if (strcmp(unit, "mm") == 0)
      {
        float targetPosition = (String(value).toFloat());
        ESPStepperMotorServer_Logger::logDebugf("Setting target position to %f mm\n", targetPosition);
        this->serverRef->getCurrentServerConfiguration()->getStepperConfiguration(stepperid)->getFlexyStepper()->setTargetPositionRelativeInMillimeters(targetPosition);
      }
      else
      {
        Serial.println("error: provided unit not supported. Must be one of mm, steps or revs");
        return;
      }
      Serial.println(cmd);
    }
    else
    {
      Serial.println("error: missing required v parameter");
    }
  }
}

void ESPStepperMotorServer_CLI::cmdSaveConfiguration(char *cmd, char *args)
{
  if (this->serverRef->getCurrentServerConfiguration()->saveCurrentConfiguationToSpiffs())
  {
    Serial.println(cmd);
  }
  else
  {
    Serial.println("error: saving configuration to SPIFFS failed");
  }
}

void ESPStepperMotorServer_CLI::cmdSetLogLevel(char *cmd, char *args)
{
  unsigned int logLevelToSet = (String(args)).toInt();
  if (logLevelToSet == 0)
  {
    Serial.printf("%s=%i\n", cmd, ESPStepperMotorServer_Logger::getLogLevel());
  }
  else
  {
    if (logLevelToSet > ESPServerLogLevel_ALL || logLevelToSet < ESPServerLogLevel_WARNING)
    {
      Serial.printf("error: Invalid log level given. Must be in the range of %i (Warning) and %i (All)\n", ESPServerLogLevel_WARNING, ESPServerLogLevel_ALL);
    }
    ESPStepperMotorServer_Logger::setLogLevel(logLevelToSet);
  }
}

/**
 * convert given char* to int value and check if it represents a valid stepper config id (within the allowed limits and with an existing stepper configuation existing)
 * -1 is returned if not valid and an error os printed to the serial interface
 */
int ESPStepperMotorServer_CLI::getValidStepperIdFromArg(char *arg)
{
  if (arg && isdigit(arg[0]))
  {
    int id = (String(arg)).toInt();
    ESPStepperMotorServer_Logger::logDebugf("extracted stepper id %i from argument string %s\n", id, arg);
    if (id > ESPServerMaxSteppers || !this->serverRef->getCurrentServerConfiguration()->getStepperConfiguration((byte)id))
    {
      Serial.println("error: invalid stepper id given");
      return -1;
    }
    return (byte)id;
  }
  else
  {
    ESPStepperMotorServer_Logger::logDebug("no argument string given to extract stepper id from, will return -1");
  }
  return -1;
}

/**
 * helper function to extract parameters and values from the given argument string
 */
void ESPStepperMotorServer_CLI::getParameterValue(const char *args, const char *parameterNameToGetValueFor, char *result)
{
  if (args == NULL)
  {
    ESPStepperMotorServer_Logger::logDebugf("getParameterValue called with on NULL and %s\n", parameterNameToGetValueFor);
    strcpy(result, "");
    return;
  }

  ESPStepperMotorServer_Logger::logDebugf("getParameterValue called with %s and %s\n", args, parameterNameToGetValueFor);
  char *parameterValue;
  char *save_ptr;
  String argumentString = String(args);
  char workingCopy[argumentString.length() + 1];
  strcpy(workingCopy, args);

  char *keyValuePairString = strtok_r(workingCopy, this->_PARAM_PARAM_SEPRATOR, &save_ptr);
  while (keyValuePairString != NULL)
  {
    ESPStepperMotorServer_Logger::logDebugf("Found a key value pair: %s\n", keyValuePairString);
    char *restKeyValuePair = keyValuePairString;
    char *parameterName = strtok_r(restKeyValuePair, this->_PARAM_VALUE_SEPRATOR, &restKeyValuePair);
    if (parameterName != NULL)
    {
      ESPStepperMotorServer_Logger::logDebugf("Found parameter '%s'\n", parameterName);
      if (strcmp(parameterName, parameterNameToGetValueFor) == 0)
      {
        if (restKeyValuePair && strcmp(restKeyValuePair, "") != 0)
        {
          parameterValue = strtok_r(restKeyValuePair, this->_PARAM_VALUE_SEPRATOR, &restKeyValuePair);
          ESPStepperMotorServer_Logger::logDebugf("Found matching parameter: %s with value %s\n", parameterName, parameterValue);
          strcpy(result, parameterValue);
          return;
        }
      }
      else
      {
        ESPStepperMotorServer_Logger::logDebug("parameter did not match requested parameter");
      }
    }
    keyValuePairString = strtok_r(NULL, this->_PARAM_PARAM_SEPRATOR, &save_ptr);
  }
  ESPStepperMotorServer_Logger::logDebug("No match found");
  strcpy(result, "");
}

////// internal helpers to prevent code duplication
void ESPStepperMotorServer_CLI::getUnitWithFallback(char *args, char *unit)
{
  this->getParameterValue(args, "u", unit);
  if (unit[0] == NULLCHAR)
  {
    ESPStepperMotorServer_Logger::logDebug("no unit provided, will use 'steps' as default");
    strcpy(unit, "steps");
  }
}

// -------------------------------------- End --------------------------------------

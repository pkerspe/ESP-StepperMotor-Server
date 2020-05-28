
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

#include "ESPStepperMotorServer_CLI.h"

#define CR '\r'
#define LF '\n'
#define BS '\b'
#define NULLCHAR '\0'
#define SPACE ' '
#define COMMAND_BUFFER_LENGTH 25 //length of serial buffer for incoming commands

// ---------------------------------------------------------------------------------
//                                  Setup functions
// ---------------------------------------------------------------------------------

//
// constructor for the command line interface module
// creates a freeRTOS Task that runs in the background and polls the serial interface for input to parse
//
ESPStepperMotorServer_CLI::ESPStepperMotorServer_CLI()
{
  this->registerCommands();
  ESPStepperMotorServer_Logger::logInfof("Command Line Interface created, registered %i commands, Type 'help' to get a list of all supported commands", this->commandCounter);
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
  ESPStepperMotorServer_Logger::logInfo("Command Line Interface polling task started");
}

void ESPStepperMotorServer_CLI::executeCommand(String cmd)
{
  Serial.printf("received command from serial: %s\n", cmd.c_str());
  for (int i = 0; i < this->commandCounter; i++)
  {
    if (strcmp(cmd.c_str(),this->command_details[i][0]) == 0)
    {
      Serial.println("found command");
      (this->*command_functions[i])(cmd);
      return;
    }
    
  }
  Serial.printf("Command '%s' is unknown\n", cmd.c_str());
}

void ESPStepperMotorServer_CLI::stop()
{
  vTaskDelete(this->xHandle);
  ESPStepperMotorServer_Logger::logInfo("Command Line Interface stopped");
}

void ESPStepperMotorServer_CLI::processSerialInput(void *parameter)
{
  ESPStepperMotorServer_CLI *ref = (ESPStepperMotorServer_CLI *)parameter;
  char commandLine[COMMAND_BUFFER_LENGTH + 1];
  uint8_t charsRead = 0;
  while (true)
  {
    while (Serial.available())
    {
      char c = Serial.read();
      switch (c)
      {
      case CR: //likely have full command in buffer now, commands are terminated by CR and/or LS
      case LF:
        commandLine[charsRead] = NULLCHAR; //null terminate our command char array
        if (charsRead > 0)
        {
          charsRead = 0; //charsRead is static, so have to reset
          ref->executeCommand(String(commandLine));
        }
        break;
      case BS: // handle backspace in input: put a space in last char
        if (charsRead > 0)
        { //and adjust commandLine and charsRead
          commandLine[--charsRead] = NULLCHAR;
          //Serial << byte(BS) << byte(SPACE) << byte(BS); //no idea how this works, found it on the Internet
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
    delay(100);
  }
}

// ---------------------------------------------------------------------------------
//                          Command interpreter functions
// ---------------------------------------------------------------------------------

void ESPStepperMotorServer_CLI::registerCommands()
{
  this->registerNewCommand("help", "show the help screen", &ESPStepperMotorServer_CLI::cmdHelp);
  this->registerNewCommand("reboot", "reboot the ESP and start all over", &ESPStepperMotorServer_CLI::cmdReboot);
  this->registerNewCommand("switchStatus", "print the status of all input switches", &ESPStepperMotorServer_CLI::cmdSwitchStatus);
}

void ESPStepperMotorServer_CLI::registerNewCommand(const char cmd[], const char description[], void (ESPStepperMotorServer_CLI::*cmdFunction)(String))
{
  if (this->commandCounter < MAX_CLI_CMD_COUNTER)
  {
    this->command_details[this->commandCounter][0] = cmd;
    this->command_details[this->commandCounter][1] = description;
    this->command_functions[this->commandCounter] = cmdFunction;
    this->commandCounter++;
  }
  else
  {
    ESPStepperMotorServer_Logger::logWarningf("The maximum number of CLI commands has been exceeded. You need to increase the MAX_CLI_CMD_COUNTER value to add more then %i commands\n", MAX_CLI_CMD_COUNTER);
  }
}

void ESPStepperMotorServer_CLI::cmdHelp(String args)
{
  Serial.println("The following commands are available");
  for (int i = 0; i < this->commandCounter; i++)
  {
    Serial.printf("%s: %s\n", this->command_details[i][0], this->command_details[i][1]);
  }
}

void ESPStepperMotorServer_CLI::cmdReboot(String args)
{
  Serial.println("initiating restart");
  ESP.restart();
}
void ESPStepperMotorServer_CLI::cmdSwitchStatus(String args)
{
  Serial.println("Switch status: 0000000");
}

// -------------------------------------- End --------------------------------------

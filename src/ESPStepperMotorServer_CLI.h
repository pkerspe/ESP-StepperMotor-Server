//      ******************************************************************
//      *                                                                *
//      *       Header file for ESPStepperMotorServer_CLI.cpp            *
//      *                                                                *
//      *               Copyright (c) Paul Kerspe, 2019                  *
//      *                                                                *
//      ******************************************************************

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

#ifndef ESPStepperMotorServer_CLI_h
#define ESPStepperMotorServer_CLI_h

#define MAX_CLI_CMD_COUNTER 50

#include <ESPStepperMotorServer.h>
#include <ESPStepperMotorServer_Logger.h>

//need this forward declaration here due to circular dependency (use in constructor/member variable)
class ESPStepperMotorServer;

struct commandDetailsStructure {
  String command;
  String shortCut;
  String description;
  bool hasParameters;
};


class ESPStepperMotorServer_CLI
{
public:
  ESPStepperMotorServer_CLI(ESPStepperMotorServer *serverRef);
  ~ESPStepperMotorServer_CLI();
  static void processSerialInput(void *parameter);
  void executeCommand(String cmd);
  void start();
  void stop();

private:
  void cmdHelp(char *cmd, char *args);
  void cmdEmergencyStop(char *cmd, char *args);
  void cmdRevokeEmergencyStop(char *cmd, char *args);
  void cmdGetPosition(char *cmd, char *args);
  void cmdGetCurrentVelocity(char *cmd, char *args);
  void cmdMoveBy(char *cmd, char *args);
  void cmdMoveTo(char *cmd, char *args);
  void cmdPrintConfig(char *cmd, char *args);
  void cmdRemoveSwitch(char *cmd, char *args);
  void cmdReboot(char *cmd, char *args);
  void cmdRemoveStepper(char *cmd, char *args);
  void cmdRemoveEncoder(char *cmd, char *args);
  void cmdStopServer(char *cmd, char *args);
  void cmdSwitchStatus(char *cmd, char *args);
  void cmdServerStatus(char *cmd, char *args);
  void cmdSetLogLevel(char *cmd, char *args);
  void cmdSaveConfiguration(char *cmd, char *args);
  void cmdSetApName(char *cmd, char *args);
  void cmdSetApPassword(char *cmd, char *args);
  void cmdSetHttpPort(char *cmd, char *args);
  void cmdSetSSID(char *cmd, char *args);
  void cmdSetWifiPassword(char *cmd, char *args);
  int getValidStepperIdFromArg(char *arg);
  void getParameterValue(const char * args, const char* parameterNameToGetValueFor, char* result);
  void registerCommands();
  void registerNewCommand(commandDetailsStructure commandDetails, void (ESPStepperMotorServer_CLI::*f)(char *, char*));
  void getUnitWithFallback(char *args, char *unit);

  TaskHandle_t xHandle = NULL;
  ESPStepperMotorServer *serverRef;
  void (ESPStepperMotorServer_CLI::*command_functions[MAX_CLI_CMD_COUNTER + 1])(char *, char *);
  //const char *command_details[MAX_CLI_CMD_COUNTER +1 ][4];
  commandDetailsStructure allRegisteredCommands[MAX_CLI_CMD_COUNTER +1];
  unsigned int commandCounter = 0;

  const char* _CMD_PARAM_SEPRATOR = "=";
  const char* _PARAM_PARAM_SEPRATOR = "&";
  const char* _PARAM_VALUE_SEPRATOR = ":";

};

#endif
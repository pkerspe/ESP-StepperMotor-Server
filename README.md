# ESP-StepperMotor-Server - A complete stepper motor control server running on ESP32 modules

Turn your ESP32 into a complete stepper motor control server.
Connect one ore more stepper controllers with step and direction input, and optionally some limit-switches to the IO-pins of your ESP module and controll the stepper motor via an easy to use web interface, via REST API or via a serial control interface.

## Introduction

This library started as a fork for the FlexyStepper library (https://github.com/Stan-Reifel/FlexyStepper). While the FlexyStepper Library is a general Arduino compatible library this fork has a focus on the ESP32 modules form Espressif. It also became much more than a modfied version of FlexyStepper but turned into a complete application to turn a regular ESP32 module into a complete stepper server.
Since these modules contain a WiFi module they are perfectly suited for web controlled stepper server and since they have enough memory and processing power they are ideal as low cost, low energy consumption standalone server component, that allows configuration and controlling of one to many stepper motor drivers with limit-switches and outputs (e.g. for Relays and LEDs).

Once the ESPp Stepper Motor Server has been uploaded to the ESP module, all further configuration and controlling can be done vie the web UI without the need to code another line in the Arduino or PlatformIO IDE.

### What this library is NOT

This library is not ideal if you are looking for a solution to control your CNC Router. It does not support Gerber commands (GRBL) in general or parallel, synchronized multi axis movements.
If you need such a solution you might want to look into the [Grbl_Esp32](https://github.com/bdring/Grbl_Esp32) (for ESP32 specifically) or [grbl](https://github.com/gnea/grbl) (for Arduino in general) Libraries. If you are looking for an easy way to setup and control one or more stepper motors independly and ading limit switches and rotary encoders to conrol them, this project here might be just what you are looking for. 

## Prerequisites / Dependencies:

In order to compile your project with the ESP-StepperMotor-Server Library, the following 3rd party extensions need to be installed on your system (if you use PlatformIO all needed dependencies will be installed for you when you follow the instructions in the [Using PlatformIO](#using-platformio) section above):
- ESPAsyncWebserver: https://github.com/me-no-dev/ESPAsyncWebServer
- AsyncTCP: https://github.com/me-no-dev/AsyncTCP
- ArduinoJSON (NOTE: must version 6.x, version 5 will not work): https://arduinojson.org/
- FlexyStepper: https://github.com/Stan-Reifel/FlexyStepper
- FS file system wrapper: should be installed with the ESP32 libraries already if you setup your IDE for these modules)
- WiFi: should be installed with the ESP32 libraries already when you setup your IDE for these modules

When using PlatformIO add these dependencies to you platfromio.ini project file and let PlatfromIO install the required dependencies for you:
```ini
lib_deps = ESP-StepperMotor-Server
```

When using Arduino you need to install these libraries using the Library Manager.

If you use PlatformIO you can simply setup your project with the provided paltformio.ini file in this repository

## Setting up your ESP-StepperMotor-Server

In order to get started you need the following elements:
- A ESP32 board of your choice (boards with USB-Serial Chips are recommended for the ease of programming them, ohter boards just work as well, yet you have to figure out how to flash the firmware yourself, since this proces will not be covered in this manual)
- A configured, Arduino compatible IDE ([Arduino](https://www.arduino.cc/en/Main/Software) or [PlatformIO](http://platformio.org))
- A stepper motor
- A power supply that fits to your stepper motors specs
- A stepper driver board that fits to your stepper motors specs
Optional:
- Limit switches 
- Rotary Encoders

The following chapers describe the setup of a simple project with 2 stepper motors, limit switches and connected rotary encoders. The use of the limit switches and rotary encorders can be skipped if you do not need those

### 1. Firmware installation

#### Using PlatformIO

[PlatformIO](http://platformio.org) is an open source ecosystem for IoT development with cross platform build system, library manager and full support for Espressif ESP32 development. It is based on the free Visual Studio Code from Microsoft, but still works on most popular host OS: Mac OS X, Windows, Linux 32/64, Linux ARM (like Raspberry Pi, BeagleBone, CubieBoard).

1. Install [PlatformIO IDE](http://platformio.org/platformio-ide)
2. Create new project using "PlatformIO Home > New Project"
3. Add "ESP-StepperMotor-Server" to the project using [Project Configuration File `platformio.ini`](http://docs.platformio.org/page/projectconf.html) and [lib_deps](http://docs.platformio.org/page/projectconf/section_env_library.html#lib-deps) option. Here is an example platformio.ini file you can use for an ESP32 based module with ESP-StepperMotor-Server:
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps = ESP-StepperMotor-Server

monitor_speed = 115200
```
 5. now you can open the main.cpp file and include the ESP-StepperMotor-Server and create an instance of the server with a minimal configuration like this:
```
#include <Arduino.h>
#include <ESPStepperMotorServer.h>

ESPStepperMotorServer server(ESPServerWebserverEnabled | ESPServerRestApiEnabled | ESPServerSerialEnabled);

void setup()
{
  Serial.begin(115200);
  //set the log level to DEBUG to show some more output on the serial console
  server.setLogLevel(ESPServerLogLevel_DEBUG);
  server.setWifiMode(ESPServerWifiModeClient);
  server.setWifiCredentials("<YOUR WIFI SSID HERE>", "<YOUR WIFI PASSWORD>");
  server.start();

  //put your own setup code here
}

void loop()
{
     // put your own code that that should be executed 
}
 ```
NOTE: Replace "<YOUR WIFI SSID HERE>" with the name of your WiFI and "<YOUR WIFI PASSWORD>" with your WiFi password
6. [Build/compile and upload the project](http://docs.platformio.org/en/latest/tutorials/espressif32/arduino_debugging_unit_testing.html#compiling-and-uploading-the-firmware) to your connected ESP32 module and open the console Monitor in PlatformIO (or connect to the board using your prefered serial terminal console) to see the output of the ESP-StepperMotor-Server starting up.
In the output on the serial monitor you should see that some output similar to this:
```
[INFO] Starting ESP-StepperMotor-Server (v. 0.0.1)
[INFO] Trying to connect to WiFi with ssid TEST-WIFI
...[INFO] 
[INFO] Connected to network
[INFO] ESPStepperMotorServer WiFi details:
[INFO] WiFi status: server acts as wifi client in existing network with DHCP
[INFO] SSID: HITRON-9480
[INFO] IP address: 192.168.0.22
[INFO] Strength: -45 dBm
...
```
During Startup the ESP32 will check if the User Interface is installed on the ESP32s SPI Flash File System (SPIFFS). If it cannot find one ore more required files it will attemtp to downloa the files via WiFi from the git hub repository. In case your WiFi does not provide an open internet connection, you need to upload the files manually using he "Upload File System image" Task from PlatformIO. More Details can be found in the section [Insallation of the Web UI](#insallation-of-the-web-ui)

#### Using Arduino IDE


### Connecting the hardware


## Insallation of the Web UI

## General API Documentation:
for further documentations see 
- the provided example files,
- the github repository and included README files and examples on: https://github.com/pkerspe/ESP-StepperMotor-Server
- and the wiki on the github page: https://github.com/pkerspe/ESP-StepperMotor-Server/wiki

## License:
Copyright (c) 2019 Paul Kerspe - Licensed under the MIT license.

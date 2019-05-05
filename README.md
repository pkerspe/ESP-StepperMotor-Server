# ESP-StepperMotor-Server - A complete stepper motor control server running on ESP32 modules

Turn your ESP32 into a complete stepper motor control server.
Connect one ore more stepper controllers with step and direction input, and optionally some limit-switches to the IO-pins of your ESP module and controll the stepper motor via an easy to use web interface, via REST API or via a serial control interface.

## Introduction

This library started as a fork for the FlexyStepper library (https://github.com/Stan-Reifel/FlexyStepper). While the FlexyStepper Library is a general Arduino compatible library this fork has a focus on the ESP32 modules form Espressif. It also became much more than a modfied version of FlexyStepper but turned into a complete application to turn a regular ESP32 module into a complete stepper server.
Since these modules contain a WiFi module they are perfectly suited for web controlled stepper server and since they have enough memory and processing power they are ideal as low cost, low energy consumption standalone server component, that allows configuration and controlling of one to many stepper motor drivers with limit-switches and outputs (e.g. for Relays and LEDs).

Once the ESPp Stepper Motor Server has been uploaded to the ESP module, all further configuration and controlling can be done vie the web UI without the need to code another line in the Arduino or PlatformIO IDE.

### What this library is NOT

This library is not ideal if you are looking for a solution to control your CNC Router. It does not support Gerber commands (GRBL) in general or parallel, synchronized multi axis movements.
If you need such a solution you might want to look into the [Grbl_Esp32](https://github.com/bdring/Grbl_Esp32) (for ESP32 specifically) or [grbl](https://github.com/gnea/grbl) (for Arduino in general) Libraries


## Installation

### Using PlatformIO

[PlatformIO](http://platformio.org) is an open source ecosystem for IoT development with cross platform build system, library manager and full support for Espressif ESP8266/ESP32 development. It works on the popular host OS: Mac OS X, Windows, Linux 32/64, Linux ARM (like Raspberry Pi, BeagleBone, CubieBoard).

1. Install [PlatformIO IDE](http://platformio.org/platformio-ide)
2. Create new project using "PlatformIO Home > New Project"
3. Update dev/platform to staging version:
   - [Instruction for Espressif 8266](http://docs.platformio.org/en/latest/platforms/espressif8266.html#using-arduino-framework-with-staging-version)
   - [Instruction for Espressif 32](http://docs.platformio.org/en/latest/platforms/espressif32.html#using-arduino-framework-with-staging-version)
 4. Add "ESP-StepperMotor-Server" to project using [Project Configuration File `platformio.ini`](http://docs.platformio.org/page/projectconf.html) and [lib_deps](http://docs.platformio.org/page/projectconf/section_env_library.html#lib-deps) option:
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps = ESP-StepperMotor-Server

monitor_speed = 115200
```
 5. Happy coding with PlatformIO!

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

## Documentation:
for further documentations see 
- the provided example files,
- the github repository and included README files and examples on: https://github.com/pkerspe/ESP-StepperMotor-Server
- and the wiki on the github page: https://github.com/pkerspe/ESP-StepperMotor-Server/wiki

## License:
Copyright (c) 2019 Paul Kerspe - Licensed under the MIT license.

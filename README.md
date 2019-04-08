# ESP-StepperMotor-Server - A complete stepper motor control server running on ESP8266 and ESP32 modules

Turn your ESP8266 or ESP32 into a complete stepper motor control server.
Connect one ore more stepper controllers with step and direction input, and optionally some homing-switches to the IO-pins of your ESP module and controll the stepper motor via an easy to use web user interface, via REST API or via a serial control interface.

## Introduction

This library is a fork for the standard FlexyStepper library. While the FlexyStepper Library is a general Arduino compatible library this fork has a focus on the ESP8266 and ESP32 modules form Espressif.
Since these modules contain a WiFi module they are perfectly suited for a stand alone, web controller stepper server.

In order to function, this library requires the following 3rd party extensions to be installed:
- the ESP8266 or ESP32 libraries need to be installed in your IDE (in Arduino IDE or PlatformIO use the board manager to install the required files), especially the FS, Wifi libraries are needed
- for the webserver and rest API the ESP Async WebServer as well as the ArduinoJSON library are needed
- for the actual stepper motor controll the FlexyStepper library is needed

If you use PlatformIO you can simply setup your project with the provided paltformio.ini file in this repository

## Documentation:
TBD

## License:
Copyright (c) 2019 P. Kerspe - Licensed under the MIT license.

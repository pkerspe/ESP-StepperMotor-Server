# ESP-StepperMotor-Server:

This Library is ment for ESP32 and ESP8266 modules for development in the Arduino or PlatfromIO IDE. It allows an easy configuration of ESP based stepper motor control server.
It provides multiple interfaces to configure and control 1-n stepper motors, that are connected to the ESP modules IO pins via stepper driver modules.
Basically every stepper driver that provides an step and direction interface (IO Pins) can be controller with this library.

The ESP-StepperMotor-Server starts a webserver with a user interface and also a REST API to control all configured stepper motor drivers as well as limit switches (homing and position switches) as well as software controlelr emergency shutfown switches and outputs to control external components like relays or LEDs.

For more details visit the ESP-StepperMotor-Server github repository: https://github.com/pkerspe/ESP-StepperMotor-Server
And also check out the wiki for a detailed user manual / setup guide: https://github.com/pkerspe/ESP-StepperMotor-Server/wiki


Copyright (c) 2019 Paul Kerspe  -   Licensed under the MIT license.
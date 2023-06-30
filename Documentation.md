# ESP-StepperMotor-Server

This Library is ment for ESP32 modules for development in the Arduino or PlatfromIO IDE. It allows an easy configuration of an ESP32 based stepper motor control server.
It provides multiple interfaces to configure and control 1-n stepper motors, that are connected to the ESP modules IO pins via stepper driver modules.
Basically every stepper driver that provides an step and direction interface (IO Pins) can be controller with this library.

The ESP-StepperMotor-Server starts a webserver with a user interface and also a REST API to configure and control stepper motor drivers as well as limit switches (homing and position switches) and software controled emergency shutdown switches.

Also supports rotary encoders to control the position of connected stepper motors.

For more details visit the ESP-StepperMotor-Server [github repository](https://github.com/pkerspe/ESP-StepperMotor-Server) and check out the detailed [README.md](https://github.com/pkerspe/ESP-StepperMotor-Server/blob/master/README.md) file.

Copyright (c) 2019 Paul Kerspe  -   Licensed under the [MIT license](https://github.com/pkerspe/ESP-StepperMotor-Server/blob/master/LICENSE.txt).
# ESP-StepperMotor-Server - A complete stepper motor control server running on ESP8266 and ESP32 modules

Turn your ESP8266 or ESP32 into a complete stepper motor control server.
Connect one ore more stepper controllers with step and direction input, and optionally some limit-switches to the IO-pins of your ESP module and controll the stepper motor via an easy to use web interface, via REST API or via a serial control interface.

## Introduction

This library started as a fork for the FlexyStepper library (https://github.com/Stan-Reifel/FlexyStepper). While the FlexyStepper Library is a general Arduino compatible library this fork has a focus on the ESP8266 and ESP32 modules form Espressif. It also became much more than a modfied version of FlexyStepper but turned into a complete application to turn a regular ESP32 or ESP8266 module into a complete stepper server.
Since these modules contain a WiFi module they are perfectly suited for web controlled stepper server and since they have enough memory and processing power they are ideal as low cost, low energy consumption standalone server component, that allows configuration and controlling of one to many stepper motor drivers with limit-switches and outputs (e.g. for Relays and LEDs).

Once the ESPp Stepper Motor Server has been uploaded to the ESP module, all further configuration and controlling can be done vie the web UI without the need to code another line in the Arduino or PlatformIO IDE.

#Dependencies:

In order to function, this library requires the following 3rd party extensions to be installed:
- the ESP8266 or ESP32 libraries need to be installed in your IDE (in Arduino IDE or PlatformIO use the board manager to install the required files), especially the FS, Wifi libraries are needed
- for the webserver and rest API the ESP Async WebServer as well as the ArduinoJSON library are needed
- for the actual stepper motor controll the FlexyStepper library is needed
Prerequisites / dependencies:
The ESPStepperMotorServer Library uses the follwing external libraries, which need to be installed in order to compile this sketch:
- ESPAsyncWebserver (https://github.com/me-no-dev/ESPAsyncWebServer)
- AsyncTCP: https://github.com/me-no-dev/AsyncTCP (for ESP32 ) OR ESPAsyncTCP: https://github.com/me-no-dev/ESPAsyncTCP (for ESP8266)
- ArduinoJSON (NOTE: must version 6.x, version 5 will not work): https://arduinojson.org/
- FlexyStepper: https://github.com/Stan-Reifel/FlexyStepper
- FS file system wrapper: should be installed with the ESP8266 or ESP32 libraries already if you setup your IDE for these modules)
- WiFi: should be installed with the ESP8266 or ESP32 libraries already if you setup your IDE for these modules)

When using PlatformIO add these dependencies to you platfromio.ini project file and let PlatfromIO install the required dependencies for you:
'lib_deps = ESP Async WebServer, ArduinoJSON, FlexyStepper, ESP-StepperMotor-Server'

When using Arduino you need to install these libraries using the Library Manager.

If you use PlatformIO you can simply setup your project with the provided paltformio.ini file in this repository

## Documentation:
for further documentations see 
- the provided example files,
- the github repository and included README files on: https://github.com/pkerspe/ESP-StepperMotor-Server
- and the wiki on the github page: https://github.com/pkerspe/ESP-StepperMotor-Server/wiki

## License:
Copyright (c) 2019 Paul Kerspe - Licensed under the MIT license.

# ESP-StepperMotor-Server - A stepper motor control server running on ESP32 modules
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/d9bf5e50c7334b71a9dfba7367b3b18e)](https://app.codacy.com/manual/pkerspe/ESP-StepperMotor-Server?utm_source=github.com&utm_medium=referral&utm_content=pkerspe/ESP-StepperMotor-Server&utm_campaign=Badge_Grade_Dashboard) 

Turn your ESP32 into a standalone stepper motor control server with easy to use webinterface.
Connect one ore more stepper controllers with step and direction input, and optionally some limit-switches to the IO-pins of your ESP module and controll the stepper motor via an easy to use web interface, via REST API or via a serial control interface.

## Table of contents

* [Introduction](#introduction)
  * [What this library is NOT](#what-this-library-is-not)
  * [Prerequisites and dependencies](#prerequisites-and-dependencies)
* [Setting up your ESP-StepperMotor-Server](#Setting-up-your-ESP-StepperMotor-Server)
  * [Firmware installation](#Firmware-installation)
    * [Using Arduino IDE](#using-arduino-ide)
    * [Using PlatformIO](#using-platformio)
  * [Reducing code size](#reducing-code-size)  
  * [Installation of the web user interface](#installation-of-the-web-ui)
  * [Connecting the hardware](#connecting-the-hardware)
  * [Connecting rotary encoders](#connecting-rotary-encoders)
  * [Configuation via the web user interface](#configuation-via-the-web-user-interface)
* [Other UI masks](#other-ui-masks)
  * [The OTA Fimware Update function](#the-ota-fimware-update-function) 
  * [The self-test page](#the-self-test-page)
* [API Documentation](#api-documentation)
  * [Library API documentation](#library-api-documentation)
  * [REST API documentation](#rest-api-documentation)
  * [Serial command line interface (CLI)](#Serial-command-line-interface)
* [Further documentation](#further-documentation)
* [License](#license)

| <img src="https://github.com/pkerspe/ESP-StepperMotor-Server/raw/master/doc/images/server_startup_screen.png" alt="ESP StepperMotor Server start screen" height="200"/> | <img src="https://github.com/pkerspe/ESP-StepperMotor-Server/raw/master/doc/images/setup_screen.png" alt="ESP StepperMotor Server start screen" height="200"/> | <img src="https://github.com/pkerspe/ESP-StepperMotor-Server/raw/master/doc/images/motor_control_screen.png" alt="ESP StepperMotor Server start screen" height="200"/> | <img src="https://github.com/pkerspe/ESP-StepperMotor-Server/raw/master/doc/images/rest_api_doc_screen.png" alt="ESP StepperMotor Server REST API documentation screen" height="200"/> |
| --- | --- | --- | --- |
| home screen | configuration screen | control screen | REST API doucmentation |

## Introduction

This library started as a fork for the [FlexyStepper library](https://github.com/Stan-Reifel/FlexyStepper). While the FlexyStepper Library is a general Arduino compatible library this fork had a focus on the ESP32 modules from Espressif. Soon this library became much more than a modfied version of FlexyStepper but turned into a stand alone application to turn a regular ESP32 module into a stepper motor control server.
The core part that controls the stepper motor drivers is now based on a modified version of FlexyStepper, called [ESP-FlexyStepper](https://github.com/pkerspe/ESP-FlexyStepper).
Since the ESP-32 modules contain a WiFi module they are perfectly suited for web controlled stepper server and since they have enough memory and processing power they are ideal as low cost, low energy consumption standalone server component, that allows configuration and controlling of one to many stepper motor drivers with limit-switches and outputs (e.g. for Relays and LEDs).

Once the ESP Stepper Motor Server has been uploaded to the ESP module, all further configuration and controlling can be done vie the web UI without the need to code another line in the Arduino or PlatformIO IDE.

### What this library is NOT

This library is not ideal if you are looking for a solution to control your CNC Router. It does not support Gerber commands (GRBL) in general or fully synchronized multi axis movements (it can move multiple axis at the same time though since it generates all step signals for all connected stepper drivers asynchronous).
If you need a solution for you CNC project, you might want to look into the [Grbl_Esp32](https://github.com/bdring/Grbl_Esp32) (for ESP32 specifically) or [grbl](https://github.com/gnea/grbl) (for Arduino in general) Libraries. 
But if you are looking for an easy way to setup and control one or more stepper motors independly and adding limit switches and rotary encoders to conrol them, then this project here might be just what you are looking for.

### Prerequisites and dependencies

In order to compile your project with the ESP-StepperMotor-Server Library, the following 3rd party extensions need to be installed on your system (if you use PlatformIO all needed dependencies will be installed for you when you follow the instructions in the [Using PlatformIO](#using-platformio) section above):
*   [ESPAsyncWebserver](https://github.com/me-no-dev/ESPAsyncWebServer)
*   [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
*   [ArduinoJSON (NOTE: must version 6.x, version 5 will not work)](https://arduinojson.org)
*   [ESP-FlexyStepper](https://github.com/pkerspe/ESP-FlexyStepper)
*   FS file system wrapper: should be installed with the ESP32 libraries already if you setup your IDE for these modules
*   WiFi: should be installed with the ESP32 libraries already when you setup your IDE for these modules

When using PlatformIO add these dependencies to you platfromio.ini project file and let PlatfromIO install the required dependencies for you:
```ini
lib_deps = ESP-StepperMotor-Server
```

When using Arduino you need to install these libraries using the Library Manager.

If you use PlatformIO you can simply setup your project with the provided paltformio.ini file in this repository

## Setting up your ESP-StepperMotor-Server

In order to get started you need the following elements:
* A *ESP32* board of your choice (boards with USB-Serial Chips are recommended for the ease of programming them, ohter boards just work as well, yet you have to figure out how to flash the firmware yourself, since this proces will not be covered in this manual)
* A configured, Arduino compatible IDE ([Arduino](https://www.arduino.cc/en/Main/Software) or [PlatformIO](http://platformio.org))
* A *stepper motor*
* A *power supply* that fits to your stepper motors and drivers specs
* A *stepper driver* board that fits to your stepper motors specs

Optional:
* Switches for limit/homing, position and emegency stop functionality
* Rotary encoders to control the motors directly using physical controls

### Firmware installation

#### Using Arduino IDE
USING ARDUINO IDE IS NOT SUGGESTED due to multiple issues with the dependencies and much higher manual effort to get everything running.
If you still want to try it, the folllowing steps might work, but you are basically on your own here.
Do yourself a favour and use a real IDE like Visual Studio Code with PlatformIO (it is free and way more comfortable and pwerfull than the Arduino IDE) :-)

1. Download and install the [Arduino IDE](https://www.arduino.cc/en/main/software)
2. open the library manager and search for "ESP-StepperMotor-Server", select the latest version and click on "install"
3. if asked to install the reuquired dependencies confirm to install all dependencies as well
4. if dependency installation fails install the following dependencies manually: 
  * ArduinoJSON (the one from Benoit Blanchon)
  * ESP-FlexyStepper
  * at the time of writing this documentation, the following two libraries are not available via the library manager, so they need to be installed manually, if you you cannot find them in the library manager:
    * ESPAsyncWebServer: [https://github.com/me-no-dev/ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
    * AsyncTCP [https://github.com/me-no-dev/AsyncTCP](https://github.com/me-no-dev/AsyncTCP)

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
5. now you can open the main.cpp file and include the ESP-StepperMotor-Server and create an instance of the server with a minimal configuration that connects to an existing WiFi like this:
```#include <Arduino.h>
#include <ESPStepperMotorServer.h>

ESPStepperMotorServer *stepperMotorServer;

const char *wifiName= "<your wifi ssid>"; // enter the SSID of the wifi network to connect to
const char *wifiSecret = "<your wifi password>"; // enter the password of the the existing wifi network here

void setup() 
{
  Serial.begin(115200);
  stepperMotorServer = new ESPStepperMotorServer(ESPServerRestApiEnabled | ESPServerWebserverEnabled | ESPServerSerialEnabled);
  stepperMotorServer->setWifiCredentials(wifiName, wifiSecret);
  stepperMotorServer->setWifiMode(ESPServerWifiModeClient); //start the server as a wifi client (DHCP client of an existing wifi network)
  stepperMotorServer->start();
}

void loop() 
{
}
```
*NOTE:* Replace `<your wifi ssid>` with the name of your WiFI and `<your wifi password>` with your WiFi password in the example code above
This is the absolute minimum example how to start the server. For further examples and more inline documentation see the provided example sketches.

6. [Build/compile and upload the project](http://docs.platformio.org/en/latest/tutorials/espressif32/arduino_debugging_unit_testing.html#compiling-and-uploading-the-firmware) to your connected ESP32 module and open the console Monitor in PlatformIO (or connect to the board using your prefered serial terminal console) to see the output of the ESP-StepperMotor-Server starting up.
In the output on the serial monitor you should see that some output similar to this:
```[INFO] Loading configuration file /config.json from SPIFFS
[INFO] 2 stepper configuration entries loaded from config file
[INFO] 2 switch configuration entries loaded from config file
[INFO] 2 rotary encoder configuration entries loaded from config file
[INFO] Starting ESP-StepperMotor-Server (v. 0.0.6)
[INFO] Trying to connect to WiFi with SSID '<your wifi SSID could be here>' ....
[INFO] Connected to network with IP address XXX.XXX.XXX.XXX
...
[INFO] Starting webserver on port YY
[INFO] Webserver started, you can now open the user interface on http://XXX.XXX.XXX.XXX:YY/
...
```
During Startup the ESP32 will check if the User Interface is installed on the ESP32s SPI Flash File System (SPIFFS). If it cannot find one ore more required files it will attemtp to download the files via WiFi from the git hub repository (if the SPIFFS has ben initialized at leas once before). In case your WiFi does not provide an open internet connection, you need to upload the files manually using he "Upload File System image" Task from PlatformIO. More Details can be found in the section [Insallation of the Web UI](#insallation-of-the-web-ui)
If the UI is installed on the SPIFFS you should see the following (or a similar) output in the serial console after the Wifi Connection has been established:
```[INFO] Listing files in root folder of SPIFFS:
[INFO] File: /index.html (615) -1
[INFO] File: /js/app.js.gz (266875) -1
[INFO] File: /img/rotaryEncoderWheel.svg (13750) -1
[INFO] File: /img/stepper.svg (22739) -1
[INFO] File: /img/switch.svg (19709) -1
[INFO] File: /img/logo.svg (25066) -1
[INFO] File: /favicon.ico.gz (1565) -1
```

Now that it is started, you can open the UI in a browser on you PC connected to the same WiFi network, by typing the IP Address you saw in the serial console before into the address bar of your browser prefixed with "http://".

You should now see the start screen of the ESP StepperMotor Server:
![startup screen][startup_screen]

### Reducing code size
When you build the ESP-StepperMotor-Server without any optimizations it takes up a fair amount of the ESP32s flash size (in the standard OTA partition layout).
If you do not need all modules of the ESP-StepperMotor-Server you can use build flags to reduce the code size significantly.
The following build flags are supported:
* ```ESPStepperMotorServer_COMPILE_NO_WEB```: using this flag completely disables the Web Interface, the REST API and the Websocket server. This has the biggest impact on the compiled size, since it also affects the inclusion of the external dependencies of the ESP Async WebServer and AsyncTCP libraries. If you use this flag you will not be able to use the webinterface of the ESP Stepper motor server anymore for configuration and control of the server. You can then only interact with the server using the serial command line interface
* ```ESPStepperMotorServer_COMPILE_NO_DEBUG```: this flag will remove all debug output and debug functions, leading to a small reduction of the size
* ```ESPStepperMotorServer_COMPILE_NO_CLI_HELP```: this flag will remove all help texts from the Command line interface help command and by that reducing the size a bit further

The following chart shows the impact on file size when disabling one or more features (numbers base on a rather small main programm as provided in the examples folder):
![compiled size][compiled_size]

To use one or more of these build flags in PlatformIO, simply add the following line to your platformio.ini file of your project (e.g. to disable debug output and the help texts in the Command Line Interface):
```
build_flags = -D ESPStepperMotorServer_COMPILE_NO_DEBUG -D ESPStepperMotorServer_COMPILE_NO_CLI_HELP
```

### Installation of the Web UI
Once you uploaded the comiled sketch to your ESP32 (dont forget to enter your SSID and Wifi Password in the sketch!) the ESP will connect to the WiFi and check if the UI files are already installed in the SPI Flash File System (SPIFFS) of the ESP. If not it will try to download it.
Once all is done you can enter the IP Adress of you ESP32 module in the browser and you will see the UI of the Stepper Motor Server, where you can configure the stepper motors and controls.

To figure out the IP Adresse of your ESP-32 module, you can either check your routers admin ui or you can connect to the serial port of the ESP-32 and check the output. Once the connection to you WiFi has been established, the module will print the IP address to the serial console.

### Connecting the hardware
The following wiring chart shows an example of a setup with an optional emergency/kill switch and two optional homing/limit switches. In a real setup, the homing switches would be attached for example to each end of a linear rail to detect when the object moved by the stepper motor reaches the end of the track.

*NOTE:* in the below wiring diagram the ENABLE pin of the driver is connected to +5V, you need to check your driver if this is required or actually correct. Some drivers are DISABLED if a high signal is present on the ENABLE pin!

![hardware example setup][connection_setup_example]
(image created with [fritzing](https://fritzing.org/home/))

### Connecting rotary encoders
to connect a rotary encoder you need to free IO Pins, one for the A and one for the B pin of your encoder.
The common pin on the rotary encoder needs to be connected to ground.
The specified IO Pins on the ESP32 will be configured as INPUT with internal pullup resistor automatically.
PLEASE NOTE: once you turn the rotary encoder the current stepper position of the connected stepper motor will be incremented/decremented by the amount of steps you configured in the multiplier field and set as the new target position.
This might not be what you want it to behave, but currently it is the way it is developed. I might implement an additional option that will allow you to increase/decrease the current target position of the stepper motor with each increment on the rotary encoder.
Simply spoken: no matter how quick you turn the rotary encoder it will always just cause the stepper to move an amount of configured steps from its CURRENT physical position when the last signal from the rotary encoder has been received.
For now you could change this behaviour by chaning the following lines in the ESPStepperMotorServer.cpp file:

`stepperConfig->_flexyStepper->setTargetPositionRelativeInSteps(1 * rotaryEncoder->_stepMultiplier);` 

to

 `stepperConfig->_flexyStepper->setTargetPositionInSteps(stepperConfig->_flexyStepper->getTargetPositionInSteps() + 1 * rotaryEncoder->_stepMultiplier);` 

and the line

`stepperConfig->_flexyStepper->setTargetPositionRelativeInSteps(newPosition);` 

to

`stepperConfig->_flexyStepper->setTargetPositionInSteps(stepperConfig->_flexyStepper->getTargetPositionInSteps() + newPosition);`

### Configuation via the web user interface
After you installed everything on the hardware side, you can open the web UI to setup/configure the server.
In the navigation on the left side click on "SETUP" to open the configuration page.

The following devices can be configured:

* Stepper Motors (or to be more precise "connected stepper drivers")
* Switches (input signals in general): multiple types of functions are supported for the switches
  * [Limit](https://en.wikipedia.org/wiki/Limit_switch)/Homing Switches
  * Position Switches
  * emergency stop / [kill switches](https://en.wikipedia.org/wiki/Kill_switch)
  * in future versions also switches to trigger movement macros will be supported
* [Incremental rotary encoders](https://en.wikipedia.org/wiki/Incremental_encoder) as control inputs to control the configured steppers via physical controls (you can always use the web interface or serial control commands directly to control the stepper motor position, speed etc.)

![startup screen][add_stepper_dialog]
![startup screen][add_switch_dialog]
![startup screen][add_rotary_encoder_dialog]

## Other UI masks
Besides the regular UI views to configure the ESP-StepperMotorServer there are currently to other view you can call directly via specic paths:
### The OTA Fimware Update function
When entering the URL `http://<ip of your esp>:<port>/update` you will see the Dialog to update the firmware of the ESP-StepperMotoroServer over the air (OTA).
Once you have the Firmware transfered to your ESP32 initially via the physical connection you do not need to recconnect it to your computer for future updates. You can always use the OTA update function to write new versions of the firmware via Wifi the connection.
For further details on how to create the needed firmware file please refer to your IDE and build chain documentation.

*NOTE: in order to be able to use OTA you need to make sure that you use a flash partition pattern that allows for OTA update. Usually this is the default partition in most IDE settings, so unless you changed the partitioning manually you should be fine anyway.*

### The self-test page
When entering the URL `http://<ip of your esp>:<port>/selftest` you will get to a page that outputs information on your current setup / installation status of the ESP-StepperMotorServer. This page is basically for trouble shooting and will be extended over time.
Whenever you have any issues with your installation you might want to check this page to see if any errors are shown here.
Currently it will putput information mainly about the SPIFFS (SPI-Flash-File-System) status, since it seems to be a common cause for problems according to the issue list on this project. Thus you should check it to see if any negative results are displayed.

## API documentation

### Library API documentation
The Library is designed with ease of use and flexibility of the configuration options in mind. You can configure the server completely in your code or you can just use the basic example code and perform all further configuration in the Web user interface using your browser.

Here is an overview of the most important API calls, in case you want to go down the road of customizing and configuration within you sketch:
*class ESPStepperMotorServer*
this is the main class and the one you want to start with.
|function|description|parameters|
|----|----|----|
|`ESPStepperMotorServer(byte serverMode, byte logLevel)`|constructor to create a new instance of the server. Note: Do not create more than one instance of the server in your code, to prevent issues with the interrupt service routines that are used by the server to listen to inputs.|`byte serverMode`: specify which modules of the server shall be started. Currently three modules are supported: the web interface, the REST API and the serial console control interface. The webinterface relies on the REST API, so it should not be enabled without the REST API. If you do not want to connect to a wifi or start your own AP the serial console is the only way to interact with the server. Use the constants ESPServerRestApiEnabled, ESPServerWebserverEnabled and ESPServerSerialEnabled to decide which modules to enable. e.g. `stepperMotorServer = new ESPStepperMotorServer(ESPServerRestApiEnabled | ESPServerWebserverEnabled | ESPServerSerialEnabled);`<br><br>`byte logLevel`: *optional* parameter to set the log level during instanciation. Default is INFO. Use one of `ESPServerLogLevel_DEBUG`, `ESPServerLogLevel_INFO`, `ESPServerLogLevel_WARNING`|
|`void setHttpPort(int portNumber)`|Set the http port number on which the server should liste for requests for the web interface and the REST API. Only used if the server is started in the ESPServerWebserverEnabled or ESPServerRestApiEnabled (or both) mode|`int portNumber`: the port number to start the webserver on. Only needed if the ESPServerRestApiEnabled or ESPServerWebserverEnabled module is enabled. Default port is 80|
|`void setAccessPointName(const char *accessPointSSID)`|Set the name of the Access Point (SSID) to create by the server|`const char *accessPointSSID`: the name of the access point to open up. Only used when the server is configured to start in AP mode by using `setWifiMode(ESPServerWifiModeAccessPoint)` is used. The default value is 'ESP-StepperMotor-Server'|
|`void setAccessPointPassword(const char *accessPointPassword)`|Set the password needed to connect to the Access Point created by the server. nly used when the server is configured to start in AP mode by using `setWifiMode(ESPServerWifiModeAccessPoint)` is used. The default value is 'Aa123456'|`const char *accessPointPassword`|
|`void setWifiCredentials(const char *ssid, const char *pwd)`|Set the wifi credentials for your local WiFi to connect to. Only used when the server is configured in WiFi Cloent mode by using `setWifiMode(ESPServerWifiModeClient)`|`const char *ssid`: the name of the WiFi to connect to. `const char *pwd`: the password for the WiFi to connect to|
|`void setWifiMode(byte wifiMode)`|Set the WiFi mode to start the server in. It can either operate in WiFi Client or Access Point mode. As a client it connectes to an existing WiFi network. Requires the WiFi access credentials to be set using the `setWifiCredentials` function. In Access Point mode, the server opens it's own WiFi network and waits for clients to connect. You can specify the AP Name and Password using the `setAccessPointName` and `setAccessPointPassword` functions (otherwise default values will be used, for details see documentation of the mentioned functions and parameters)|`byte wifiMode`: the mode to use. Supported values should be provided with the constants `ESPServerWifiModeClient` and `ESPServerWifiModeAccessPoint`. To disable the WiFi modes completely use `ESPServerWifiDisabled`|
|`void setStaticIpAddress(IPAddress staticIP, IPAddress gatewayIP, IPAddress subnetMask, IPAddress dns1, IPAddress dns2)`|Set a static IP Address, gateway IP and subnet mask. The primary and secondary DNS Server arguments are optional and can be omitted|
|`void printWifiStatus()`|prints current WiFi connection deails to the serial console. This should be called only AFTER the server has been started, since the connection to the WiFi network or setup of an Access Point is only done after calling the `start()` function of the server|none|
|`int addOrUpdateStepper(ESPStepperMotorServer_StepperConfiguration *stepper, int stepperIndex = -1)`|Add a new stepper motor to the server configuration or update an existing one with a given id. This function returns the ID of the newly created stepper configuration for further reference. If an existing configuration has been updated, the id of the updated configuration is returned (same as provided `stepperIndex` parameter value)|`ESPStepperMotorServer_StepperConfiguration *stepper,`: pointer to a configured `ESPStepperMotorServer_StepperConfiguration` instance. Optional `int stepperIndex`: if set this parameter indicates the configuration ID of an existing stepper configuration, that shall be overwritten/replace with the new one supplied using the `stepper` parameter|
|`int addOrUpdatePositionSwitch(ESPStepperMotorServer_PositionSwitch *posSwitchToAdd, int switchIndex = -1)`|Add a new position switch to the server configuration or update an existing one with a given id. This function returns the ID of the newly created position switch configuration for further reference. If an existing configuration has been updated, the id of the updated configuration is returned (same as provided `switchIndex` parameter value)|`ESPStepperMotorServer_PositionSwitch *posSwitchToAdd,`: pointer to a configured `ESPStepperMotorServer_PositionSwitch` instance. Optional `int switchIndex`: if set this parameter indicates the configuration ID of an existing switch configuration, that shall be overwritten/replace with the new one supplied using the `posSwitchToAdd` parameter|
|`int addOrUpdateRotaryEncoder(ESPStepperMotorServer_RotaryEncoder *rotaryEncoder, int encoderIndex = -1)`|Add a new rotary encoder to the server configuration or update an existing one with a given id. This function returns the ID of the newly created encoder configuration for further reference. If an existing configuration has been updated, the id of the updated configuration is returned (same as provided `encoderIndex` parameter value)|`ESPStepperMotorServer_RotaryEncoder *rotaryEncoder,`: pointer to a configured `ESPStepperMotorServer_RotaryEncoder` instance. Optional `int encoderIndex`: if set this parameter indicates the configuration ID of an existing encoder configuration, that shall be overwritten/replace with the new one supplied using the `rotaryEncoder` parameter|
|`void removePositionSwitch(int positionSwitchIndex)`|Remove/Delete a previously configured position switch|`int positionSwitchIndex`: the id of the configuration to delete|
|`void removeStepper(byte stepperConfigurationIndex)`|Remove/Delete a previously configured stepper motor|`int stepperConfigurationIndex`: the id of the configuration to delete|
|`void removeRotaryEncoder(byte rotaryEncoderConfigurationIndex)`|Remove/Delete a previously configured encoder|`int rotaryEncoderConfigurationIndex`: the id of the configuration to delete|
|`void printPositionSwitchStatus()`|Print the current switch status to the serial console as JSON formated string. This contains detailed information for each regsitered switch along with its current status. e.g. `{"settings":{"positionSwitchCounterLimit":10,"statusRegisterCounter":2},"switchStatusRegister":[{"statusRegisterIndex":0,"status":"00000000"},{"statusRegisterIndex":1,"status":"00000000"}],"positionSwitches":[{"id":0,"name":"Limit 1 X-Axis","ioPin":14,"position":-1,"stepperId":0,"active":1,"type":{"pinMode":"Active Low"}},{"id":1,"name":"Limit 2 X-Axis","ioPin":15,"position":200000,"stepperId":1,"active":1,"type":{"pinMode":"Active Low"}}]}`|none|
|`void start()`|start the ESP-StepperMotor-Server. Calling this function lets all the magic begin. Depending on the configuration this includes the folllowing tasks: starting the webserver, starting the REST API, starting the serial console command interface, setting up the configured IO pins, registering interrupt handlers for all switches and rotary encoders and printing out a whole bunch of information on the serial console to inform you about the progress of the startup sequence|none|
|`void stop()`|start the ESP-StepperMotor-Server. Basically un-do all the stuff that has been done during the start-up sequence|none|
|`byte getPositionSwitchStatus(int positionSwitchIndex)`|Get the switch status of a specific switch. Return 1 if the switch is currently triggered, 0 if it is not|`int positionSwitchIndex`|
|`void getButtonStatusRegister(byte buffer[ESPServerSwitchStatusRegisterCount])`|Populate the given byte buffer with the switch status for each configrued button (1 = button is tiggered, 0 = button is not triggered)|`byte buffer[]`: a byte array to populate with the status register contents for all configured switches (basically the current switch status in regards to being active/incate)|
|`String getIpAddress()`|get the current IP address of the server. Only Available if connected to a WiFi or if started in AP mode|none|
|`ESPStepperMotorServer_Configuration *getCurrentServerConfiguration()`|get the pointer of the ESPStepperMotorServer_Configuration instance that represents the current server complete configuration|none|

### REST API documentation
Besides the web based User Interface the ESP StepperMotor Server offers a REST API to control all aspects of the server that can also be controlled via the web UI (in fact the web UI uses the REST API for all operations).

The following is an excerpt of the endpoints being provided:
| METHOD | PATH | DESCRIPTION |
|---|---|---|
|GET |`/api/status`|get the current stepper server status report including the following information: version string of the server, wifi information (wifi mode, IP address), spiffs information (total space and free space)|
|POST |`/api/steppers/returnhome`|endpoint to trigger homing of the stepper motor. This is a non blocking call, meaning the API will directly return even though the stepper motor is still performing the homing movement.<br /><br />*IMPORTANT:* this function should only be called if you previously configured a homing / limit switch for this stepper motor, otherwise the stepper will start jogging for a long time (a default limit of 2000000000 steps is configured, but can be overwritten with a POST parameter) before coming to a halt.<br/><br />*Required post parameters:*<br />__id__: the id of the stepper motor to pefrom the homing command for)<br />__speed__: the speed in steps per second to perform the homing command with<br /><br />*Optional POST parameters:*<br/>__switchId__: define the configuration id of the position switch to use as limit switch. __NOTE__: this switch should be assigned to the stepper motor, so you should not provide the id of a position switch that is not linked to the stepper driver defined in the mandatory __id__ parameter. Ideally the switch is also configured as a limit type switch.<br />__direction__: the homing direction for the stepper movement. Could be either 1 or -1. If parameter is not given the direction will be determined from the limit switch configuration (depending on the switch type "begin" or "end")<br/>__accel__: the acceleration for the homing procedure in steps/sec^2, if ommitted the previously defined acceleration in the flexy stepper instance will be used<br />__maxSteps__: this parameter defines the maximum number of steps to perform before cancelling the homing procedure. This is kind of a safeguard to prevent endless spinning of the stepper motor. Defaults to 2000000000 steps|    
|POST|`/api/steppers/moveby`|endpoint to set a new RELATIVE target position for the stepper motor in either mm, revs or steps. Required post parameters: id, unit, value. Optional post parameters: speed, acel, decel|
|POST |`/api/steppers/position`|endpoint to set a new absolute target position for the stepper motor in either mm, revs or steps. Required post parameters: id, unit, value. Optional post parameters: speed, acel, decel|
| GET |`/api/steppers` or `/api/steppers?id=<id>`|endpoint to list all configured steppers or a specific one if "id" query parameter is given
|DELETE|`/api/steppers?id=<id>`|delete an existing stepper configuration entry|
|POST |`/api/steppers`|add a new stepper configuration entry|
| PUT|`/api/steppers?id=<id>`|upate an existing stepper configuration entry|
| GET |`/api/switches/status` or `/api/switches/status?id=<id>`|get the current switch status (active, inactive) of either one specific switch or all switches (returned as a bit mask in MSB order)|
| GET |`/api/switches` or `/api/switches?id=<id>`|endpoint to list all position switch configurations or a specific configuration if the "id" query parameter is given|
| POST |`/api/switches`|endpoint to add a new switch configuration|
| PUT |`/api/switches?id=<id>`|endpoint to update an existing switch configuration|
| DELETE |`/api/switches?id=<id>`|delete a specific switch configuration|
| GET |`/api/config`|get the JSON represenation of the current server configuration with all configured steppers, switches and encoders. This is the in memory configuration (current is-state) which might differ from the persisted configuration. To persist the current configuration see `GET /api/config/save`|
| GET |`/api/config/save`|save the current in-memory configuration of the server to the [SPIFFS](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/spiffs.html) into the `config.json` file. You can download this file using the URL schema `http://<ip of your esp>:<port>/config.json`. Calling this endpoint persists the configuration in its current state to survive also power loss / reboot / reset of the server. This should be called whenver you perform any changes on the configuration that you want to keep even after a reboot/reset of the ESP|

To get a full list of endpoints navigate to the about page in the web ui and click on the REST API documentation link
![about screen][about_screen]

### Serial command line interface
Once started, the stepper server offers a CLI (command line interface) on the serial port to control most of the functions that can also be controlled via the web interface or REST API and some additional functions.
Once the server is started (and the CLI has not been disabled in the constructor) you will see some log output on the console and also the following line:
`[INFO] Command Line Interface started, registered XX commands. Type 'help' to get a list of all supported commands`
Now you can input commands via the serial port.
To get a list of all available commands type `help` in the serial console and press enter.
The output should look like this (depending on the version and how up2date this manual is, you might see some more commands):
````-------- ESP-StepperMotor-Server-CLI Help -----------
The following commands are available:

<command> [<shortcut>]: <description>
help [h]:               show a list of all available commands
moveby [mb]*:           move by an specified amount of units. requires the id of the stepper to move, the amount pf movement and also optional the unit for the movement (mm, steps, revs). If no unit is specified steps will be assumed as unit. E.g. mb=0&v:-100&u:mm to move the stepper with id 0 by -100 mm
moveto [mt]*:           move to an absolute position. requires the id of the stepper to move, the amount pf movement and also optional the unit for the movement (mm, steps, revs). If no unit is specified steps will be assumed as unit. E.g. mt=0&v:100&u:revs to move the stepper with id 0 to the absolute position at 100 revolutions
config [c]:             print the current configuration to the console as JSON formatted string
emergencystop [es]:     trigger emergency stop for all connected steppers. This will clear all target positions and stop the motion controller module immediately. In order to proceed normal operation after this command has been issued, you need to call the revokeemergencystop [res] command
revokeemergencystop [res]:      revoke a previously triggered emergency stop. This must be called before any motions can proceed after a call to the emergencystop command
position [p]*:          get the current position of a specific stepper or all steppers if no explicit index is given (e.g. by calling 'pos' or 'pos=&u:mm'). If no parameter for the unit is provided, will return the position in steps. Requires the ID of the stepper to get the position for as parameter and optional the unit using 'u:mm'/'u:steps'/'u:revs'. E.g.: p=0&u:steps to return the current position of stepper with id = 0 with unit 'steps'
velocity [v]*:          get the current velocity of a specific stepper or all steppers if no explicit index is given (e.g. by calling 'pos' or 'pos=&u:mm'). If no parameter for the unit is provided, will return the position in steps. Requires the ID of the stepper to get the velocity for as parameter and optional the unit using 'u:mm'/'u:steps'/'u:revs'. E.g.: v=0&u:mm to return the velocity in mm per second of stepper with id = 0
removeswitch [rsw]*:    remove an existing switch configuration. E.g. rsw=0 to remove the switch with the ID 0
removestepper [rs]*:    remove and existing stepper configuration. E.g. rs=0 to remove the stepper config with the ID 0
removeencoder [re]*:    remove an existing rotary encoder configuration. E.g. re=0 to remove the encoder with the ID 0
reboot [r]:             reboot the ESP
save [s]:               save the current configuration to the SPIFFS in config.json
stop [st]:              stop the stepper server (also stops the CLI!)
loglevel [ll]*:         set or get the current log level for serial output. valid values to set are: 1 (Warning) - 4 (ALL). E.g. to set to log level DEBUG use sll=3 to get the current loglevel call without parameter
serverstatus [ss]:      print status details of the server as JSON formated string
switchstatus [pss]:     print the status of all input switches as JSON formated string
setapname [san]*:       set the name of the access point to be opened up by the esp (if in AP mode)
setappwd [sap]*:        set the password for the access point to be opened by the esp
sethttpport [shp]*:     set the http port to listen for for the web interface
setwifissid [sws]*:     set the SSID of the WiFi to connect to (if in client mode)
setwifipwd [swp]*:      set the password of the Wifi network to connect to")

commmands marked with a * require input parameters.
Parameters are provided with the command separarted by a = for the primary parameter.
Secondary parameters are provided in the format '&<parametername>:<parametervalue'
````

In general there are two types of commands:
Commands with parameters, and commands without parameters. Some command support also optional parameters.
Each command also has a shortcut (listed in the `help` output in `[]`) that can be used if you are not so much into typing a lot.
Commands with parmaeters need to be called following this schema:
`<commandname or shortcut name>=<primary parameter>&<optional additional parameter name>:<optional additional parameter value>`
An example for a command with multiple parameters is the `moveto` command. The shortcut for this command is `mt`.
The command supports three parameters: the id of the stepper to move (primary parameter), the amount/value for the movement (v parameter) and the unit (u parameter) for the movement (mm, steps or revolutions).
Example: 
If you want to move the configured stepper motor with the id 0 by 10 revolutions the command looks as follows:
`mt=0&v=10&u=revs`

### Further documentation
for further details have a look at 
* the provided example files / projects in the [examples folder](https://github.com/pkerspe/ESP-StepperMotor-Server/tree/master/examples) of this repository

## License
Copyright (c) 2019 Paul Kerspe - Licensed under the MIT license.

[startup_screen]: https://github.com/pkerspe/ESP-StepperMotor-Server/raw/master/doc/images/server_startup_screen.png "ESP StepperMotor Server startup screen"
[add_stepper_dialog]: https://github.com/pkerspe/ESP-StepperMotor-Server/raw/master/doc/images/add_stepper_config_dialog.png "The dialog to add a new stepper motor configuration"
[add_switch_dialog]: https://github.com/pkerspe/ESP-StepperMotor-Server/raw/master/doc/images/add_switch_dialog.png "The dialog to add a new switch (input signal) configuration"
[add_rotary_encoder_dialog]: https://github.com/pkerspe/ESP-StepperMotor-Server/raw/master/doc/images/add_rotary_encoder_dialog.png "The dialog to add a new rotary encoder to control a stepper"
[about_screen]: https://github.com/pkerspe/ESP-StepperMotor-Server/raw/master/doc/images/about_screen.png "The about screen with the link to the REST API documentation"
[connection_setup_example]: https://github.com/pkerspe/ESP-StepperMotor-Server/raw/master/doc/images/connection_setup_example.png "Example setup wiring diagram"
[compiled_size]: https://github.com/pkerspe/ESP-StepperMotor-Server/raw/master/doc/images/compiled_size.png "Compile size comparison of different build options"

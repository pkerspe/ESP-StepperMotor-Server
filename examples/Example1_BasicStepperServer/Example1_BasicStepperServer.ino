
//      ******************************************************************
//      *                                                                *
//      *     Simple example for starting the stepper motor server       *
//      *                                                                *
//      *            Paul Kerspe                8.4.2019                 *
//      *                                                                *
//      ******************************************************************
//
// This is the simplest example of how to start the ESP Stepper Motor Server with the Webinterface to perform all setup steps via the Web UI
//
// Documentation for this library can be found at:
//    https://github.com/pkerspe/ESP-StepperMotor-Server/wiki/ESP-Stepper-Motor-Server-user-manual
//
//
// This library requires that your stepper motor be connected to the ESP32/ESP8266
// using an external drivet that has a "Step and Direction" interface.
// The library provides an easy to setup server that allows configuration and controlling
// of 1-n stepper motors via a serial connection, web user interface and a REST API.
//
// internally the server uses the FlexyStepper Library by Stan Reifel (https://github.com/Stan-Reifel/FlexyStepper).
//
// The get a list of tested stepper drivers please visit:
// https://github.com/pkerspe/ESP-StepperMotor-Server/wiki/Tested-drivers-to-work-with-this-library
//
// This list is just an example, in generall all stepper drivers with a step and direction input should work.
//
// For all driver boards, it is VERY important that you set the motor
// current before running the example. This is typically done by adjusting
// a potentiometer on the board or using dip switches.
// Read the driver board's documentation to learn how to configure the driver
//
// Prerequisites / dependencies:
// The ESPStepperMotorServer Library uses the follwing external libraries, which need to be installed in order to compile this sketch:
// - ESPAsyncWebserver (https://github.com/me-no-dev/ESPAsyncWebServer)
// - AsyncTCP: https://github.com/me-no-dev/AsyncTCP (for ESP32 ) OR ESPAsyncTCP: https://github.com/me-no-dev/ESPAsyncTCP (for ESP8266)
// - ArduinoJSON (NOTE: must version 6.x, version 5 will not work): https://arduinojson.org/
// - FlexyStepper: https://github.com/Stan-Reifel/FlexyStepper
// - FS file system wrapper: should be installed with the ESP8266 or ESP32 libraries already if you setup your IDE for these modules)
// - WiFi: should be installed with the ESP8266 or ESP32 libraries already if you setup your IDE for these modules)
// When using PlatformIO the provided platformio.ini file in this project should already take care of installing all required libraries for you.
// When using Arduino you need to install these libraries using the Library Manager.
//
// Getting started with this script:
// 1.a) if you want the stepper server to connect to an existing Wifi network, enter your SSID and wifi PWD in the lines 46/47 where the variables
// wifiName and wifiSecret are defined
// 1.b) if you do NOT want to connect to an existing wifi network, but want to start a new access point with the server,
// then remove lines 58/59 (starting with "stepperMotorServer.setWifiCredentials(...) and "stepperMotorServer.setWifiMode(...) and unconnent line 63 (stepperMotorServer.setWifiMode(ESPServerWifiModeAccessPoint);)
// 2.) compile the script and upload to you ESP
// 3.) the webserver files are stored on the SPIFFS (the internal flash file system of the ESP). When uploading the sketch for the first time, you also need
// to upload the files from the data folder to the ESPs SPIFFS.
// In order to perform the upload you can use the following options, depending on your IDE:
//
// Arduino IDE: you need to install the Arduino ESP32 File System uploader Library (https://github.com/me-no-dev/arduino-esp32fs-plugin#installation) first (if you did not already do that before).
//              then open the arduino sketch for the ESp Stepper Motor Server and click on "tools" -> "ESP32 Sketch Data Upload"
// NOTE: for more detalils on how to install the upload library, check out this tutorial (or any other tutorial on this topic): https://www.dfrobot.com/blog-1114.html?gclid=EAIaIQobChMIz6DJq5_A4QIVged3Ch0y3AdAEAAYASAAEgJrCvD_BwE
// NOTE: for the ESP8266 another library is needed, check for example this tutorial to install it and upload the files: https://www.instructables.com/id/Using-ESP8266-SPIFFS/
//
// PaltformIO IDE: in PlatformIO this is much simpler (as long as start the project task "Upload File System image"
// this step only needs to be done for the first time you upload the ESP Stepper Motor Server Library to your ESP (or when you want to update to a newer version of the ESP Stepper Motor Server Library)
//
// Once you have uploaded all ressources, connect to the serial port (set to 115200 baud) your ESP is connected to and press the reset buttion.
// You should see some output from the stepper motor server and the ESP should connect to your wifi (or open up a new Access Point with the default name "ESP-StepperMotor-Server")
// The IP Adress of the server will be displayed in the serial console, once the ESP got a IP from the DHCP server.
// Now you can open the Web Interface of your ESP Stepper Motoro Server by entering the IP Adress in a browser on a PC that is connected to the same wifi Network as your ESP.
// Now you are ready to start configuring stepper motors and switches in the web UI.
//
// also see the other examples to learn how to pre-configure the ESPStepper Motor Server in your sketch instead of using the web interface.
// ***********************************************************************

#include <Arduino.h>
#include <ESPStepperMotorServer.h>

// Create an instance of the ESPStepperMotorServer 
// NOTE: do NOT create more than one instance in your sketch, since it will lead to problems with interrupts
ESPStepperMotorServer stepperMotorServer(ESPServerRestApiEnabled | ESPServerWebserverEnabled | ESPServerSerialEnabled);

const char *apName = "ESP-StepperMotor-Server";
const char *wifiName= "<your wifi ssid>"; // enter the SSID of the wifi network to connect to
const char *wifiSecret = "<your wifi password>"; // enter the password of the the existing wifi network here

void setup() 
{
  //start serial connection with 115200 baud
  Serial.begin(115200);

  // stepperMotorServer.setLogLevel(ESPServerLogLevel_DEBUG); //optionally if you want to see more logs in your serial console, you can enable this line

  //connect to a local wifi. Make sure you set the vairables wifiName and wifiSecret to match you SSID and wifi pasword (see above in lines 43/44)
  stepperMotorServer.setWifiCredentials(wifiName, wifiSecret);
  stepperMotorServer.setWifiMode(ESPServerWifiModeClient); //start the server as a wifi client (DHCP client of an existing wifi network)

  //NOTE: if you want to start the server in a stand alone mode that opens a wifi access point, then comment out the above two lines and uncomment the following line
  //stepperMotorServer.setWifiMode(ESPServerWifiModeAccessPoint);

  //start the server
  stepperMotorServer.start();
}

void loop() 
{
  //put your code here
}




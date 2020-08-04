//      ******************************************************************
//      *     Simple example for starting the stepper motor server       *
//      *            Paul Kerspe                31.5.2020                *
//      ******************************************************************
//
// This is the simplest example of how to start the ESP Stepper Motor Server with the Webinterface to perform all setup steps via the Web UI
//
// This library requires that your stepper motor be connected to the ESP32
// using an external driver that has a "Step and Direction" interface.
//
// For all driver boards, it is VERY important that you set the motor
// current before running the example. This is typically done by adjusting
// a potentiometer on the board or using dip switches.
// Read the driver board's documentation to learn how to configure the driver
//
// all you need to do, to get started with this example, is fill in your wifi credentials in lines 26/27, then compile and upload to your ESP32.
// In order to use the Web Interface of the server, you need to upload the contents of the "data" folder in this example to the SPIFFS of your ESP32
//
// for a detailed manual on how to use this library please visit: https://github.com/pkerspe/ESP-StepperMotor-Server/blob/master/README.md
// ***********************************************************************
#include <Arduino.h>
#include <ESPStepperMotorServer.h>

ESPStepperMotorServer *stepperMotorServer;

const char *wifiName= "<your wifi ssid>"; // enter the SSID of the wifi network to connect to
const char *wifiSecret = "<your wifi password>"; // enter the password of the the existing wifi network here

void setup() 
{
  // start the serial interface with 115200 baud
  // IMPORTANT: the following line is important, since the server relies on the serial console for log output 
  // Do not remove this line! (you can modify the baud rate to your needs though, but keep in mind, that slower baud rates might cause timing issues especially if you set the log level to DEBUG)
  Serial.begin(115200);
  // now create a new ESPStepperMotorServer instance (this must be done AFTER the Serial interface has been started)
  // In this example We create the server instance with all modules activated and log level set to INFO (which is the default, you can also use ESPServerLogLevel_DEBUG to set it to debug instead)
  stepperMotorServer = new ESPStepperMotorServer(ESPServerRestApiEnabled | ESPServerWebserverEnabled | ESPServerSerialEnabled, ESPServerLogLevel_INFO);
  // connect to an existing WiFi network. Make sure you set the vairables wifiName and wifiSecret to match you SSID and wifi pasword (see above before the setup function)
  stepperMotorServer->setWifiCredentials(wifiName, wifiSecret);
  stepperMotorServer->setWifiMode(ESPServerWifiModeClient); //start the server as a wifi client (DHCP client of an existing wifi network)

  // NOTE: if you want to start the server in a stand alone mode that opens a wifi access point, then comment out the above two lines and uncomment the following line
  // stepperMotorServer->setWifiMode(ESPServerWifiModeAccessPoint);
  // you can define the AP name and the password using the following two lines, otherwise the defaults will be used (Name: ESP-StepperMotor-Server, password: Aa123456)
  // stepperMotorServer->setAccessPointName("<ap-name>");
  // stepperMotorServer->setAccessPointPassword("<ap password must be longer than 8 characters>");

  //start the server
  stepperMotorServer->start();
  // the server will now connect to the wifi and start the webserver, rest API and serial command line interface.
  // check the serial console for more details like the URL of the WebInterface
}

void loop() 
{
  //put your code here
}

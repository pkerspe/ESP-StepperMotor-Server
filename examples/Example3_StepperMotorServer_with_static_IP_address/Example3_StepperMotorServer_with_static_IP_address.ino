//      *****************************************************
//      *     Example how to configure static IP address    *
//      *            Paul Kerspe                9.10.2020   *
//      *****************************************************
//
// This example shows how to configure the stepper server with a static IP address, gateway and subnet mask
// you only need to modify lines 16 / 17 to configure your wifi SSID and password then compile and upload to your ESP32
//
// for a detailed manual on how to use this library please visit: https://github.com/pkerspe/ESP-StepperMotor-Server/blob/master/README.md
// ***********************************************************************
#include <Arduino.h>
#include <ESPStepperMotorServer.h>

ESPStepperMotorServer *stepperMotorServer;

const char *wifiName = "<your wifi ssid>";       // enter the SSID of the wifi network to connect to
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

  // here we set the static IP address:
  IPAddress staticIp(192, 168, 178, 49);
  IPAddress gatewayIp(192, 168, 178, 1);
  IPAddress subnetMask(255, 255, 255, 0);
  stepperMotorServer->setStaticIpAddress(staticIp, gatewayIp, subnetMask);

  //start the server
  stepperMotorServer->start();
  // the server will now connect to the wifi and start the webserver, rest API and serial command line interface.
  // check the serial console for more details like the URL of the WebInterface
}

void loop()
{
  //put your code here
}

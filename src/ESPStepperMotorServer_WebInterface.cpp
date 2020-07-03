
//      *********************************************************
//      *                                                       *
//      *           ESP32 Stepper Motor Web Interface           *
//      *                                                       *
//      *            Copyright (c) Paul Kerspe, 2019            *
//      *                                                       *
//      **********************************************************

// MIT License
//
// Copyright (c) 2019 Paul Kerspe
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is furnished
// to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <ESPStepperMotorServer_WebInterface.h>

HTTPClient http;
// ---------------------------------------------------------------------------------
//                                  Setup functions
// ---------------------------------------------------------------------------------

//
// constructor for the web user interface module
//
ESPStepperMotorServer_WebInterface::ESPStepperMotorServer_WebInterface(ESPStepperMotorServer *serverRef)
{
  this->_serverRef = serverRef;
  this->_httpServer = NULL;
}

/**
 * check if the UI files exist in the SPIFFS and then register all endpoints for the web UI in the http server
 */
void ESPStepperMotorServer_WebInterface::registerWebInterfaceUrls(AsyncWebServer *httpServer)
{
  this->_httpServer = httpServer;
  if (checkIfGuiExistsInSpiffs())
  {
    this->_httpServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
      request->send(SPIFFS, this->webUiIndexFile);
    });
    this->_httpServer->on(this->webUiIndexFile, HTTP_GET, [this](AsyncWebServerRequest *request) {
      request->send(SPIFFS, this->webUiIndexFile);
    });
    this->_httpServer->on(this->webUiFaviconFile, HTTP_GET, [this](AsyncWebServerRequest *request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiFaviconFile, "image/x-icon");
      //response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });
    /* REMOVED FOR NOW DUE TO SECURITY ISSUES. CAN STILL USE /api/config endpoint to download config (in memory config though!)
    this->_httpServer->on(this->_serverRef->defaultConfigurationFilename, HTTP_GET, [this](AsyncWebServerRequest *request) {
      //FIME: currently this streams the json config including the WiFi Credentials, which might be a security risk.
      //TODO: replace wifi passwords in config with placeholder (maybe add config flag to API to allowed disabling the security feature in the future)
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->_serverRef->defaultConfigurationFilename, "application/json", true);
      request->send(response);
    });
    */
    
    this->_httpServer->on("/js/app.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
      request->redirect(this->webUiJsFile);
    });
    this->_httpServer->on(this->webUiJsFile, HTTP_GET, [this](AsyncWebServerRequest *request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiJsFile, "text/javascript");
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });

    // register image paths with caching header present
    this->_httpServer->on(this->webUiLogoFile, HTTP_GET, [this](AsyncWebServerRequest *request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiLogoFile, "image/svg+xml");
      response->addHeader("Cache-Control", "max-age=36000, public");
      request->send(response);
    });
    this->_httpServer->on(this->webUiStepperGraphic, HTTP_GET, [this](AsyncWebServerRequest *request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiStepperGraphic, "image/svg+xml");
      response->addHeader("Cache-Control", "max-age=36000, public");
      request->send(response);
    });
    this->_httpServer->on(this->webUiEncoderGraphic, HTTP_GET, [this](AsyncWebServerRequest *request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiEncoderGraphic, "image/svg+xml");
      response->addHeader("Cache-Control", "max-age=36000, public");
      request->send(response);
    });
    this->_httpServer->on(this->webUiEmergencySwitchGraphic, HTTP_GET, [this](AsyncWebServerRequest *request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiEmergencySwitchGraphic, "image/svg+xml");
      response->addHeader("Cache-Control", "max-age=36000, public");
      request->send(response);
    });
    this->_httpServer->on(this->webUiSwitchGraphic, HTTP_GET, [this](AsyncWebServerRequest *request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiSwitchGraphic, "image/svg+xml");
      response->addHeader("Cache-Control", "max-age=36000, public");
      request->send(response);
    });
  }
}

/**
 * Checks if all required UI files exist in the SPIFFS. 
 * Will try to download the current version of the files from the git hub repo if they could not be found
 */
bool ESPStepperMotorServer_WebInterface::checkIfGuiExistsInSpiffs()
{
  ESPStepperMotorServer_Logger::logDebug("Checking if web UI is installed in SPIFFS");
  bool uiComplete = true;
  const char *notPresent = "The file %s could not be found on SPIFFS";
  const char *files[] = {this->webUiIndexFile, this->webUiJsFile, this->webUiLogoFile, this->webUiFaviconFile, this->webUiEncoderGraphic, this->webUiEmergencySwitchGraphic, this->webUiStepperGraphic, this->webUiSwitchGraphic};

  for (int i = 0; i < 4; i++)
  {
    if (!SPIFFS.exists(files[i]))
    {
      ESPStepperMotorServer_Logger::logInfof(notPresent, files[i]);
      if (this->_serverRef->getCurrentServerConfiguration()->wifiMode == ESPServerWifiModeClient && WiFi.isConnected())
      {
        char downloadUrl[200];
        strcpy(downloadUrl, webUiRepositoryBasePath);
        strcat(downloadUrl, files[i]);
        if (!this->downloadFileToSpiffs(downloadUrl, files[i]))
        {
          uiComplete = false;
        }
      }
      else
      {
        uiComplete = false;
      }
    }
  }

  if (uiComplete == false && this->_serverRef->getCurrentServerConfiguration()->wifiMode == ESPServerWifiModeAccessPoint)
  {
    ESPStepperMotorServer_Logger::logWarning("The UI does not seem to be installed completely on SPIFFS. Automatic download failed since the server is in Access Point mode and not connected to the internet");
    ESPStepperMotorServer_Logger::logWarning("Start the server in wifi client (STA) mode to enable automatic download of the web interface files to SPIFFS");
  }
  else if (ESPStepperMotorServer_Logger::isDebugEnabled())
  {
    if (uiComplete == true)
    {
      ESPStepperMotorServer_Logger::logDebug("Check completed successfully");
    }
    else
    {
      ESPStepperMotorServer_Logger::logDebug("Check failed, one or more UI files are missing and could not be downloaded automatically");
    }
  }
  return uiComplete;
}

// Perform an HTTP GET request to a remote page to download a file to SPIFFS
bool ESPStepperMotorServer_WebInterface::downloadFileToSpiffs(const char *url, const char *targetPath)
{
  ESPStepperMotorServer_Logger::logDebugf("downloading %s from %s\n", targetPath, url);

  if (http.begin(url))
  {
    int httpCode = http.GET();
    ESPStepperMotorServer_Logger::logDebugf("server responded with %i\n", httpCode);

    //////////////////
    // get length of document (is -1 when Server sends no Content-Length header)
    int len = http.getSize();
    uint8_t buff[128] = {0};

    ESPStepperMotorServer_Logger::logDebugf("starting download stream for file size %i\n", len);

    WiFiClient *stream = &http.getStream();

    ESPStepperMotorServer_Logger::logDebug("opening file for writing");
    File f = SPIFFS.open(targetPath, "w+");

    // read all data from server
    while (http.connected() && (len > 0 || len == -1))
    {
      // get available data size
      size_t size = stream->available();

      ESPStepperMotorServer_Logger::logDebugf("%i bytes available to read from stream\n", size);

      if (size)
      {
        // read up to 128 byte
        int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
        // write it to file
        for (int i = 0; i < c; i++)
        {
          f.write(buff[i]);
        }
        if (len > 0)
        {
          len -= c;
        }
      }
      delay(1);
    }
    f.close();
    ESPStepperMotorServer_Logger::logInfof("Download of %s completed\n", targetPath);
    http.end(); //Close connection
  }

  return SPIFFS.exists(targetPath);
}

// -------------------------------------- End --------------------------------------

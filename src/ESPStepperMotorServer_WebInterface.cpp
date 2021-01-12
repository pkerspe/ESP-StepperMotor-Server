
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

    //OTA update form
    this->_httpServer->on("/update", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (SPIFFS.exists(this->webUiFirmwareUpdate))
        {
            AsyncWebServerResponse *response = request->beginResponse(SPIFFS, this->webUiFirmwareUpdate, "text/html");
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        }
        else
        {
            request->send(200, "text/html", "<html><body><h1>Firmware update</h1><form method='POST' action='#' enctype='multipart/form-data' id='upload_form'><p>Firmware File: <input type='file' accept='.bin' name='update'></p><p><input type='submit' value='Update'></p></form></body></html>");
        }
    });

    //OTA Update handler
    this->_httpServer->on(
        "/update", HTTP_POST, [this](AsyncWebServerRequest *request) {
            // the request handler is triggered after the upload has finished... 
            AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (Update.hasError())?"UPDATE FAILED":"SUCCESS. Rebooting server now");
            response->addHeader("Connection", "close");
            response->addHeader("Access-Control-Allow-Origin", "*");
            request->send(response);
            if (!Update.hasError()) {
                delay(100);
                this->_serverRef->requestReboot("Firmware update completed");
            } },

        //Upload handler to process chunks of data
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!filename.endsWith(".bin"))
            {
                Serial.println("Invalid firmware file provided, must have .bin-extension");
                request->send(400, "text/plain", "Invalid fimrware file given");
                request->client()->close();
            }
            else
            {
                if (!index)
                { // if index == 0 then this is the first frame of data
                    Serial.printf("UploadStart: %s\n", filename.c_str());
                    Serial.setDebugOutput(true);

                    // calculate sketch space required for the update
                    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                    if (!Update.begin(maxSketchSpace))
                    { //start with max available size
                        Update.printError(Serial);
                    }
                }

                //Write chunked data to the free sketch space
                if (Update.write(data, len) != len)
                {
                    Update.printError(Serial);
                }

                if (final)
                { // if the final flag is set then this is the last frame of data
                    if (Update.end(true))
                    { //true to set the size to the current progress
                        Serial.printf("Update Success: %u B\nRebooting...\n", index + len);
                    }
                    else
                    {
                        Update.printError(Serial);
                    }
                    Serial.setDebugOutput(false);
                }
            }
        });

    if (this->_serverRef->isSPIFFSMounted() && checkIfGuiExistsInSpiffs())
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

        //little test page to show contents of SPIFFS and check if it is initialized at all for trouble shooting
        this->_httpServer->on("/selftest", HTTP_GET, [this](AsyncWebServerRequest *request) {
            AsyncResponseStream *response = request->beginResponseStream("text/html");
            response->print("<!DOCTYPE html><html lang=\"en\"><head><title>ESP-StepperMotorServer Test Page</title><link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css\" crossorigin=\"anonymous\"></head><body>");
            response->print("<div class=\"container\"><h1>ESP-StepperMotorServer self test</h1><p>Testing environment:</p><ul>");
            response->printf("<li>Server Version: <strong>%s</strong>", this->_serverRef->version);
            response->printf("<li>SPIFFS Initialized: %s</li>", (this->_serverRef->isSPIFFSMounted()) ? "<span class=\"badge badge-success\">true</span>" : "<span class=\"badge badge-danger\">false</span>");
            if (!this->_serverRef->isSPIFFSMounted())
            {
                response->print("ERROR: SPIFFS not initialized & mounted, in case you are intending to use the WEB UI, you need to make sure the SPI Flash Filesystem has been properly initialized on your ESP32");
            }
            else
            {
                response->printf("<li>WEB UI installed completely: %s</li>", (this->checkIfGuiExistsInSpiffs()) ? "<span class=\"badge badge-success\">true</span>" : "<span class=\"badge badge-danger\">false</span>");

                File root = SPIFFS.open("/");
                if (!root)
                {
                    response->print("ERROR: Failed to open root folder on SPIFFS for reading");
                }
                if (root.isDirectory())
                {
                    response->print("<li>Listing files in root folder of SPIFFS:<ul>");
                    File file = root.openNextFile();
                    while (file)
                    {
                        response->printf("<li>File: %s (%i) %ld</li>", file.name(), file.size(), file.getLastWrite());
                        file = root.openNextFile();
                    }
                    response->printf("</ul></li>");
                    root.close();
                }
            }
            response->printf("</ul>");
            String output = String("");
            this->_serverRef->getServerStatusAsJsonString(output); //populate string with json
            response->print(output);
            response->print("</div></body></html>");
            //send the response last
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
        this->_httpServer->onNotFound([this](AsyncWebServerRequest *request) {
            request->send(404, "text/html", "<html><body><h1>ESP-StepperMotor-Server</h1><p>The requested file could not be found</body></html>");
        });
    }
    else
    {
        ESPStepperMotorServer_Logger::logInfo("No web UI could be registered");
    }
}

/**
 * Checks if all required UI files exist in the SPIFFS.
 * Will try to download the current version of the files from the git hub repo if they could not be found
 */
bool ESPStepperMotorServer_WebInterface::checkIfGuiExistsInSpiffs()
{
#ifndef ESPStepperMotorServer_COMPILE_NO_DEBUG
    ESPStepperMotorServer_Logger::logDebug("Checking if web UI is installed in SPIFFS");
#endif

    if (!this->_serverRef->isSPIFFSMounted())
    {
        ESPStepperMotorServer_Logger::logWarning("SPIFFS is not mounted, UI files not found");
        return false;
    }
    else
    {
        bool uiComplete = true;
        const char *notPresent = "The file %s could not be found on SPIFFS\n";
        const char *files[] = {
            this->webUiIndexFile,
            this->webUiJsFile,
            this->webUiLogoFile,
            this->webUiFaviconFile,
            this->webUiEncoderGraphic,
            this->webUiEmergencySwitchGraphic,
            this->webUiStepperGraphic,
            this->webUiSwitchGraphic,
            this->webUiFirmwareUpdate};

        for (int i = 0; i < 9; i++) //ALWAYS UPDATE THIS COUNTER IF NEW FILES ARE ADDED TO UI
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
#ifndef ESPStepperMotorServer_COMPILE_NO_DEBUG
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
#endif
        return uiComplete;
    }
}

// Perform an HTTP GET request to a remote page to download a file to SPIFFS
bool ESPStepperMotorServer_WebInterface::downloadFileToSpiffs(const char *url, const char *targetPath)
{
    if (!this->_serverRef->isSPIFFSMounted())
    {
        ESPStepperMotorServer_Logger::logWarningf("donwloading of %s was canceled since SPIFFS is not mounted\n");
        return false;
    }
    else
    {
#ifndef ESPStepperMotorServer_COMPILE_NO_DEBUG
        ESPStepperMotorServer_Logger::logDebugf("downloading %s from %s\n", targetPath, url);
#endif

        if (http.begin(url))
        {
#ifndef ESPStepperMotorServer_COMPILE_NO_DEBUG
            int httpCode = http.GET();
            ESPStepperMotorServer_Logger::logDebugf("server responded with %i\n", httpCode);
#endif

            //////////////////
            // get length of document (is -1 when Server sends no Content-Length header)
            int len = http.getSize();
            uint8_t buff[128] = {0};
#ifndef ESPStepperMotorServer_COMPILE_NO_DEBUG
            ESPStepperMotorServer_Logger::logDebugf("starting download stream for file size %i\n", len);
#endif

            WiFiClient *stream = &http.getStream();
#ifndef ESPStepperMotorServer_COMPILE_NO_DEBUG
            ESPStepperMotorServer_Logger::logDebug("opening file for writing");
#endif
            File f = SPIFFS.open(targetPath, "w+");

            // read all data from server
            while (http.connected() && (len > 0 || len == -1))
            {
                // get available data size
                size_t size = stream->available();
#ifndef ESPStepperMotorServer_COMPILE_NO_DEBUG
                ESPStepperMotorServer_Logger::logDebugf("%i bytes available to read from stream\n", size);
#endif

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
}

// -------------------------------------- End --------------------------------------

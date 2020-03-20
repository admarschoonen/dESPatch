/**************************************************************
   dESPatch is a library to update ESP32 modules over HTTP
   connections
   Based on the arduino-esp32 Update library.

   Licensed under L-GPL
 **************************************************************/

#include "dESPatch.h"
#include <Update.h>
#include <Preferences.h>

#define DESPATCH_TASK_PRIORITY tskIDLE_PRIORITY /* higher means higher priority; 0 is lowest (idle task) */
#define DESPATCH_STACK_SIZE 8192 /* stack size in bytes */
#define DESPATCH_MINIMUM_DELAY 1000 /* minimum delay between checking for updates (ms) */

typedef struct dESPatchTaskArg {
  bool autoInstall;
  DESPatch * dESPatch;
} DESPatchTaskArg;

//#define MUTEX_DEBUG

int myMutexTakeFunc(const char * f, SemaphoreHandle_t x, TickType_t timeout)
{
  int retval;

#ifdef MUTEX_DEBUG
  Serial.print(millis());
  Serial.print(" ");
  Serial.print(f);
  Serial.println("(): waiting for mutex");
#endif
  retval = xSemaphoreTake(x, timeout);
#ifdef MUTEX_DEBUG
  Serial.print(millis());
  Serial.print(" ");
  Serial.print(f);
  if (retval == pdPASS) {
    Serial.println("(): got mutex");
  } else {
    Serial.println("(): failed to get mutex");
  }
#endif
  
  return retval;
}

int myMutexGiveFunc(const char * f, SemaphoreHandle_t x)
{
  int retval;

#ifdef MUTEX_DEBUG
  Serial.print(millis());
  Serial.print(" ");
  Serial.print(f);
  Serial.println("(): releasing mutex");
#endif
  retval = xSemaphoreGive(x);
#ifdef MUTEX_DEBUG
  Serial.print(millis());
  Serial.print(" ");
  Serial.print(f);
  if (retval == pdPASS) {
    Serial.println("(): mutex released");
  } else {
    Serial.println("(): failed to release mutex");
  }
#endif

  return retval;
}

#define myMutexTake(x, timeout) myMutexTakeFunc(__func__, x, timeout)

#define myMutexGive(x) myMutexGiveFunc(__func__, x)

#define myMutexCreateMutex() xSemaphoreCreateMutex()

static Preferences dESPatchPrefs;
static WiFiClient client;

String DESPatch::getHeaderValue(String header, String headerName)
{
  return header.substring(strlen(headerName.c_str()));
}

String DESPatch::getMac(void)
{
  String mac;
  uint64_t tmp;
  uint64_t tmp64;
  uint64_t mac_rev = 0;
  uint64_t n;
  uint64_t byte;

  tmp = ESP.getEfuseMac();
  
  tmp64 = (tmp & 0xFFFFFF);
  for (n = 0; n < 3; n++) {
    byte = ((tmp64 >> (n * 8)) & 0xFF);
    mac_rev |= (byte << ((5 - n) * 8));
  }
  tmp64 = ((tmp >> 24) & 0xFFFFFF);
  for (n = 0; n < 3; n++) {
    byte = ((tmp64 >> (n * 8)) & 0xFF);
    mac_rev |= (byte << ((2 - n) * 8));
  }
  mac = String((uint16_t) (mac_rev >> 32), HEX) + String((uint32_t) mac_rev, HEX);
  return mac;
}

int DESPatch::getFile(String filename)
{
  // return codes:
  //  0: installed firmware is same as on server
  //  1: new firmware available
  // -1: error during checking for new firmware
  // -2: error during installation of new firmware
  //
  // Note that if installation of new firmware was successful, this function performs a reset and in that case it does not return

  int retval = 0;
  int pos = -1;
  int httpResponseCode = 0;
  long contentLength = 0;
  bool isValidContentType = false;
  bool isJson = false;
  String fname;
  String line;

  // Connect to server
  if (client.connect(_hostname.c_str(), _port)) {
    // Connection Succeed.
    // Fecthing the bin

    Serial.print("getFile: ");
    Serial.println(filename);
    pos = filename.lastIndexOf('.');
    if ((pos > 0) && (filename.substring(pos).compareTo(".json") == 0)) {
      isJson = true;
    }
    // Get the contents of the bin file
    client.print(String("GET ") + filename + " HTTP/1.1\r\n" +
                 "Host: " + _hostname + "\r\n" +
                 "Cache-Control: no-cache\r\n" +
                 "Connection: close\r\n\r\n");

    // Check what is being sent
    //    Serial.print(String("GET ") + bin + " HTTP/1.1\r\n" +
    //                 "Host: " + host + "\r\n" +
    //                 "Cache-Control: no-cache\r\n" +
    //                 "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("Client Timeout !");
        client.stop();
        return -1;
      }
    }
    // Once the response is available,
    // check stuff

    /*
       Response Structure
        HTTP/1.1 200 OK
        x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
        x-amz-request-id: 2D56B47560B764EC
        Date: Wed, 14 Jun 2017 03:33:59 GMT
        Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
        ETag: "d2afebbaaebc38cd669ce36727152af9"
        Accept-Ranges: bytes
        Content-Type: application/octet-stream
        Content-Length: 357280
        Server: AmazonS3
                                   
        {{BIN FILE CONTENTS}}
    */
    while (client.available()) {
      // read line till /n
      line = client.readStringUntil('\n');
      // remove space, to check if the line is end of headers
      line.trim();

      // if the the line is empty,
      // this is end of headers
      // break the while and feed the
      // remaining `client` to the
      // Update.writeStream();
      if (!line.length()) {
        //headers ended
        break; // and get the OTA started
      }

      // Check if the HTTP Response is 200
      // else break and Exit Update
      if (line.startsWith("HTTP/1.1 ")) {
        httpResponseCode = line.substring(9).toInt();
        if (httpResponseCode != 200) {
          Serial.println("Got a non 200 status code from server: " + 
            String(httpResponseCode) + ". Exiting OTA Update.");
          retval = -1;
          break;
        }
      }

      // extract headers here
      // Start with content length
      if (line.startsWith("Content-Length: ")) {
        contentLength = atol((getHeaderValue(line, 
          "Content-Length: ")).c_str());
      }

      // Next, the content type
      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        if ((isJson) && (contentType == "application/json")) {
          isValidContentType = true;
        } else if ((isJson == false) && (contentType == 
            "application/octet-stream")) {
          isValidContentType = true;
        } else {
          Serial.println("Got invalid content type: " + contentType);
        }
      }
    }
  } else {
    // Connect to S3 failed
    // May be try?
    // Probably a choppy network?
    Serial.println("Connection to " + String(_hostname) + 
      " failed. Please check your setup");
    retval = -1;
    // retry??
    // execOTA();
  }

  // Check what is the contentLength and if content type is `application/octet-stream`

  // check contentLength and content type
  if (contentLength && isValidContentType) {
    if (isJson) {
      while (client.available()) {
        const int     capacity = JSON_OBJECT_SIZE(4) + 110;
        StaticJsonDocument<capacity> _jsonDoc;
        DeserializationError err;

        line = client.readString();
        err = deserializeJson(_jsonDoc, line);
        if (err) {
          Serial.print(F("JSON deserialization failed: "));
          Serial.println(err.c_str());
          retval = -1;
        } else {
          // default to _interval in case updateInterval does not exist
          _interval = _jsonDoc["updateInterval"] | _interval;

          if (_jsonDoc["version"] != nullptr) {
            _remoteVersion = String((const char *) _jsonDoc["version"]);
          } else {
            retval = -1;
          }

          if (_jsonDoc["filename"] != nullptr) {
            _binName = String((const char *) _jsonDoc["filename"]);
          } else {
            retval = -1;
          }

          // url is optional
          if (_jsonDoc["url"] != nullptr) {
            _url = String((const char *) _jsonDoc["url"]);
          } else {
            _url = String("");
          }

          if ((_remoteVersion != "") && (_localVersion != _remoteVersion)) {
            updateAvailable = true;
            retval = 1;
          } else {
            updateAvailable = false;
          }
        }
      }
    } else {
      // Check if there is enough to OTA Update
      bool canBegin = Update.begin(contentLength);
  
      // If yes, begin
      if (canBegin) {
        Serial.println("Begin OTA. This may take 2 - 5 mins to complete. "
          "Things might be quiet for a while.. Patience!");
        // No activity would appear on the Serial monitor
        // So be patient. This may take 2 - 5mins to complete
        size_t written = Update.writeStream(client);
  
        if (written == contentLength) {
          Serial.println("Written : " + String(written) + " successfully");
        } else {
          Serial.println("Written only : " + String(written) + "/" + 
            String(contentLength) + ". Retry?" );
          // retry??
          // execOTA();
        }
  
        if (Update.end()) {
          Serial.println("OTA done!");
          if (Update.isFinished()) {
            dESPatchPrefs.begin("dESPatch", false);
            dESPatchPrefs.putString("version", _remoteVersion);
            dESPatchPrefs.end();
            Serial.println("Update successfully completed. Rebooting.");
            ESP.restart();
          } else {
            Serial.println("Update not finished? Something went wrong!");
            retval = -2;
          }
        } else {
          Serial.println("Error Occurred. Error #: " + 
            String(Update.getError()));
          retval = -2;
        }
      } else {
        // not enough space to begin OTA
        // Understand the partitions and
        // space availability
        Serial.println("Not enough space to begin OTA");
        // calling client.flush(); here crashes the ESP32?
        retval = -2;
      }
    }
  } else {
    // calling client.flush(); here crashes the ESP32?
  }
  return retval;
}

int DESPatch::installUpdateNoMutex(void)
{
  return getFile(_binNameFullPath);
}

int DESPatch::installUpdate(void)
{
  int retval = -ENOTRECOVERABLE;

  if (myMutexTake(_busyMutex, portMAX_DELAY) == pdPASS) {
    retval = installUpdateNoMutex();
    myMutexGive(_busyMutex);
  }

  return retval;
}

int DESPatch::checkForUpdate(bool autoInstall)
{
  unsigned long now;
  int retval = 0;
  int pos = -1;

  if (myMutexTake(_busyMutex, portMAX_DELAY) == pdPASS) {
    now = millis();
    _runState = DESPatchRunStateChecking;
    if (_appendMac) {
      retval = getFile(_jsonNameWithMac);
      if (retval >= 0) {
        pos = _jsonNameWithMac.lastIndexOf('/');
        if (pos >= 0) {
          _binNameFullPath = _jsonNameWithMac.substring(0, pos) + "/" + _binName;
        }
      } else {
        _binNameFullPath = "";
      }
    }
    if ((retval < 0) || (!_appendMac)) {
      // try again without mac
      retval = getFile(_jsonName);
      if (retval >= 0) {
        pos = _jsonName.lastIndexOf('/');
        if (pos >= 0) {
          _binNameFullPath = _jsonName.substring(0, pos) + "/" + _binName;
        }
      } else {
        _binNameFullPath = "";
      }
    }
    if ((retval == 1) && autoInstall) {
      _runState = DESPatchRunStateInstalling;
      Serial.println(String(__func__) + "(): need to do SW update with binname: "
        + _binNameFullPath);
      retval = installUpdateNoMutex();
    }

    _lastTimeChecked = now;
    _runState = DESPatchRunStateIdle;
    myMutexGive(_busyMutex);
  } else {
    retval = -ENOTRECOVERABLE;
  }
  return retval;
}

unsigned long DESPatch::getInterval(void)
{
  unsigned long interval = 0;

  if (myMutexTake(_busyMutex, portMAX_DELAY) == pdPASS) {
    interval = _interval;
    myMutexGive(_busyMutex);
  }

  return interval;
}

String * DESPatch::getUrl(String * s)
{
  if (myMutexTake(_busyMutex, portMAX_DELAY) == pdPASS) {
    *s = _url;
    myMutexGive(_busyMutex);
  } else {
    *s = "";
  }

  return s;
}

String * DESPatch::getLocalVersion(String * s)
{
  if (myMutexTake(_busyMutex, portMAX_DELAY) == pdPASS) {
    *s = _localVersion;
    myMutexGive(_busyMutex);
  } else {
    *s = "";
  }

  return s;
}

String * DESPatch::getRemoteVersion(String * s)
{
  if (myMutexTake(_busyMutex, portMAX_DELAY) == pdPASS) {
    *s = _remoteVersion;
    myMutexGive(_busyMutex);
  } else {
    *s = "";
  }

  return s;
}

void DESPatch::setLogLevel(uint8_t logLevel)
{
  if (myMutexTake(_busyMutex, portMAX_DELAY) == pdPASS) {
    _logLevel = logLevel;
    myMutexGive(_busyMutex);
  }
}

void dESPatchTask(void * DESPatchTaskArgP)
{
  unsigned long start, stop, interval, delay;
  DESPatch * dESPatchP = ((DESPatchTaskArg *) DESPatchTaskArgP)->dESPatch;
  bool autoInstall = ((DESPatchTaskArg *) DESPatchTaskArgP)->autoInstall;

  interval = dESPatchP->getInterval() * 1000;
  while (1) {
    start = millis();

    if (interval > 0) {
      dESPatchP->checkForUpdate(autoInstall);
    }

    interval = dESPatchP->getInterval() * 1000;
    stop = millis();
    if ((interval > (stop - start)) && (interval - (stop - start) > 
        DESPATCH_MINIMUM_DELAY)) {
      delay = interval - (stop - start);
    } else {
      delay = DESPATCH_MINIMUM_DELAY;
    }

    vTaskDelay(delay / portTICK_PERIOD_MS);
  }
}

void DESPatch::setRunState(DESPatchRunState state)
{
  if (myMutexTake(_busyMutex, portMAX_DELAY) == pdPASS) {
    if ((state > DESPatchRunStateIdle) && (_runState == DESPatchRunStatePaused)) {
      _runState = DESPatchRunStateIdle;
    } else {
      _runState = DESPatchRunStatePaused;
    }

    myMutexGive(_busyMutex);
  }
}

DESPatchRunState DESPatch::getRunState(void)
{
  // No mutex here since we want to know exactly in what state the task is
  return _runState;
}

int DESPatch::configure(String hostname, int port, String filename, 
    bool appendMac, unsigned long interval, bool autoInstall, 
    DESPatchCallback callback, void * userdata)
{
  DESPatchTaskArg dESPatchTaskArg;
  int pos;
  int priority;

  if ((interval > 0) && (interval * 1000 < DESPATCH_MINIMUM_DELAY)) {
    Serial.print(F("Error! interval must be equal to or larger than "));
    Serial.println((DESPATCH_MINIMUM_DELAY + 500) / 1000);
    return -EDOM;
  }
  _busyMutex = myMutexCreateMutex();
  if (_busyMutex == NULL) {
    Serial.println("configure(): failed to create mutex!");
    return DESPatchEventInternalError;
  }

  dESPatchPrefs.begin("dESPatch", true);
  _localVersion = dESPatchPrefs.getString("version", "");
  dESPatchPrefs.end();
  _remoteVersion = "";
  updateAvailable = false;
  _url = "";
  _logLevel = 0;

  _hostname = hostname;
  _port = port;

  pos = filename.lastIndexOf('.');
  if ((pos <= 0) || (filename.substring(pos).compareTo(".json") != 0)) {
    return -EINVAL;
  }
  _jsonName = "/" + filename;
  _jsonNameWithMac = "/" + filename.substring(0, pos) + "_" + getMac() + 
    filename.substring(pos);

  _appendMac = appendMac;
  _interval = interval;
  _autoInstall = autoInstall;
  _callback = callback;
  _userdata = userdata;

  // Set _lastTimeChecked to now - _interval to ensure update() will 
  // immediately check for updates the first time it is called
  _lastTimeChecked = millis() - _interval * 1000;

  if (_interval > 0) {
    // Create the background task that will check and install updates
    priority = DESPATCH_TASK_PRIORITY;
    //priority = uxTaskPriorityGet(NULL);
    dESPatchTaskArg.autoInstall = _autoInstall;
    dESPatchTaskArg.dESPatch = this;
    xTaskCreate(dESPatchTask, "dESPatchTask", DESPATCH_STACK_SIZE, &dESPatchTaskArg, priority, NULL);
  }
   
  return 0;
}

void DESPatch::dESPatch() {
}


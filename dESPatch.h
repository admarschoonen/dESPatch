/**************************************************************
   dESPatch is a library to update ESP32 modules over HTTP
   connections
   Based on the arduino-esp32 Update library.

   Licensed under L-GPL
 **************************************************************/

#ifndef dESPatch_h
#define dESPatch_h

#include <errno.h>
#include <ArduinoJson.h>
 
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#if defined(ESP8266)
extern "C" {
  #include "user_interface.h"
}
#define ESP_getChipId()   (ESP.getChipId())
#else
#include <esp_wifi.h>
#define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())
#endif

class DESPatch
{
  public:
    void          dESPatch();

    int           configure(String hostname, int port, String filename, bool appendMac, unsigned int interval);
    int           checkForUpdate(bool installUpdate);
    int           installUpdate(void);
    int           getInterval(void);
    String *      getLocalVersion(void);
    String *      getRemoteVersion(void);
    String *      getUrl(void);

  private:
    String        getHeaderValue(String header, String headerName);
    int           getFile(String filename);
    String        getMac(void);
    String        _hostname;
    int           _port;
    unsigned int  _interval;
    String        _localVersion;
    String        _remoteVersion;
    String        _jsonName;
    String        _jsonNameWithMac;
    String        _binName;
    String        _binNameFullPath;
    String        _url;
    bool          _appendMac;
    unsigned long _lastTimeChecked;
};

#endif

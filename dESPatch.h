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
#include <HTTPClient.h>

#if defined(ESP8266)
extern "C" {
  #include "user_interface.h"
}
#define ESP_getChipId()   (ESP.getChipId())
#else
#include <esp_wifi.h>
#define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())
#endif

typedef enum dESPatchEvent {
  DESPatchEventInternalError = -ENOTRECOVERABLE,
  DESPatchEventInstallationError = -EIO,
  DESPatchEventConnectionError = -ECONNABORTED,
  DESPatchEventAlreadyUpToDate = 0,
  DESPatchEventUpdateAvailable = 1,
  DESPatchEventUpdateInstalledSuccessfully = 2
} DESPatchEvent;

typedef enum dESPatchRunState {
  DESPatchRunStatePaused = 0,
  DESPatchRunStateIdle,
  DESPatchRunStateChecking,
  DESPatchRunStateInstalling,
  DESPatchRunStateError
} DESPatchRunState;

typedef void (*DESPatchCallback)(DESPatchEvent event, void * userdata);

class DESPatch
{
  public:
    /* functions */
    void              dESPatch();
    int               configure(String url, bool appendMac, 
                        bool useBackgroundTask,
                        unsigned long interval, bool autoInstall);
    int               configure(String url, bool appendMac, 
                        bool useBackgroundTask,
                        unsigned long interval, bool autoInstall, 
                        const char * root_ca);
    int               configure(String url, bool appendMac, 
                        bool useBackgroundTask,
                        unsigned long interval, bool autoInstall, 
                        const char * root_ca, DESPatchCallback callback, 
                        void * userdata);
    int               installUpdate(void);
    unsigned long     getInterval(void);
    String            getLocalVersion(void);
    String            getRemoteVersion(void);
    String            getReleaseNotes(void);
    bool              updateAvailable;
    void              setLogLevel(uint8_t logLevel);
    void              setRunState(DESPatchRunState state);

    /*
     * _checkForUpdate() should ideally be a private function but then it
     * cannot be accessed from within dESPatchTask. Do not use
     * _checkForUpdate() in your own sketch; use checkForUpdate() instead
     */
    int               _checkForUpdate(bool autoInstall);
    DESPatchRunState  getRunState(void);
    int               checkForUpdate(bool autoInstall);
    unsigned long     getLastTimeChecked(void);

  private:
    /* functions */
    String            getHeaderValue(String header, String headerName);
    int               parseJson(String line);
    int               doUpdate(HTTPClient & http);
    int               getFile(String filename);
    String            getMac(void);
    int               installUpdateNoMutex(void);

    /* variables */
    SemaphoreHandle_t _busyMutex;
    unsigned long     _interval;
    String            _localVersion;
    String            _remoteVersion;
    String            _jsonUrl;
    String            _jsonUrlWithMac;
    String            _binName;
    String            _binNameFullPath;
    String            _releaseNotes;
    bool              _appendMac;
    bool              _autoInstall;
    DESPatchCallback  _callback;
    void *            _userdata;
    unsigned long     _lastTimeChecked;
    uint8_t           _logLevel;
    DESPatchRunState  _runState;
    const char *      _root_ca;
};

#endif

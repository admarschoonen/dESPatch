# dESPatch
Library to update ESP32 over HTTP


### Usage
- Include in your sketch
```cpp
#include <dESPatch.h>         
DESPatch dESPatch;

```

- Initialize library, in your setup function add
```cpp
//first parameter is the hostname
//second parameter is the port
//third parameter is the file that contains meta-data over the firmware
//fourth parameter indicates if mac address should be appended (ie: download esp32_fw_AABBCCDDEEFF.json instead of esp32_fw.json)
//fifth parameter is the interval in seconds at which it should check for updates (if checkForUpdate() is called within this interval, it immediately returns)
dESPatch.configure("example.com", 80, "esp32_fw.json", true, 10);
```

- In your main loop, repeatedly call
```cpp
//first parameter indicates if update should be installed; if this argument is false it will only check if new firmware is available
//return values:
// 0: installed firmware is same as on server
//  1: new firmware available
// -1: error during checking for new firmware
// -2: error during installation of new firmware
// Note that if installation of new firmware was successful, this function performs a reset and in that case it does not return
dESPatch.checkForUpdate(true);
```

Alternatively, you can call
```cpp
if (dESPatch.checkForUpdate(false) > 0){
  // Update available; ask confirmation from user
  // ...
  dESPatch.installUpdate();
}
```
##### Save settings
This library uses the Preferences library to store the version of the currently installed firmware.


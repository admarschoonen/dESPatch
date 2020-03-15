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
dESPatch.configure("example.com", 80, "esp32_fw.json", true, 10);
```
Replace `example.com` with the hostname where you have stored the firmware and change `80` in case the server runs ons a different port. Replace `esp32_fw.json` with the JSON file that contains the description of the firmware.

- In your main loop, repeatedly call
```cpp
dESPatch.checkForUpdate(true);
```
to check for new firmware and immediately install the firmware.

Alternatively, you can call
```cpp
if (dESPatch.checkForUpdate(false) > 0){
  // Update available; ask confirmation from user
  // ...
  dESPatch.installUpdate();
}
```
if you first want some confirmation from the user to install the firmware.

##### Save settings
This library uses the Preferences library to store the version of the currently installed firmware.


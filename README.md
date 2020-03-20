# dESPatch
Library to perform firmware updates for ESP32 modules via a webserver

## Usage
### Includes
In the top of your sketch add
```cpp
#include <dESPatch.h>
DESPatch dESPatch;
```

### Setup
In your setup function add
```cpp
dESPatch.configure("example.com", 80, "path/to/esp32_update.ino.esp32.json", true, 0, false, NULL, NULL);
```
Replace `example.com` with the hostname where you have stored the firmware and change `80` in case the server runs ons a different port.
Replace `path/to/esp32_update.ino.esp32.json` with the JSON file that contains the description of the firmware.

### Main loop
In your main loop, repeatedly call
```cpp
dESPatch.checkForUpdate(true);
```
to check for new firmware and immediately install the firmware if a new version is available.

### Export binary
In Arduino export the binary with `Sketch` -> `Export compiled binary`. Copy the binary to the server from where the ESP should download it.

### Create JSON file
Create a JSON text file with the following structure and place it in on the server in the same folder as where you stored the binary:
```json
{
  "version": "1.0.0",
  "filename": "esp32_update.ino.esp32.bin",
  "updateInterval": 10
}
```
Replace the version string with the version string of your program (any versioning scheme is allowed).
Replace the filename string with the filename of the binary.
The update interval is the interval in seconds at which dESPatch will check for new versions (see below for checking and installing in background).

That's it! Your ESP should now automatically update itself when you create a new binary and change the version number in the JSON file. See below for more advanced usage.

## Automatic checking and installing in background
Alternatively, instead of manually polling the server for updates dESPatch can automatically check for updates in the background and optionally also automatically install them. See the examples directory for more information.

## Custom firmware for specific devices
The 4th argument of `dESPatch.configure()` indicates if the ESP should append its MAC address to the JSON filename. If it is set to true, it will check for `path/to/esp32_update.ino.esp32_XXXXXXXXXXXX.json` first, where `XXXXXXXXXXXX` is the MAC address in HEX format. If this file is not found, it will check for `path/to/esp32_update.ino.esp32_XXXXXXXXXXXX.json`. This way, you can deploy specific firmware such as beta-versions to select devices first before making it generally available.
                                                                                 

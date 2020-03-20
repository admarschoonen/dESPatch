# dESPatch
Library to perform firmware updates for ESP32 modules via a webserver

## Usage
To use this library, you'll need to do 6 things:
1. Add this library to your sketch and create a DESPatch object.
2. Configure the library so that it knows where to find new versions of your code
3. Change the main loop to check and install updates
4. Export the binary and upload it to the server
5. Create a JSON file that contains the version number and upload it to the server
6. Upload the sketch to the ESP
### Add library and create DESPatch object
In the top of your sketch add
```cpp
#include <dESPatch.h>
DESPatch dESPatch;
```

### Configure the library
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

### Upload this code to your ESP
Upload this sketch to your ESP with `Sketch` -> `Upload Using Programmer` to upload the firmware to your ESP. It will now periodically check and install updates.

That's it! Your ESP should now automatically update itself when you create a new binary and change the version number in the JSON file. All you need to do to install new firmware in future is to export the binary, upload it to the server and change the version number in the JSON file. The ESP will update itself when it has a wifi connection.

For more advanced usage such as automatic checking in the background or deploying custom firmware to specific devices, see below.

## Automatic checking and installing in background
Alternatively, instead of manually polling the server for updates dESPatch can automatically check for updates in the background and optionally also automatically install them. See the examples directory for more information.

## Custom firmware for specific devices
The 4th argument of `dESPatch.configure()` indicates if the ESP should append its MAC address to the JSON filename. If it is set to true, it will check for `path/to/esp32_update.ino.esp32_XXXXXXXXXXXX.json` first, where `XXXXXXXXXXXX` is the MAC address in HEX format. If this file is not found, it will check for `path/to/esp32_update.ino.esp32.json`. This way, you can deploy specific firmware such as beta-versions to select devices first before making it generally available.


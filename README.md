# dESPatch
Library to perform firmware updates for ESP32 modules via a webserver

## Usage
To use this library, you'll need to do a couple of things:
1. Create an account on a server hosting the dESPatch-web service and create a project there
2. Add this library to your sketch and create a DESPatch object.
3. Add the URL including API key to your sketch
4. Change the main loop to check and install updates
5. Export the binary and upload it to the server
6. Upload the sketch to the ESP

From then on your ESP will regularly check the server for new software versions. Releasing a new software version is as easy as creating a new release, uploading the exported binary and entering a unique version number.

### Create an account
For this you need to find a server which runs the dESPatch-web service and create an account there.

### Add library and create DESPatch object
In the top of your sketch add
```cpp
#include <dESPatch.h>
DESPatch dESPatch;
const char* root_ca = \
  "-----BEGIN CERTIFICATE-----\n" \
  "REPLACE THIS TEXT WITH THE ROOT CERTIFICATE OF YOUR SERVER\n" \
  "-----END CERTIFICATE-----\n";
```

and place the root certificate of your server in the `root_ca` object.

### Add the URL and API key
In your setup function add
```cpp
const char url=[] "https://apikey:REPLACE-WITH-APIKEY@example.com/path/to/esp32_update.ino.esp32.json"
unsigned long interval = 60; // By default check for updates every 60 seconds
dESPatch.configure(url, true, false, interval, false, root_ca);
```
Replace the url and API key with the location where you stored the JSON file that contains the description of the firmware.

### Main loop
In your main loop, repeatedly call
```cpp
dESPatch.checkForUpdate(true);
```
to check for new firmware and immediately install the firmware if a new version is available.

If you first want ask the user for confirmation to install updates, use
```cpp
dESPatch.checkForUpdate(false);
if (dESPatch.updateAvailable) {
  // Ask user for confirmation
  ...
  // If user approved update:
  dESPatch.checkForUpdate(true);
}
```

### Export binary
In Arduino export the binary with `Sketch` -> `Export compiled binary`. Upload the binary to the server from where the ESP should download it.

### Upload this code to your ESP
Upload this sketch to your ESP with `Sketch` -> `Upload Using Programmer` to upload the firmware to your ESP. It will now periodically check and install updates.

That's it! Your ESP should now automatically update itself when you create a new binary and change the version number in the JSON file. All you need to do to install new firmware in future is to export the binary, upload it to the server and change the version number in the JSON file. The ESP will update itself when it has a wifi connection.

For more advanced usage such as automatic checking in the background or deploying custom firmware to specific devices, see below.

## Custom firmware for specific devices
The 2nd argument of `dESPatch.configure()` function indicates if the ESP should append its MAC address to the JSON filename. If it is set to true, it will check for `path/to/esp32_update.ino.esp32_XXXXXXXXXXXX.json` first, where `XXXXXXXXXXXX` is the MAC address in HEX format. If this file is not found, it will check for `path/to/esp32_update.ino.esp32.json`. This way, you can deploy specific firmware such as beta-versions to select devices first before making it generally available.


#include <WiFiManager.h>           //https://github.com/admarschoonen/WiFiManager
#include <dESPatch.h>

WiFiManager wifiManager;
DESPatch dESPatch;

void setup() {
  int updateResult;

  Serial.begin(115200);

  //wifiManager.resetSettings();
  wifiManager.configure("test", true, LED_BUILTIN, true, BUTTON_BUILTIN, false);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.print("connected with address: ");
  Serial.println(WiFi.localIP());

  //keep LED on
  digitalWrite(LED_BUILTIN, LED_ON_VALUE_DEFAULT);

  updateResult = dESPatch.configure("example.com", 80, "path/to/esp32_update.ino.esp32.json", true, 10);
  Serial.print("dESPatch.configure() returned with code ");
  Serial.println(updateResult);
}

void loop(void) {
  static unsigned long t_prev = 0;
  static bool ledOn = false;
  unsigned long t_now = millis();
  const int buttonPin = 0;
  
  if (t_now - t_prev >= 2000) {
    t_prev = t_now;
    ledOn ^= true;
    digitalWrite(LED_BUILTIN, ledOn);
  }
  if (dESPatch.checkForUpdate(false) > 0) {
    Serial.print(F("Update available\nLocal version:  "));
    Serial.println(*dESPatch.getLocalVersion());
    Serial.print(F("Remote version: "));
    Serial.println(*dESPatch.getRemoteVersion());
    Serial.println("Press button to install the update");
    if (digitalRead(buttonPin) == LOW) {
      dESPatch.installUpdate();
    }
  }
}


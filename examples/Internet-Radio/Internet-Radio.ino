#include <Arduino.h>
#include <IOXESP32Audio.h>
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  
  // Connect WiFi
  WiFi.begin("WiFi Name", "WiFi Password");
  
  // Wait WiFi connect
  Serial.print("Connecting");
  while(!WiFi.isConnected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println(" connected");
  
  // Init IOXESP32 Audio shield
  Audio.begin(); 

  // Start play internet radio
  Audio.play("https://coolism-web.cdn.byteark.com/;stream/1"); 

  // Set volume level
  Audio.setVolume(80);
}

void loop() {
  delay(2000);
}

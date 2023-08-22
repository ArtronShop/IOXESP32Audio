#include <Arduino.h>
#include <IOXESP32Audio.h>

void setup() {
  Serial.begin(115200);
  
  // Init IOXESP32 Audio shield
  Audio.begin();

  // Set volume level
  Audio.setVolume(80);

  // Start play 1.mp3 on MicroSD Card
  Audio.play("SD:/1.mp3"); 
}

void loop() {
  delay(2000);
}

# IOXESP32Audio

Plays mp3 and wav files from SD card via I2S with IOXESP32 Audio shield hardware.
HELIX-mp3 and -aac decoder is included.

Play `1.mp3` file from MicroSD Card

```c++
#include <Arduino.h>
#include <IOXESP32Audio.h>

void setup() {
  Serial.begin(115200);

  Audio.begin();
  Audio.setVolume(80);
  Audio.play("SD:/1.mp3");
}

void loop() {
  delay(2000);
}
```

Play internet radio

```c++
#include <Arduino.h>
#include <IOXESP32Audio.h>
#include <WiFi.h>

void setup() {
  Serial.begin(115200);

  WiFi.begin("Artron@Kit", "Kit_Artron");

  Serial.print("Connecting");
  while(!WiFi.isConnected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println(" connected");

  Audio.begin();
  Audio.play("https://coolism-web.cdn.byteark.com/;stream/1");
  Audio.setVolume(80);
}

void loop() {
  delay(2000);
}
```


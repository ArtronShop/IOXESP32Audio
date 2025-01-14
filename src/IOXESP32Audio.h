#ifndef __IOXESP32AUDIO_H__
#define __IOXESP32AUDIO_H__

#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <SPIFFS.h>
#include "Audio.h"
#include <esp_log.h>
#include "Wire.h"
#include "SparkFun_WM8960_Arduino_Library.h"

enum AudioType {
    TYPE_MP3,
    TYPE_WAV_8000_MONO,
    TYPE_WAV_16000_MONO,
    TYPE_WAV_44100_MONO,
    TYPE_WAV_8000_STEREO,
    TYPE_WAV_16000_STEREO,
    TYPE_WAV_44100_STEREO
};

typedef void (*EventCallback)(void);

class IOXESP32Audio {
    private:
        ESP32_I2S_Audio audio;

        TaskHandle_t audioLoopTaskHandle;
        bool isV2 = true;

        bool write_register_i2c(uint8_t reg, uint16_t value) ;
        WM8960 *codec = NULL;

    public:
        IOXESP32Audio() ;
        ~IOXESP32Audio() ;

        bool begin(bool isV2 = true) ;
        bool play(const char *path, const char *lang = "en") ;
        bool play(String path, String lang = "en") ;
        bool play(uint8_t* data, uint32_t len, AudioType type) ;
        bool play(File file) ;
        bool play() { return resume(); };
        bool pause() ;
        bool resume() ;
        bool stop() ;

        bool isPlaying() ;

        int getVolume() ;
        void setVolume(int level) ;

        // Event
        void onConnecting(EventCallback cb) ;
        void onPlaying(EventCallback cb) ;
        void onStop(EventCallback cb) ;
        void onPause(EventCallback cb) ;
        void onEOF(EventCallback cb) ;

}
;

extern IOXESP32Audio Audio;

#endif

#ifndef __IOXESP32AUDIO_H__
#define __IOXESP32AUDIO_H__

#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <SPIFFS.h>
#include "Audio.h"
#include <esp_log.h>

enum AudioType {
    TYPE_MP3,
    TYPE_WAV_8000_MONO,
    TYPE_WAV_16000_MONO,
    TYPE_WAV_44100_MONO,
    TYPE_WAV_8000_STEREO,
    TYPE_WAV_16000_STEREO,
    TYPE_WAV_44100_STEREO
};

class IOXESP32Audio {
    public:
        ESP32_I2S_Audio audio;

        TaskHandle_t audioLoopTaskHandle;

    public:
        IOXESP32Audio() ;

        void begin() ;
        bool play(const char *path, const char *lang = "en") ;
        bool play(String path, String lang = "en") ;
        bool play(uint8_t* data, uint32_t len, AudioType type) ;
        bool play() { resume(); };
        bool pause() ;
        bool resume() ;
        bool stop() ;

        bool isPlaying() ;

        int getVolume() ;
        void setVolume(int level) ;

}
;

extern IOXESP32Audio Audio;

#endif
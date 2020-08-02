#ifndef __IOXESP32AUDIO_CPP__
#define __IOXESP32AUDIO_CPP__

#include "IOXESP32Audio.h"

EventCallback connectingCb = NULL;
EventCallback playingCb = NULL;
EventCallback stopCb = NULL;
EventCallback pauseCb = NULL;
EventCallback eofCb = NULL;

void audioLoopTask(void* p) {
    ESP32_I2S_Audio *audio = (ESP32_I2S_Audio*) p;
    while(1) {
        audio->loop();
        delay(1);
    }
    vTaskDelete(NULL);
}

IOXESP32Audio::IOXESP32Audio() {

}

IOXESP32Audio::~IOXESP32Audio() {
    vTaskDelete(audioLoopTaskHandle);
}

void IOXESP32Audio::begin() {
    SPI.begin(18, 19, 23); // SCK, MISO, MOSI
    SD.begin(5); // CS

    this->audio.setVolume(11); // 0...21
    this->audio.setPinout(27, 26, 25); // BCLK, LRC, DOUT

    xTaskCreatePinnedToCore(audioLoopTask, "audioLoopTask", 32 * 1024, &this->audio, 10, &audioLoopTaskHandle, 1);
}

bool IOXESP32Audio::play(const char *path, const char *lang) {
    return this->play(String(path), String(lang));
}

bool IOXESP32Audio::play(String path, String lang) {
    if (path.startsWith("SD:")) { // Play file on SD Card
        this->audio.connecttoFS(SD, path.substring(3));
        ESP_LOGI("Audio", "SD File");
    } else if (path.startsWith("FS:")) { // Play file on SPIFFS
        this->audio.connecttoFS(SPIFFS, path.substring(3));
        ESP_LOGI("Audio", "SPIFFS File");
    } else if (path.startsWith("http://") || path.startsWith("https://")) { // Play file on HTTP
        this->audio.connecttohost(path);
        ESP_LOGI("Audio", "Play http");
    } else {
        this->audio.connecttospeech(path, lang);
        ESP_LOGI("Audio", "TTS");
    }

    return true;
}

bool IOXESP32Audio::play(uint8_t* data, uint32_t len, AudioType type) {

}

bool IOXESP32Audio::play(File file) {
    ESP_LOGI("Audio", "File class");
    this->audio.connecttoFS(file);

    return true;
}

bool IOXESP32Audio::pause() {
    if (this->audio.isRunning()) {
        this->audio.pauseResume();
    }
}

bool IOXESP32Audio::resume() {
    if (!this->audio.isRunning()) {
        this->audio.pauseResume();
    }
}

bool IOXESP32Audio::stop() {
    this->audio.stopSong();
}

bool IOXESP32Audio::isPlaying() {
    return this->audio.isRunning();
}

int IOXESP32Audio::getVolume() {
    return map(this->audio.getVolume(), 0, 21, 0, 100);
}

void IOXESP32Audio::setVolume(int level) {
    level = constrain(level, 0, 100);
    this->audio.setVolume(map(level, 0, 100, 0, 21));
}

void IOXESP32Audio::onConnecting(EventCallback cb) { connectingCb = cb; };
void IOXESP32Audio::onPlaying(EventCallback cb) { playingCb = cb; };
void IOXESP32Audio::onStop(EventCallback cb) { stopCb = cb; };
void IOXESP32Audio::onPause(EventCallback cb) { pauseCb = cb; };
void IOXESP32Audio::onEOF(EventCallback cb) { eofCb = cb; };

IOXESP32Audio Audio;

void audio_info(const char *info){
    ESP_LOGI("Audio", "info: %s", info);
    if (String(info).indexOf("StreamTitle=") >= 0) {
        if (playingCb) playingCb();
    }
}

void audio_id3data(const char *info){  //id3 metadata
    ESP_LOGI("Audio", "id3data: %s", info);
}

void audio_eof_mp3(const char *info){  //end of file
    ESP_LOGI("Audio", "eof_mp3: %s", info);
    if (eofCb) eofCb();
}

void audio_showstation(const char *info){
    ESP_LOGI("Audio", "station: %s", info);
}
void audio_showstreaminfo(const char *info){
    ESP_LOGI("Audio", "streaminfo: %s", info);
}

void audio_showstreamtitle(const char *info){
    ESP_LOGI("Audio", "streamtitle: %s", info);
}

void audio_bitrate(const char *info){
    ESP_LOGI("Audio", "bitrate: %s", info);
}

void audio_commercial(const char *info){  //duration in sec
    ESP_LOGI("Audio", "commercial: %s", info);
}

void audio_icyurl(const char *info){  //homepage
    ESP_LOGI("Audio", "icyurl: %s", info);
}

void audio_lasthost(const char *info){  //stream URL played
    ESP_LOGI("Audio", "lasthost: %s", info);
    if (connectingCb) connectingCb();
}

void audio_eof_speech(const char *info){
    ESP_LOGI("Audio", "eof_speech: %s", info);
}

#endif
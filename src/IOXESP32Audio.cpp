#include "IOXESP32Audio.h"
#include "wm8960.h"

#define CODEC_ADDR  0x1a

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

void IOXESP32Audio::begin(bool isV2) {
    this->isV2 = isV2;

    SPI.begin(18, 19, 23); // SCK, MISO, MOSI
    SD.begin(5); // CS

    this->audio.setVolume(this->isV2 ? 21 : 11); // 0...21
    this->audio.setPinout(27, 26, 25); // BCLK, LRC, DOUT

    if (this->isV2) {
        Wire.begin(21, 22, 100E3); // SDA, SCL, Freq

        // Power Management
        this->write_register_i2c(0x19, 1<<8 | 1<<7 | 1<<6);
        this->write_register_i2c(0x1A, 1<<8 | 1<<7 | 1<<6 | 1<<5 | 1<<4 | 1<<3);
        this->write_register_i2c(0x2F, 1<<3 | 1<<2);

        // Configure clock
        //MCLK->div1->SYSCLK->DAC/ADC sample Freq = 25MHz(MCLK)/2*256 = 48.8kHz
        write_register_i2c(0x04, 0x0000);
        
        //Configure ADC/DAC
        write_register_i2c(0x05, 0x0000);
        
        //Configure audio interface
        //I2S format 16 bits word length
        write_register_i2c(0x07, 0x0002);
        
        //Configure HP_L and HP_R OUTPUTS
        write_register_i2c(0x02, 0x0061 | 0x0100);  //LOUT1 Volume Set
        write_register_i2c(0x03, 0x0061 | 0x0100);  //ROUT1 Volume Set
        
        //Configure SPK_RP and SPK_RN
        write_register_i2c(0x28, 0x0077 | 0x0100); //Left Speaker Volume
        write_register_i2c(0x29, 0x0077 | 0x0100); //Right Speaker Volume
        
        //Enable the OUTPUTS
        write_register_i2c(0x31, 0x00F7); //Enable Class D Speaker Outputs
        
        //Configure DAC volume
        write_register_i2c(0x0a, 0x00FF | 0x0100);
        write_register_i2c(0x0b, 0x00FF | 0x0100);
        
        //3D
        //  write_register_i2c(0x10, 0x000F);
        
        //Configure MIXER
        write_register_i2c(0x22, 1<<8 | 1<<7);
        write_register_i2c(0x25, 1<<8 | 1<<7);
        
        //Jack Detect
        write_register_i2c(0x18, 1<<6 | 0<<5);
        write_register_i2c(0x17, 0x01C3);
        write_register_i2c(0x30, 0x0009);//0x000D,0x0005
    }

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
    return false;
}

bool IOXESP32Audio::play(File file) {
    ESP_LOGI("Audio", "File class");
    this->audio.connecttoFS(file);

    return true;
}

bool IOXESP32Audio::pause() {
    if (this->audio.isRunning()) {
        return this->audio.pauseResume();
    }

    return true;
}

bool IOXESP32Audio::resume() {
    if (!this->audio.isRunning()) {
        return this->audio.pauseResume();
    }

    return true;
}

bool IOXESP32Audio::stop() {
    this->audio.stopSong();
    return true;
}

bool IOXESP32Audio::isPlaying() {
    return this->audio.isRunning();
}

int IOXESP32Audio::getVolume() {
    return map(this->audio.getVolume(), 0, 21, 0, 100);
}

void IOXESP32Audio::setVolume(int level) {
    level = constrain(level, 0, 100);
    if (this->isV2) {
        uint8_t vol_write = map(level, 0, 100, 0b0101111, 0b1111111) & 0b1111111;

        //Configure HP_L and HP_R OUTPUTS
        write_register_i2c(0x02, vol_write | 0x0100);  //LOUT1 Volume Set
        write_register_i2c(0x03, vol_write | 0x0100);  //ROUT1 Volume Set
        
        //Configure SPK_RP and SPK_RN
        write_register_i2c(0x28, vol_write | 0x0100); //Left Speaker Volume
        write_register_i2c(0x29, vol_write | 0x0100); //Right Speaker Volume
    } else {
        this->audio.setVolume(map(level, 0, 100, 0, 21));
    }
}

void IOXESP32Audio::onConnecting(EventCallback cb) { connectingCb = cb; };
void IOXESP32Audio::onPlaying(EventCallback cb) { playingCb = cb; };
void IOXESP32Audio::onStop(EventCallback cb) { stopCb = cb; };
void IOXESP32Audio::onPause(EventCallback cb) { pauseCb = cb; };
void IOXESP32Audio::onEOF(EventCallback cb) { eofCb = cb; };

bool IOXESP32Audio::write_register_i2c(uint8_t reg, uint16_t value) {
    Wire.beginTransmission(CODEC_ADDR);
    Wire.write((reg << 1) | ((value >> 8) & 0x01));
    Wire.write(value & 0xff);
    int ret = Wire.endTransmission();
    if (ret != 0) {
        ESP_LOGI("Audio", "I2C write error code : %d", ret);
    } else {
        ESP_LOGI("Audio", "I2C write OK!");
    }
    return ret == 0;
}

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

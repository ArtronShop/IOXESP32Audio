#include "IOXESP32Audio.h"

EventCallback connectingCb = NULL;
EventCallback playingCb = NULL;
EventCallback stopCb = NULL;
EventCallback pauseCb = NULL;
EventCallback eofCb = NULL;

static void audioLoopTask(void* p) {
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

bool IOXESP32Audio::begin(bool isV2) {
    this->isV2 = isV2;

    SPI.begin(18, 19, 23); // SCK, MISO, MOSI
    SD.begin(5); // CS

    this->audio.setVolume(this->isV2 ? 21 : 11); // 0...21
    this->audio.setPinout(27, 26, 25); // BCLK, LRC, DOUT

    if (this->isV2) {
        Wire.begin(21, 22, 100E3); // SDA, SCL, Freq

        this->codec = new WM8960();
        if (!this->codec->begin(Wire)) {
            return false;
        }

        // General setup needed
        this->codec->enableVREF();
        this->codec->enableVMID();

        // Connect from DAC outputs to output mixer
        this->codec->enableLD2LO();
        this->codec->enableRD2RO();

        // Set gainstage between booster mixer and output mixer
        // For this loopback example, we are going to keep these as low as they go
        this->codec->setLB2LOVOL(WM8960_OUTPUT_MIXER_GAIN_NEG_21DB); 
        this->codec->setRB2ROVOL(WM8960_OUTPUT_MIXER_GAIN_NEG_21DB);

        // Enable output mixers
        this->codec->enableLOMIX();
        this->codec->enableROMIX();

        // CLOCK STUFF, These settings will get you 44.1KHz sample rate, and class-d 
        // freq at 705.6kHz
        this->codec->enablePLL(); // Needed for class-d amp clock
        this->codec->setPLLPRESCALE(WM8960_PLLPRESCALE_DIV_2);
        this->codec->setSMD(WM8960_PLL_MODE_FRACTIONAL);
        this->codec->setCLKSEL(WM8960_CLKSEL_PLL);
        this->codec->setSYSCLKDIV(WM8960_SYSCLK_DIV_BY_2);
        this->codec->setBCLKDIV(4);
        this->codec->setDCLKDIV(WM8960_DCLKDIV_16);
        this->codec->setPLLN(7);
        this->codec->setPLLK(0x86, 0xC2, 0x26); // PLLK=86C226h
        //this->codec->setADCDIV(0); // Default is 000 (what we need for 44.1KHz)
        //this->codec->setDACDIV(0); // Default is 000 (what we need for 44.1KHz)
        this->codec->setWL(WM8960_WL_16BIT);

        this->codec->enablePeripheralMode();
        //this->codec->enableMasterMode();
        //this->codec->setALRCGPIO(); // Note, should not be changed while ADC is enabled.

        // Enable DACs
        this->codec->enableDacLeft();
        this->codec->enableDacRight();

        //this->codec->enableLoopBack(); // Loopback sends ADC data directly into DAC
        this->codec->disableLoopBack();

        // Default is "soft mute" on, so we must disable mute to make channels active
        this->codec->disableDacMute(); 

        this->codec->enableSpeakers();
        this->codec->enableHeadphones();
        this->codec->disableOUT3MIX();
        // this->codec->enableOUT3MIX(); // Provides VMID as buffer for headphone ground

        ESP_LOGI("Audio", "Volume set to +0dB");
        this->codec->setHeadphoneVolumeDB(0.00);

        ESP_LOGI("Audio", "Volume set to +0dB");
        this->codec->setSpeakerVolumeDB(0.00);
    }

    xTaskCreatePinnedToCore(audioLoopTask, "audioLoopTask", 32 * 1024, &this->audio, 10, &audioLoopTaskHandle, 1);

    return true;
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
        float vol_write = map(level, 0, 100, -74, 6);
        this->codec->setHeadphoneVolumeDB(vol_write);
        this->codec->setSpeakerVolumeDB(vol_write);
    } else {
        this->audio.setVolume(map(level, 0, 100, 0, 21));
    }
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

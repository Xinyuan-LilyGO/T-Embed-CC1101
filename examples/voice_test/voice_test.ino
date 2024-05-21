#include "Arduino.h"
#include "Audio.h"
#include "SD.h"
#include "SPI.h"
#include "FS.h"
#include "Ticker.h"
#include "SPIFFS.h"


// Digital I/O used
#define SD_CS         13
#define SPI_MOSI      9
#define SPI_MISO      10
#define SPI_SCK       11
#define I2S_DOUT      7
#define I2S_BCLK      46
#define I2S_LRC       40


Audio audio;
Ticker ticker;
struct tm timeinfo;
time_t now;

uint8_t hour    = 6;
uint8_t minute  = 59;
uint8_t sec     = 45;

bool f_time     = false;
int8_t timefile = -1;
char chbuf[100];

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}


void tckr1s(){
    sec++;
    if(sec > 59)   {sec = 0;     minute++;}
    if(minute > 59){minute = 0; hour++;}
    if(hour > 23)  {hour = 0;}
    if(minute == 59 && sec == 50) f_time = true;  // flag will be set 10s before full hour
    Serial.printf("%02d:%02d:%02d\n", hour, minute, sec);
}

void setup() {
    Serial.begin(115200);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SD.begin(SD_CS);

    if(!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    listDir(SPIFFS, "/", 0);


    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(255); // 0...21
    ticker.attach(1, tckr1s);
}

void loop(){
    audio.loop();
    if(f_time == true){
        f_time = false;
        timefile = 3;
        uint8_t next_hour = hour + 1;
        if(next_hour == 25) next_hour = 1;
        sprintf(chbuf, "/%03d.mp3", 1);
        Serial.println(chbuf);
        audio.connecttoFS(SPIFFS, chbuf);
    }
    delay(1);
}

void audio_eof_mp3(const char *info){  //end of file
    Serial.printf("file :%s\n", info);
    if(timefile>0){
        if(timefile==1){audio.connecttoFS(SPIFFS, "/001.mp3");     timefile--;}  // stroke
        if(timefile==2){audio.connecttoFS(SPIFFS, "/001.mp3");     timefile--;}  // precisely
        if(timefile==3){audio.connecttoFS(SPIFFS, "/001.mp3"); timefile--;}
    }
}

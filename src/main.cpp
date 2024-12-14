#include "Arduino.h"
#include <EEPROM.h>
#include <esp_camera.h>
#include <FS.h>
#include <SD_MMC.h>

#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

#define RED_LED_PIN 33
#define GPIO12_PIN 12

#define EEPROM_SIZE 8
#define PICTURENR_ADDRESS 4

volatile unsigned long pulseBegin = micros();
volatile unsigned long pulseEnd = micros();
volatile unsigned long pulseDuration = 0;

static void gpio12PinInterrupt() {
    if (digitalRead(GPIO12_PIN) == HIGH) {
        pulseBegin = micros();
    } else {
        pulseEnd = micros();
        pulseDuration = pulseEnd - pulseBegin;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GPIO12_PIN, INPUT);

    attachInterrupt(GPIO12_PIN, gpio12PinInterrupt, CHANGE);

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_QVGA;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 10;
    config.fb_count = 2;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, 1);
    s->set_hmirror(s, 2);

    if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println("SD Card Mount Failed");
        return;
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD Card attached");
        return;
    }

    EEPROM.begin(EEPROM_SIZE);

    digitalWrite(RED_LED_PIN, HIGH);
}

void loop() {
    unsigned long d = pulseDuration;
    if (d < 1400) {
        digitalWrite(RED_LED_PIN, HIGH);
    } else {
        digitalWrite(RED_LED_PIN, LOW);

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            delay(1000);
            return;
        }

        uint32_t pictureNumber = EEPROM.readULong(PICTURENR_ADDRESS) + 1;
        String path = "/photo_" + String(pictureNumber) + ".jpg";

        fs::FS &fs = SD_MMC;
        File file = fs.open(path.c_str(), FILE_WRITE);
        if (!file) {
            Serial.println("Failed to open file in writing mode");
        } else {
            file.write(fb->buf, fb->len);
            Serial.printf("Saved file to path: %s\n", path.c_str());
            EEPROM.writeULong(PICTURENR_ADDRESS, pictureNumber);
            EEPROM.commit();
            file.close();
        }
        esp_camera_fb_return(fb);
    }

    delay(1000);
}

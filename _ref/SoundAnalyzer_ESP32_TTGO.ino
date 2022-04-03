/*  Sound analyzer
    Several display modes (spectrum, sound amplitude, envelope)
    (by) Lesept October 2020
*/
#include <Arduino.h>
#include <driver/adc.h>
//#include <WiFi.h>
#include <complex.h>
#define FREQ2IND (SAMPLES * 1.0 / MAX_FREQ)
#include "params.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#define MIC 32

//Display parameters
#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#define TFT_MOSI            19
#define TFT_SCLK            18
#define TFT_CS              5
#define TFT_DC              16
#define TFT_RST             23

#define TFT_BL              4   // Display backlight control pin
#define ADC_EN              14  // ADC_EN is the ADC detection enable port
#define ADC_PIN             34
#define BUTTON_1            35
#define BUTTON_2            0

TFT_eSPI display = TFT_eSPI(135, 240); // Invoke custom library

unsigned long chrono, chrono1;
unsigned long sampling_period_us;
byte peak[SAMPLES] = {0};
float complex data[SAMPLES];
int sound[SAMPLES];
float MULT = MAX_FREQ * 1000.0 / SAMPLES;
unsigned int P2P = 0;
int LOG2SAMPLE = log(SAMPLES) / log(2);
#include "functions.h"
#define MODE 19
#define MAXMODES 5

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  adc1_config_width(ADC_WIDTH_BIT_12);
  // ADC1 Channel 4 is GPIO 32 (microphone)
  // https://microcontrollerslab.com/wp-content/uploads/2019/03/ESP32-ADC-channels-pinout.jpg
  adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
  display.init();
  display.fillScreen(TFT_BLACK);
  display.setRotation(3);
  display.setTextColor(TFT_BLUE);
  display.drawString("on ESP32", 20, 110, 4); // Splash screen
  display.setTextSize(2);
  display.drawString("SONO", 0, 10, 4);
  display.drawString("METER", 68, 60, 4);
  display.setTextSize(1);
  display.setTextColor(TFT_RED);
  display.drawString("[by Lesept]", 165, 120, 2);
  delay(2000);
  sampling_period_us = round(1000ul * (1.0 / MAX_FREQ));
  chrono1 = millis();
}

void loop() {
  static int modes = 0;
  bool changeMode = false;
  // Push the button to change modes
  if (millis() - chrono1 > 1000ul) {
    if (digitalRead(BUTTON_1) == LOW || digitalRead(BUTTON_2) == LOW) {
      delay(30);
      modes = (modes + 1) % MAXMODES;
      //    Serial.printf("Mode %d\n", modes);
      changeMode = true;
      chrono1 = millis();
    }
  }
  switch (modes) {
    case 0: // Display spectrum
      acquireSound();
      displaySpectrum();
      break;
    case 1: // Display spectrum
      acquireSound();
      displaySpectrum2();
      break;
    case 2: // Display amplitude with bars
      displayAmplitudeBars();
      break;
    case 3: // Display envelope
      displayEnvelope(changeMode);
      break;
    case 4: // Display running amplitude
      displayRunningEnvelope(changeMode);
      break;
    default:
      break;
  }
  // Touch GPIO 15 to stop the display
  const int touchPin = 15;
  const int threshold = 90;
  if (touchRead(touchPin) < threshold) while (touchRead(touchPin) < threshold);
}

#ifndef MAIN_H
#define MAIN_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include "Adafruit_SHT31.h"
#include "Adafruit_VEML7700.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

/*
INIT – Powers on, initializes sensors, GPS, and Bluetooth
MENU – User sets filters and starts scan
SAMPLE – Collects and averages sensor data over 30s
PROCESS – Runs matching algorithm and scores plants
DISPLAY – Shows ranked plant recommendations
DETAIL – Shows care info and specs for selected plant
SYNC – Transfers results to mobile app via Bluetooth
IDLE – Deep sleep; awaits user input or times out from MENU
ERROR – Handles sensor failure, GPS timeout, or Bluetooth loss; returns to MENU
*/
enum State {
    INIT, 
    MENU,
    SAMPLE,
    PROCESS,   
    REC_DISPLAY,
    DETAIL,
    SYNC,
    IDLE,
    ERROR
};

extern State currState;
extern State prevState;

// Display config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 38
#define SCREEN_ADDRESS 0x3C

// wifi config
extern const char* ssid;
extern const char* password;

// database config
extern const char* url;
extern const char* anonKey;

//buttons and LEDs 
extern const int btn1;
extern const int btn2;
extern const int selectbtn;
extern const int selectLED;
extern const int LEDR;
extern const int LEDG;
extern const int LEDB;

// button and LED states
extern byte lastBtn1State;
extern byte lastBtn2State;
extern byte lastSelectState;
extern byte redLEDState;
extern byte greenLEDState;
extern byte blueLEDState;
extern byte selectLEDState;


struct Plant {
  String name;
  int min_temp;
  int max_temp;
  int min_humidity;
  int max_humidity;
  String sunlight;
  int score;
};

struct SensorData {
  float temp_f;
  float humidity;
  float lux;
  String sunlight;
};

extern Adafruit_SHT31 sht31;
extern Adafruit_VEML7700 veml;
extern Adafruit_SSD1306 display;

extern Plant plants[20];
extern int plantCount;
extern int topPlants[3];
extern int topScores[3];
extern SensorData currentReading;

void start_menu(void);
void fetchPlants(void);
void collectAndProcessData(void);
void matchPlants(void);
void displayRecommendations(void);
float celsiusToFahrenheit(float c);
String categorizeSunlight(float lux);

void setLED(const char* color); // red, green, blue, off
void scanI2C();
void collectAndProcessData();
void setSelectLED(bool state);
void loadMockPlants(); 
void handleInit();
void handleMenu();
void handleSample();
void handleProcess();
void handleRecDisplay();
void handleDetail();
void handleSync();
void handleIdle();
void handleError();

bool btn1Pressed();
bool btn2Pressed();
bool selectPressed();

#endif
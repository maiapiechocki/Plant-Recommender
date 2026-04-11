#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//MAKE SURE Arduinojson by Benoit LIBRARY IS DOWNLOADED
// -------- WIFI + SUPABASE --------
const char* ssid = "ErinYuen";
const char* password = "erinyuen";

const char* url = "https://nxvpnuvxrcjaeikjznxt.supabase.co/rest/v1/plants?select=name";
const char* anonKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im54dnBudXZ4cmNqYWVpa2p6bnh0Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzE3MTcxMDIsImV4cCI6MjA4NzI5MzEwMn0.qGHLAlnQcnSvdgpGIJZeHbFO9smyhPqsA7psFe9ZNGc";

// -------- OLED --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 38
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// -------- STORAGE --------
String plantNames[20];  // max 20 plants
int plantCount = 0;
int currentIndex = 0;

// -------- FETCH DATA --------
void fetchPlants() {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, url);

  http.addHeader("apikey", anonKey);
  http.addHeader("Authorization", String("Bearer ") + anonKey);

  int code = http.GET();

  if (code == 200) {
    String payload = http.getString();
    Serial.println(payload);

    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload);

    plantCount = 0;

    for (JsonObject obj : doc.as<JsonArray>()) {
      if (plantCount < 20) {
        plantNames[plantCount] = obj["name"].as<String>();
        plantCount++;
      }
    }
  } else {
    Serial.println("HTTP Error");
  }

  http.end();
}

// -------- DISPLAY --------
void showPlant(String name) {
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);

  display.println(name);
  display.display();
}

// -------- SETUP --------
void setup() {
  Serial.begin(115200);
  Wire.begin(11,12); // Nano ESP32 I2C

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while (1);
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");

  fetchPlants();
}

// -------- LOOP --------
void loop() {
  if (plantCount > 0) {
    showPlant(plantNames[currentIndex]);

    currentIndex++;
    if (currentIndex >= plantCount) {
      currentIndex = 0;
    }
  }

  delay(2000);  // change plant every 2 seconds
}
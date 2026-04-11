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

const char* url =
  "https://nxvpnuvxrcjaeikjznxt.supabase.co/rest/v1/plants?"
  "select=name,min_temp,max_temp,min_humidity,max_humidity,sunlight";
const char* anonKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im54dnBudXZ4cmNqYWVpa2p6bnh0Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzE3MTcxMDIsImV4cCI6MjA4NzI5MzEwMn0.qGHLAlnQcnSvdgpGIJZeHbFO9smyhPqsA7psFe9ZNGc";

// -------- OLED --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 38
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// -------- STORAGE --------
String plantNames[20];
String plantSunlight[20];
int plantMinTemp[20];
int plantMaxTemp[20];
int plantMinHumidity[20];
int plantMaxHumidity[20];
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
  http.addHeader("Accept", "application/json");

  int code = http.GET();

  if (code == 200) {
    String payload = http.getString();
    Serial.println(payload);

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, payload);

    if (err) {
      Serial.print("JSON error: ");
      Serial.println(err.c_str());
      http.end();
      return;
    }

    plantCount = 0;

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
      if (plantCount < 20) {
        plantNames[plantCount] = obj["name"] | "";
        plantMinTemp[plantCount] = obj["min_temp"] | 0;
        plantMaxTemp[plantCount] = obj["max_temp"] | 0;
        plantMinHumidity[plantCount] = obj["min_humidity"] | 0;
        plantMaxHumidity[plantCount] = obj["max_humidity"] | 0;
        plantSunlight[plantCount] = obj["sunlight"] | "";
        plantCount++;
      }
    }
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(code);
    Serial.println(http.getString());
  }

  http.end();
}

// -------- DISPLAY --------
//loop through each plant
void showPlant(int i) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.println(plantNames[i]);

  display.print("T: ");
  display.print(plantMinTemp[i]);
  display.print("-");
  display.println(plantMaxTemp[i]);

  display.print("H: ");
  display.print(plantMinHumidity[i]);
  display.print("-");
  display.println(plantMaxHumidity[i]);

  display.print("Sun: ");
  display.println(plantSunlight[i]);

  display.display();
}

// -------- SETUP --------
void setup() {
  Serial.begin(115200);

  Wire.begin(11, 12); //keep this as 11, 12

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();

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
    showPlant(currentIndex);

    currentIndex++;
    if (currentIndex >= plantCount) {
      currentIndex = 0;
    }
  } else {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("No plant data");
    display.display();
  }

  delay(2000);
}
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

// wifi config
const char* ssid = "USC Guest Wireless";
const char* password = "";

const char* url =
  "https://nxvpnuvxrcjaeikjznxt.supabase.co/rest/v1/plants?"
  "select=name,min_temp,max_temp,min_humidity,max_humidity,sunlight";
const char* anonKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im54dnBudXZ4cmNqYWVpa2p6bnh0Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzE3MTcxMDIsImV4cCI6MjA4NzI5MzEwMn0.qGHLAlnQcnSvdgpGIJZeHbFO9smyhPqsA7psFe9ZNGc";

// function declarations
void start_menu(void);
void fetchPlants(void);
void collectAndProcessData(void);
void matchPlants(void);
void displayRecommendations(void);
float celsiusToFahrenheit(float c);
String categorizeSunlight(float lux);

// sensor init
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_VEML7700 veml = Adafruit_VEML7700();

// lcd init
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 38
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// plant db storage
struct Plant {
  String name;
  int min_temp;
  int max_temp;
  int min_humidity;
  int max_humidity;
  String sunlight;
  int score; 

};

Plant plants[20];
int plantCount = 0;

// sensor data storage
struct SensorData {
  float temp_f;      // temp in Fahrenheit
  float humidity;    // humidity percentage
  float lux;         // light level
  String sunlight;   //  low, med, high
};

SensorData currentReading;

// top recommendations (indexes into plants array)
int topPlants[3] = {-1, -1, -1};
int topScores[3] = {0, 0, 0};


void setup() {
  Serial.begin(115200);
  while(!Serial) {
    delay(10);
  }

  // init i2c
  Wire.begin(11, 12);

  // Connect to sensors
  if (!veml.begin()) {
    Serial.println("VEML7700 not found");
    while (1);
  }
  Serial.println("VEML7700 found");

  veml.setLowThreshold(10000);
  veml.setHighThreshold(20000);
  veml.interruptEnable(true);

  if (!sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
  Serial.println("SHT31 found");

  // connect LCD
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  Serial.println("LCD found");

  // display connection message
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

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected!");
  display.println("Fetching plants...");
  display.display();

  fetchPlants();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Loaded ");
  display.print(plantCount);
  display.println(" plants");
  display.display();
  delay(2000);

  start_menu();
  delay(2000);

  Serial.println("\nstarting scan 𖡼.𖤣𖥧𖡼.𖤣𖥧");
  collectAndProcessData();
  matchPlants();
  displayRecommendations();
  
  Serial.println("\nscan complete 𖡼.𖤣𖥧𖡼.𖤣𖥧");
  Serial.println("Device idle. Reset to scan again.");

}



void loop() {
  delay(1000);
}


// fetch plants from db
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
    Serial.println("Database response:");
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
        plants[plantCount].name = obj["name"] | "";
        plants[plantCount].min_temp = obj["min_temp"] | 0;
        plants[plantCount].max_temp = obj["max_temp"] | 0;
        plants[plantCount].min_humidity = obj["min_humidity"] | 0;
        plants[plantCount].max_humidity = obj["max_humidity"] | 0;
        plants[plantCount].sunlight = obj["sunlight"] | "";
        plants[plantCount].score = 0;
        
        Serial.print("Loaded: ");
        Serial.println(plants[plantCount].name);
        
        plantCount++;
      }
    }
    
    Serial.print("\nTotal plants loaded: ");
    Serial.println(plantCount);
    
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(code);
    Serial.println(http.getString());
  }

  http.end();
}


// collect and average sensor readings over 30 samples, then categorize sunlight level
void collectAndProcessData() {
  const int NUM_SAMPLES = 30;
  float temp_sum = 0;
  float humidity_sum = 0;
  float lux_sum = 0;
  int valid_samples = 0;

  Serial.println("Collecting 30 samples over 30 seconds 𖡼.𖤣𖥧𖡼.𖤣𖥧");

  for (int i = 0; i < NUM_SAMPLES; i++) {
    // read sensors
    float t = sht31.readTemperature();
    float h = sht31.readHumidity();
    float l = veml.readLux();

    // validate readings
    if (!isnan(t) && !isnan(h)) {
      temp_sum += t;
      humidity_sum += h;
      lux_sum += l;
      valid_samples++;

      Serial.print("Sample ");
      Serial.print(i + 1);
      Serial.print("/");
      Serial.print(NUM_SAMPLES);
      Serial.print(" - Temp: ");
      Serial.print(t);
      Serial.print("°C, Humidity: ");
      Serial.print(h);
      Serial.print("%, Lux: ");
      Serial.println(l);

      // update LCD with current sample
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Sampling...");
      display.print("Sample: ");
      display.print(i + 1);
      display.print("/");
      display.println(NUM_SAMPLES);
      display.print("Temp: ");
      display.print(t, 1);
      display.println("C");
      display.print("Humidity: ");
      display.print(h, 1);
      display.println("%");
      display.print("Lux: ");
      display.println((int)l);
      display.display();
    } else {
      Serial.print("Sample ");
      Serial.print(i + 1);
      Serial.println(" - Invalid reading, skipping");
    }

    delay(1000);  // 1 second between samples
  }

  // calculate averages
  if (valid_samples > 0) {
    float avg_temp_c = temp_sum / valid_samples;
    currentReading.temp_f = celsiusToFahrenheit(avg_temp_c);
    currentReading.humidity = humidity_sum / valid_samples;
    currentReading.lux = lux_sum / valid_samples;
    currentReading.sunlight = categorizeSunlight(currentReading.lux);

    Serial.println("\nAverage Sensor Readings: ");
    Serial.print("Temperature: ");
    Serial.print(currentReading.temp_f);
    Serial.println("°F");
    Serial.print("Humidity: ");
    Serial.print(currentReading.humidity);
    Serial.println("%");
    Serial.print("Light: ");
    Serial.print(currentReading.lux);
    Serial.print(" lux (");
    Serial.print(currentReading.sunlight);
    Serial.println(")");
  } else {
    Serial.println("ERROR: No valid samples collected");
  }
}

// matching algorithm
void matchPlants() {
  Serial.println("\nMatching plants to environmental conditions 𖡼.𖤣𖥧𖡼.𖤣𖥧");
  
  // reset scores
  for (int i = 0; i < plantCount; i++) {
    plants[i].score = 0;
  }

  // score each plant
  for (int i = 0; i < plantCount; i++) {
    int score = 100;  // Start with perfect score

    // temp scoring
    if (currentReading.temp_f < plants[i].min_temp) {
      score -= abs(currentReading.temp_f - plants[i].min_temp) * 2;
    } else if (currentReading.temp_f > plants[i].max_temp) {
      score -= abs(currentReading.temp_f - plants[i].max_temp) * 2;
    }

    // humidity scoring
    if (currentReading.humidity < plants[i].min_humidity) {
      score -= abs(currentReading.humidity - plants[i].min_humidity);
    } else if (currentReading.humidity > plants[i].max_humidity) {
      score -= abs(currentReading.humidity - plants[i].max_humidity);
    }

    // sunlight scoring - convert database values to categories
    String plantSunlight = plants[i].sunlight;
    plantSunlight.toLowerCase();  // Convert to lowercase for matching
    
    String plantCategory = "medium";  // Default
    if (plantSunlight.indexOf("full") >= 0 || plantSunlight.indexOf("bright direct") >= 0) {
      plantCategory = "high";
    } else if (plantSunlight.indexOf("low") >= 0 || plantSunlight.indexOf("indirect") >= 0) {
      plantCategory = "low";
    } else if (plantSunlight.indexOf("bright") >= 0) {
      plantCategory = "medium";
    }
    
    // compare with current reading
    if (currentReading.sunlight != plantCategory) {
      score -= 30;  // Penalty for wrong light level
    }

    // ensure score doesn't go negative
    if (score < 0) score = 0;

    plants[i].score = score;

    Serial.print(plants[i].name);
    Serial.print(" (");
    Serial.print(plantSunlight);
    Serial.print(" -> ");
    Serial.print(plantCategory);
    Serial.print("): ");
    Serial.println(score);
  }

  // find top 3 plants
  for (int i = 0; i < 3; i++) {
    topPlants[i] = -1;
    topScores[i] = 0;
  }

  for (int i = 0; i < plantCount; i++) {
    for (int j = 0; j < 3; j++) {
      if (plants[i].score > topScores[j]) {
        // shift lower scores down
        for (int k = 2; k > j; k--) {
          topScores[k] = topScores[k-1];
          topPlants[k] = topPlants[k-1];
        }
        // insert this plant
        topScores[j] = plants[i].score;
        topPlants[j] = i;
        break;
      }
    }
  }

  Serial.println("\nTop Recommendations: ");
  for (int i = 0; i < 3; i++) {
    if (topPlants[i] != -1) {
      Serial.print(i + 1);
      Serial.print(". ");
      Serial.print(plants[topPlants[i]].name);
      Serial.print(" (Score: ");
      Serial.print(topScores[i]);
      Serial.println(")");
    }
  }
}

void displayRecommendations() {
  Serial.println("\nDisplaying recommendations 𖡼.𖤣𖥧𖡼.𖤣𖥧");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  display.println("Top Recommendations:");
  display.println("");
  
  //top 3
  for (int i = 0; i < 3; i++) {
    if (topPlants[i] != -1) {
      int plantIdx = topPlants[i];
      
      display.print(i + 1);
      display.print(". ");
      display.println(plants[plantIdx].name);
      
      Serial.print("Showing #");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(plants[plantIdx].name);
    }
  }
  
  display.display();
}

float celsiusToFahrenheit(float c) {
  return (c * 9.0 / 5.0) + 32.0;
}

String categorizeSunlight(float lux) {
  if (lux < 500) {
    return "low";
  } else if (lux < 2000) {
    return "medium";
  } else {
    return "high";
  }
}

void start_menu(void) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(30, 25);
  display.print("Ready!");
  display.display();
}
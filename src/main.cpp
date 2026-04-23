#include "main.h"

const char* ssid = "USC Guest Wireless";
const char* password = "";

const char* url = "https://nxvpnuvxrcjaeikjznxt.supabase.co/rest/v1/plants?select=name,min_temp,max_temp,min_humidity,max_humidity,sunlight";
const char* anonKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im54dnBudXZ4cmNqYWVpa2p6bnh0Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzE3MTcxMDIsImV4cCI6MjA4NzI5MzEwMn0.qGHLAlnQcnSvdgpGIJZeHbFO9smyhPqsA7psFe9ZNGc";

const int btn1 = 10;
const int btn2 = 17;
const int selectbtn = 18;
const int selectLED = 21;
const int LEDR = 8;
const int LEDG = 7;
const int LEDB = 6;

State currState = INIT;
State prevState = INIT;

byte lastBtn1State = HIGH;
byte lastBtn2State = HIGH;
byte lastSelectState = HIGH;

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_VEML7700 veml = Adafruit_VEML7700();
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Plant plants[20];
int plantCount = 0;
SensorData currentReading;
int topPlants[3] = {-1, -1, -1};
int topScores[3] = {0, 0, 0};
int selectedPlantIndex = 0;

bool vemlAvailable = false;
bool sht31Available = false;


void setup() {
  Serial.begin(115200);
  delay(2000); 
  
  Serial.println("\n\nSETUP STARTED");

  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);
  pinMode(selectbtn, INPUT_PULLUP);

  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  pinMode(selectLED, OUTPUT);

  setLED("green");  
  setSelectLED(true);
  
  Serial.println("Pins configured");
  
  currState = INIT;
}

void loop() {
  switch (currState) {
    case INIT:
      handleInit();
      break;
    case MENU:
      handleMenu();
      break;
    case SAMPLE:
      handleSample();
      break;
    case PROCESS:
      handleProcess();
      break;
    case REC_DISPLAY:
      handleRecDisplay();
      break;
    case DETAIL:
      handleDetail();
      break;
    case SYNC:
      handleSync();
      break;
    case IDLE:
      handleIdle();
      break;
    case ERROR:
      handleError();
      break;
  }
}

void scanI2C() {
  Serial.println("\nScan I2C Bus");
  byte count = 0;
  
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("Found device at 0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);
      
      if (addr == 0x3C) Serial.print(" (SSD1306 Display)");
      if (addr == 0x44) Serial.print(" (SHT31 Temp/Humidity)");
      if (addr == 0x45) Serial.print(" (SHT31 Alt Address)");
      if (addr == 0x10) Serial.print(" (VEML7700 Light)");
      
      Serial.println();
      count++;
    }
  }
  
  if (count == 0) {
    Serial.println("ERROR: No I2C devices found");
  } else {
    Serial.print("Total devices found: ");
    Serial.println(count);
  }
}

//state handlers
void handleInit() {
  Serial.println("\nSTATE: INIT");
  setLED("green");
  
  Wire.begin(11, 12);
  delay(100);
  scanI2C();

  Serial.println("Display init started");
  bool displayOK = false;
  for (int attempt = 0; attempt < 3; attempt++) {
    Serial.print("  Attempt ");
    Serial.print(attempt + 1);
    Serial.print("/3... ");
    
    if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      displayOK = true;
      Serial.println("SUCCESS");
      break;
    } else {
      Serial.println("FAILED");
      delay(500);
    }
  }
  
  if (!displayOK) {
    Serial.println("ERROR: Display init failed after 3 attempts");
    Serial.println("Continuing without display...");
  }

  if (displayOK) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Initializing...");
    display.display();
  }

  // Initialize VEML7700 light sensor
  Serial.println("\nChecking VEML7700 light sensor...");
  if (!veml.begin()) {
    Serial.println("  VEML7700 NOT FOUND - continuing without it");
    vemlAvailable = false;
  } else {
    Serial.println("  VEML7700 found OK");
    veml.setLowThreshold(10000);
    veml.setHighThreshold(20000);
    veml.interruptEnable(true);
    vemlAvailable = true;
  }

  // Initialize SHT31 temp/humidity sensor
  Serial.println("\nChecking SHT31 temp/humidity sensor...");
  Serial.print("  Trying address 0x44... ");
  if (!sht31.begin(0x44)) {
    Serial.println("FAILED");
    Serial.print("  Trying address 0x45... ");
    if (!sht31.begin(0x45)) {
      Serial.println("FAILED");
      Serial.println("  SHT31 NOT FOUND - continuing without it");
      sht31Available = false;
    } else {
      Serial.println("SUCCESS at 0x45");
      sht31Available = true;
    }
  } else {
    Serial.println("SUCCESS at 0x44");
    sht31Available = true;
  }

  if (sht31Available) {
    Serial.println("  SHT31 found OK");
    sht31Available = true;
    
    // Test reading immediately
    Serial.println("\n  Testing SHT31 initial reading...");
    delay(500);  // Give sensor time to warm up
    float test_t = sht31.readTemperature();
    float test_h = sht31.readHumidity();
    Serial.print("    Test temp: ");
    Serial.print(test_t);
    Serial.print(" (isnan? ");
    Serial.print(isnan(test_t) ? "YES" : "NO");
    Serial.println(")");
    Serial.print("    Test humidity: ");
    Serial.print(test_h);
    Serial.print(" (isnan? ");
    Serial.print(isnan(test_h) ? "YES" : "NO");
    Serial.println(")");
    
    if (isnan(test_t) || isnan(test_h)) {
      Serial.println("    WARNING: SHT31 returning NaN on first read!");
      Serial.println("    Trying reset...");
      sht31.reset();
      delay(100);
      test_t = sht31.readTemperature();
      test_h = sht31.readHumidity();
      Serial.print("    After reset - temp: ");
      Serial.print(test_t);
      Serial.print(", humidity: ");
      Serial.println(test_h);
    }
  }

  // Load mock plant data
  Serial.println("\nLoading mock plant data...");
  loadMockPlants();

  if (plantCount == 0) {
    Serial.println("ERROR: Failed to load mock plants!");
    currState = ERROR;
    return;
  }

  Serial.print("Successfully loaded ");
  Serial.print(plantCount);
  Serial.println(" plants");

  if (displayOK) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Loaded ");
    display.print(plantCount);
    display.println(" plants");
    
    if (!sht31Available || !vemlAvailable) {
      display.println("");
      display.println("Warning:");
      if (!sht31Available) display.println("No temp sensor");
      if (!vemlAvailable) display.println("No light sensor");
    }
    
    display.display();
  }
  
  delay(2000);

  Serial.println("\nINIT complete - transitioning to MENU");
  currState = MENU;
}

void handleMenu() {
  Serial.println("\n=== STATE: MENU ===");
  setLED("green");
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("==== MENU ====");
  display.println("");
  display.println("Press SELECT to");
  display.println("start scan");
  display.println("");
  display.println("BTN1: Settings");
  display.println("BTN2: Info");
  display.display();

  // Wait for user input
  while (currState == MENU) {
    if (selectPressed()) {
      Serial.println("SELECT pressed - starting scan");
      delay(50);
      currState = SAMPLE;
      break;
    }
    delay(100);
  }
}

void handleSample() {
  Serial.println("\n=== STATE: SAMPLE ===");
  setLED("blue");
  
  collectAndProcessData();
  
  currState = PROCESS;
}

void handleProcess() {
  Serial.println("\n=== STATE: PROCESS ===");
  setLED("blue");
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Processing...");
  display.display();
  
  matchPlants();
  
  currState = REC_DISPLAY;
}

void handleRecDisplay() {
  Serial.println("\n=== STATE: REC_DISPLAY ===");
  setLED("green");
  
  displayRecommendations();
  
  display.println("");
  display.println("SELECT: Details");
  display.println("BTN1: New Scan");
  display.display();

  // Wait for user input
  while (currState == REC_DISPLAY) {
    if (selectPressed()) {
      Serial.println("SELECT pressed - showing details");
      delay(50);
      selectedPlantIndex = 0;
      currState = DETAIL;
      break;
    }
    if (btn1Pressed()) {
      Serial.println("BTN1 pressed - new scan");
      delay(50);
      currState = MENU;
      break;
    }
    delay(100);
  }
}

void handleDetail() {
  Serial.println("\n=== STATE: DETAIL ===");
  setLED("green");
  
  if (topPlants[selectedPlantIndex] == -1) {
    Serial.println("No plant at this index, returning to display");
    currState = REC_DISPLAY;
    return;
  }

  int plantIdx = topPlants[selectedPlantIndex];
  
  Serial.print("Showing details for: ");
  Serial.println(plants[plantIdx].name);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  display.println(plants[plantIdx].name);
  display.println("---");
  display.print("Temp: ");
  display.print(plants[plantIdx].min_temp);
  display.print("-");
  display.print(plants[plantIdx].max_temp);
  display.println("F");
  
  display.print("Humidity: ");
  display.print(plants[plantIdx].min_humidity);
  display.print("-");
  display.print(plants[plantIdx].max_humidity);
  display.println("%");
  
  display.print("Light: ");
  display.println(plants[plantIdx].sunlight);
  
  display.println("");
  display.println("BTN1/2: Navigate");
  display.println("SELECT: Back");
  display.display();

  // Navigation
  while (currState == DETAIL) {
    if (btn1Pressed()) {
      delay(50);
      selectedPlantIndex = (selectedPlantIndex + 1) % 3;
      if (topPlants[selectedPlantIndex] != -1) {
        Serial.print("Next plant: index ");
        Serial.println(selectedPlantIndex);
        break;
      }
    }
    if (btn2Pressed()) {
      delay(50);
      selectedPlantIndex = (selectedPlantIndex - 1 + 3) % 3;
      if (topPlants[selectedPlantIndex] != -1) {
        Serial.print("Previous plant: index ");
        Serial.println(selectedPlantIndex);
        break;
      }
    }
    if (selectPressed()) {
      Serial.println("SELECT pressed - back to display");
      delay(50);
      currState = REC_DISPLAY;
      return;
    }
    delay(100);
  }
}

void handleSync() {
  Serial.println("\n=== STATE: SYNC ===");
  setLED("blue");
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Syncing...");
  display.println("(BLE coming soon)");
  display.display();
  
  delay(2000);
  currState = MENU;
}

void handleIdle() {
  Serial.println("\n=== STATE: IDLE ===");
  setLED("off");
  
  display.clearDisplay();
  display.display();
  
  if (selectPressed() || btn1Pressed() || btn2Pressed()) {
    Serial.println("Button pressed - waking up");
    delay(50);
    currState = MENU;
  }
  
  delay(1000);
}

void handleError() {
  Serial.println("\n=== STATE: ERROR ===");
  setLED("red");
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("== ERROR ==");
  display.println("");
  display.println("Sensor failure");
  display.println("or connection");
  display.println("issue");
  display.println("");
  display.println("Press SELECT");
  display.println("to retry");
  display.display();

  while (currState == ERROR) {
    if (selectPressed()) {
      Serial.println("SELECT pressed - retrying init");
      delay(50);
      currState = INIT;
      break;
    }
    delay(100);
  }
}

//Data handling functions
void loadMockPlants() {
  Serial.println("Loading mock plant data...");
  
  plants[0].name = "Snake Plant";
  plants[0].min_temp = 60;
  plants[0].max_temp = 85;
  plants[0].min_humidity = 30;
  plants[0].max_humidity = 50;
  plants[0].sunlight = "low";
  plants[0].score = 0;
  
  plants[1].name = "Pothos";
  plants[1].min_temp = 65;
  plants[1].max_temp = 80;
  plants[1].min_humidity = 40;
  plants[1].max_humidity = 60;
  plants[1].sunlight = "medium";
  plants[1].score = 0;
  
  plants[2].name = "Spider Plant";
  plants[2].min_temp = 55;
  plants[2].max_temp = 80;
  plants[2].min_humidity = 40;
  plants[2].max_humidity = 65;
  plants[2].sunlight = "bright indirect";
  plants[2].score = 0;
  
  plants[3].name = "Aloe Vera";
  plants[3].min_temp = 55;
  plants[3].max_temp = 80;
  plants[3].min_humidity = 10;
  plants[3].max_humidity = 30;
  plants[3].sunlight = "bright direct";
  plants[3].score = 0;
  
  plants[4].name = "Peace Lily";
  plants[4].min_temp = 65;
  plants[4].max_temp = 80;
  plants[4].min_humidity = 50;
  plants[4].max_humidity = 70;
  plants[4].sunlight = "low";
  plants[4].score = 0;
  
  plantCount = 5;
  
  Serial.print("Loaded ");
  Serial.print(plantCount);
  Serial.println(" mock plants:");
  for (int i = 0; i < plantCount; i++) {
    Serial.print("  ");
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.println(plants[i].name);
  }
}

void matchPlants() {
  Serial.println("Matching plants to environmental conditions...\n");
  
  // Reset scores
  for (int i = 0; i < plantCount; i++) {
    plants[i].score = 0;
  }

  // Score each plant
  for (int i = 0; i < plantCount; i++) {
    int score = 100;

    // Temperature scoring
    if (currentReading.temp_f < plants[i].min_temp) {
      score -= abs(currentReading.temp_f - plants[i].min_temp) * 2;
    } else if (currentReading.temp_f > plants[i].max_temp) {
      score -= abs(currentReading.temp_f - plants[i].max_temp) * 2;
    }

    // Humidity scoring
    if (currentReading.humidity < plants[i].min_humidity) {
      score -= abs(currentReading.humidity - plants[i].min_humidity);
    } else if (currentReading.humidity > plants[i].max_humidity) {
      score -= abs(currentReading.humidity - plants[i].max_humidity);
    }

    // Sunlight scoring
    String plantSunlight = plants[i].sunlight;
    plantSunlight.toLowerCase();
    
    String plantCategory = "medium";
    if (plantSunlight.indexOf("full") >= 0 || plantSunlight.indexOf("bright direct") >= 0) {
      plantCategory = "high";
    } else if (plantSunlight.indexOf("low") >= 0 || plantSunlight.indexOf("indirect") >= 0) {
      plantCategory = "low";
    } else if (plantSunlight.indexOf("bright") >= 0) {
      plantCategory = "medium";
    }
    
    if (currentReading.sunlight != plantCategory) {
      score -= 30;
    }

    if (score < 0) score = 0;
    plants[i].score = score;

    Serial.print(plants[i].name);
    Serial.print(" (");
    Serial.print(plantSunlight);
    Serial.print(" -> ");
    Serial.print(plantCategory);
    Serial.print("): Score = ");
    Serial.println(score);
  }

  // Find top 3
  for (int i = 0; i < 3; i++) {
    topPlants[i] = -1;
    topScores[i] = 0;
  }

  for (int i = 0; i < plantCount; i++) {
    for (int j = 0; j < 3; j++) {
      if (plants[i].score > topScores[j]) {
        for (int k = 2; k > j; k--) {
          topScores[k] = topScores[k-1];
          topPlants[k] = topPlants[k-1];
        }
        topScores[j] = plants[i].score;
        topPlants[j] = i;
        break;
      }
    }
  }

  Serial.println("\n=== Top Recommendations ===");
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
  Serial.println("==========================\n");
}

void collectAndProcessData() {
  const int NUM_SAMPLES = 5;
  float temp_sum = 0;
  float humidity_sum = 0;
  float lux_sum = 0;
  int valid_samples = 0;

  Serial.println("\nCollecting sensor samples...");
  Serial.print("SHT31 available: ");
  Serial.println(sht31Available ? "YES" : "NO");
  Serial.print("VEML available: ");
  Serial.println(vemlAvailable ? "YES" : "NO");

  for (int i = 0; i < NUM_SAMPLES; i++) {
    float t = NAN;
    float h = NAN;
    float l = 0;

    // Read SHT31
    if (sht31Available) {
      Serial.print("\n--- Sample ");
      Serial.print(i + 1);
      Serial.println(" ---");
      
      Serial.print("Reading SHT31... ");
      t = sht31.readTemperature();
      h = sht31.readHumidity();
      
      Serial.print("Raw temp = ");
      Serial.print(t);
      Serial.print(", Raw humidity = ");
      Serial.println(h);
      
      Serial.print("isnan(temp)? ");
      Serial.print(isnan(t) ? "YES (BAD)" : "NO (GOOD)");
      Serial.print(", isnan(humidity)? ");
      Serial.println(isnan(h) ? "YES (BAD)" : "NO (GOOD)");
    } else {
      Serial.print("Sample ");
      Serial.print(i + 1);
      Serial.println(" - SHT31 not available, using mock data");
      t = 22.0 + random(-2, 3);
      h = 45.0 + random(-5, 6);
    }

    // Read VEML7700
    if (vemlAvailable) {
      Serial.print("Reading VEML7700... ");
      l = veml.readLux();
      Serial.print("Raw lux = ");
      Serial.println(l);
    } else {
      l = 500 + random(-100, 101);
    }

    // Validate and sum
    Serial.print("Validation check: ");
    if (!isnan(t) && !isnan(h)) {
      Serial.println("VALID - adding to sum");
      temp_sum += t;
      humidity_sum += h;
      lux_sum += l;
      valid_samples++;

      Serial.print("Running totals - temp_sum: ");
      Serial.print(temp_sum);
      Serial.print(", humidity_sum: ");
      Serial.print(humidity_sum);
      Serial.print(", lux_sum: ");
      Serial.print(lux_sum);
      Serial.print(", valid_samples: ");
      Serial.println(valid_samples);

      // Update LCD
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
      Serial.print("INVALID - skipping. ");
      if (isnan(t)) Serial.print("Temp is NaN. ");
      if (isnan(h)) Serial.print("Humidity is NaN. ");
      Serial.println();
    }

    delay(1000);
  }

  Serial.println("\n=== Final Summary ===");
  Serial.print("Total valid samples: ");
  Serial.println(valid_samples);
  Serial.print("temp_sum: ");
  Serial.println(temp_sum);
  Serial.print("humidity_sum: ");
  Serial.println(humidity_sum);
  Serial.print("lux_sum: ");
  Serial.println(lux_sum);

  // Calculate averages
  if (valid_samples > 0) {
    float avg_temp_c = temp_sum / valid_samples;
    currentReading.temp_f = celsiusToFahrenheit(avg_temp_c);
    currentReading.humidity = humidity_sum / valid_samples;
    currentReading.lux = lux_sum / valid_samples;
    currentReading.sunlight = categorizeSunlight(currentReading.lux);

    Serial.println("\n=== Average Sensor Readings ===");
    Serial.print("Avg temp (C): ");
    Serial.println(avg_temp_c, 2);
    Serial.print("Temperature: ");
    Serial.print(currentReading.temp_f, 1);
    Serial.println("°F");
    Serial.print("Humidity: ");
    Serial.print(currentReading.humidity, 1);
    Serial.println("%");
    Serial.print("Light: ");
    Serial.print(currentReading.lux, 0);
    Serial.print(" lux (");
    Serial.print(currentReading.sunlight);
    Serial.println(")");
    Serial.println("==============================\n");
  } else {
    Serial.println("ERROR: No valid samples collected - all readings were NaN");
    Serial.println("This suggests the SHT31 sensor is responding on I2C but not providing valid data.");
    Serial.println("Possible issues:");
    Serial.println("  - Sensor needs more warm-up time");
    Serial.println("  - Power supply issue");
    Serial.println("  - Faulty sensor");
  }
}


void displayRecommendations() {
  Serial.println("Displaying recommendations on screen");
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  display.println("Top Recommendations:");
  display.println("");
  
  for (int i = 0; i < 3; i++) {
    if (topPlants[i] != -1) {
      int plantIdx = topPlants[i];
      display.print(i + 1);
      display.print(". ");
      display.println(plants[plantIdx].name);
    }
  }
  
  display.display();
}

//Calibration functions
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

void setLED(const char* color) {
  digitalWrite(LEDR, LOW);
  digitalWrite(LEDG, LOW);
  digitalWrite(LEDB, LOW);

  if (strcmp(color, "red") == 0) {
    digitalWrite(LEDR, HIGH);
  } else if (strcmp(color, "green") == 0) {
    digitalWrite(LEDG, HIGH);
  } else if (strcmp(color, "blue") == 0) {
    digitalWrite(LEDB, HIGH);
  } else if (strcmp(color, "off") == 0) {
  }
}

void setSelectLED(bool state) {
  digitalWrite(selectLED, state ? HIGH : LOW);
}

//button functions
bool btn1Pressed() {
  byte currentState = digitalRead(btn1);
  if (currentState == LOW && lastBtn1State == HIGH) {
    lastBtn1State = currentState;
    return true;
  }
  lastBtn1State = currentState;
  return false;
}

bool btn2Pressed() {
  byte currentState = digitalRead(btn2);
  if (currentState == LOW && lastBtn2State == HIGH) {
    lastBtn2State = currentState;
    return true;
  }
  lastBtn2State = currentState;
  return false;
}

bool selectPressed() {
  byte currentState = digitalRead(selectbtn);
  if (currentState == LOW && lastSelectState == HIGH) {
    lastSelectState = currentState;
    return true;
  }
  lastSelectState = currentState;
  return false;
}

//DB functions
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
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

//state handlers
void handleInit() {
  Serial.println("\nSTATE: INIT");
  setLED("green");
  
  // Initialize I2C
  Wire.begin(11, 12);
  delay(100);

  // Initialize display (simple, no retry)
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Display init failed");
  } else {
    Serial.println("Display OK");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Initializing");
    display.display();
  }

  // Initialize VEML7700
  vemlAvailable = veml.begin();
  Serial.print("VEML7700: ");
  Serial.println(vemlAvailable ? "OK" : "NOT FOUND");
  
  if (vemlAvailable) {
    veml.setLowThreshold(10000);
    veml.setHighThreshold(20000);
    veml.interruptEnable(true);
  }

  // Initialize SHT31
  sht31Available = sht31.begin(0x44);
  Serial.print("SHT31: ");
  Serial.println(sht31Available ? "OK" : "NOT FOUND");

  // Load mock plant data
  loadMockPlants();

  if (plantCount == 0) {
    Serial.println("ERROR: Failed to load mock plants!");
    currState = ERROR;
    return;
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Loaded ");
  display.print(plantCount);
  display.println(" plants");
  
  if (!sht31Available || !vemlAvailable) {
    display.println("\nUsing mock data");
  }
  
  display.display();
  delay(2000);

  Serial.println("INIT complete");
  currState = MENU;
}

void handleMenu() {
  Serial.println("\n STATE: MENU ");
  setLED("green");
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("---- MENU ----");
  display.println("");
  display.println("Press SELECT to");
  display.println("start scan");
  display.println("");
  display.println("BTN1: Settings");
  display.println("BTN2: Info");
  display.display();

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
  Serial.println("\n STATE: SAMPLE ");
  setLED("blue");
  
  collectAndProcessData();
  
  currState = PROCESS;
}

void handleProcess() {
  Serial.println("\n STATE: PROCESS ");
  setLED("blue");
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Processing");
  display.display();
  
  matchPlants();
  
  currState = REC_DISPLAY;
}

void handleRecDisplay() {
  Serial.println("\n STATE: REC_DISPLAY ");
  setLED("green");
  
  displayRecommendations();
  
  display.println("");
  display.println("SELECT: Details");
  display.println("BTN1: New Scan");
  display.display();

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
  Serial.println("\n STATE: DETAIL ");
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
  Serial.println("\n STATE: SYNC ");
  setLED("blue");
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Syncing");
  display.println("(BLE coming soon)");
  display.display();
  
  delay(2000);
  currState = MENU;
}

void handleIdle() {
  Serial.println("\n STATE: IDLE ");
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
  Serial.println("\n STATE: ERROR ");
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
  Serial.println("Loading mock plant data");
  
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
  Serial.println(" mock plants");
}

void matchPlants() {
  Serial.println("Matching plants\n");
  
  for (int i = 0; i < plantCount; i++) {
    plants[i].score = 0;
  }

  for (int i = 0; i < plantCount; i++) {
    int score = 100;

    if (currentReading.temp_f < plants[i].min_temp) {
      score -= abs(currentReading.temp_f - plants[i].min_temp) * 2;
    } else if (currentReading.temp_f > plants[i].max_temp) {
      score -= abs(currentReading.temp_f - plants[i].max_temp) * 2;
    }

    if (currentReading.humidity < plants[i].min_humidity) {
      score -= abs(currentReading.humidity - plants[i].min_humidity);
    } else if (currentReading.humidity > plants[i].max_humidity) {
      score -= abs(currentReading.humidity - plants[i].max_humidity);
    }

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
    Serial.print(": ");
    Serial.println(score);
  }

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

  Serial.println("\nTop 3:");
  for (int i = 0; i < 3; i++) {
    if (topPlants[i] != -1) {
      Serial.print(i + 1);
      Serial.print(". ");
      Serial.print(plants[topPlants[i]].name);
      Serial.print(" (");
      Serial.print(topScores[i]);
      Serial.println(")");
    }
  }
}

void collectAndProcessData() {
  const int NUM_SAMPLES = 5;
  float temp_sum = 0;
  float humidity_sum = 0;
  float lux_sum = 0;
  int valid_samples = 0;

  Serial.println("\nCollecting samples");

  for (int i = 0; i < NUM_SAMPLES; i++) {
    float t, h, l;

    if (sht31Available) {
      t = sht31.readTemperature();
      h = sht31.readHumidity();
    } else {
      t = 22.0 + random(-2, 3);
      h = 45.0 + random(-5, 6);
    }

    if (vemlAvailable) {
      l = veml.readLux();
    } else {
      l = 500 + random(-100, 101);
    }

    if (!isnan(t) && !isnan(h)) {
      temp_sum += t;
      humidity_sum += h;
      lux_sum += l;
      valid_samples++;

      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Sampling");
      display.print("Sample: ");
      display.print(i + 1);
      display.print("/");
      display.println(NUM_SAMPLES);
      display.print("Temp: ");
      display.print(t, 1);
      display.println("C");
      display.print("Hum: ");
      display.print(h, 1);
      display.println("%");
      display.print("Lux: ");
      display.println((int)l);
      display.display();
    }

    delay(1000);
  }

  if (valid_samples > 0) {
    float avg_temp_c = temp_sum / valid_samples;
    currentReading.temp_f = celsiusToFahrenheit(avg_temp_c);
    currentReading.humidity = humidity_sum / valid_samples;
    currentReading.lux = lux_sum / valid_samples;
    currentReading.sunlight = categorizeSunlight(currentReading.lux);

    Serial.println("\nAverages:");
    Serial.print("Temp: ");
    Serial.print(currentReading.temp_f, 1);
    Serial.println("F");
    Serial.print("Humidity: ");
    Serial.print(currentReading.humidity, 1);
    Serial.println("%");
    Serial.print("Lux: ");
    Serial.print(currentReading.lux, 0);
    Serial.print(" (");
    Serial.print(currentReading.sunlight);
    Serial.println(")");
  } else {
    Serial.println("No valid samples");
  }
}

void displayRecommendations() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  display.println("Top Plants:");
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

float celsiusToFahrenheit(float c) {
  return (c * 9.0 / 5.0) + 32.0;
}

String categorizeSunlight(float lux) {
  if (lux < 500) return "low";
  else if (lux < 2000) return "medium";
  else return "high";
}

void setLED(const char* color) {
  digitalWrite(LEDR, LOW);
  digitalWrite(LEDG, LOW);
  digitalWrite(LEDB, LOW);

  if (strcmp(color, "red") == 0) digitalWrite(LEDR, HIGH);
  else if (strcmp(color, "green") == 0) digitalWrite(LEDG, HIGH);
  else if (strcmp(color, "blue") == 0) digitalWrite(LEDB, HIGH);
}

void setSelectLED(bool state) {
  digitalWrite(selectLED, state ? HIGH : LOW);
}

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
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, payload);

    if (!err) {
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
          plantCount++;
        }
      }
    }
  }

  http.end();
}
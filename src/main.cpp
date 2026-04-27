#include "main.h"

const char* ssid = "USC Guest Wireless";
const char* password = "";

const char* url = "https://nxvpnuvxrcjaeikjznxt.supabase.co/rest/v1/plants?select=id,name,min_temp,max_temp,min_humidity,max_humidity,sunlight";
const char* anonKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im54dnBudXZ4cmNqYWVpa2p6bnh0Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzE3MTcxMDIsImV4cCI6MjA4NzI5MzEwMn0.qGHLAlnQcnSvdgpGIJZeHbFO9smyhPqsA7psFe9ZNGc";

const int btn1 = 10;
const int btn2 = 17;
const int selectbtn = 18;
const int selectLED = 21;
const int LEDR = 8;
const int LEDG = 7;
const int LEDB = 6;
const int RST = 9;

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
  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH);

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
    case READINGS:
      handleReadings();
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

  // Initialize display
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

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

Serial.println("\nWiFi connected!");
 
  fetchPlants();

  if (plantCount == 0) {
    Serial.println("ERROR: Failed to load plants!");
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
  display.println("      ~~ MENU ~~");
  display.println("Press SELECT to Start");
  display.drawBitmap((display.width() - FLOWER_WIDTH ) / 2,
    (display.height() - FLOWER_HEIGHT) / 2,
    flower_btmp, 24, 24, 1);
  display.println("");
  display.println("");
  display.println("");
  display.println("");
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
  setLED("red");
  
  collectAndProcessData();
  
  currState = PROCESS;
}

void handleProcess() {
  Serial.println("\n STATE: PROCESS ");
  setLED("red");
  
  display.clearDisplay();
  
  matchPlants();
  
  currState = REC_DISPLAY;
}

void handleRecDisplay() {
  Serial.println("\n STATE: REC_DISPLAY ");
  setLED("green");
  
  displayRecommendations();
  
  display.println("");
  display.println("BTN1: Details");
  display.println("SELECT: New Scan");
  display.println("BTN2: Sensor Readings");
  display.display();

  while (currState == REC_DISPLAY) {
    if (btn1Pressed()) {
      Serial.println("BTN1 pressed - showing details");
      delay(50);
      selectedPlantIndex = 0;
      currState = DETAIL;
      break;
    }
    if (selectPressed()) {
      Serial.println("SELECT pressed - new scan");
      delay(50);
      currState = MENU;
      break;
    }
    if (btn2Pressed()) {
      Serial.println("BTN2 pressed - showing sensor readings");
      delay(50);
      currState = READINGS;
      break;  
    }

    delay(100);
  }
}

void handleDetail() {
  Serial.println("\n STATE: DETAIL ");
  setLED("blue");
  
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

void handleReadings() {

  currState = READINGS;
  Serial.println("\n STATE: READINGS ");
  setLED("blue");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Sensor Readings:");
  display.println("");
  display.print("Temp: ");
  display.print(currentReading.temp_f);
  display.println(" F");
  
  display.print("Humidity: ");
  display.print(currentReading.humidity);
  display.println(" %");
  
  display.print("Light: ");
  display.print(currentReading.lux);
  display.println(" lux");
  
  display.println("");
  display.println("Press SELECT to go back");
  display.display();

  while (currState == READINGS) {
    if (selectPressed()) {
      Serial.println("SELECT pressed - going back");
      delay(50);
      currState = REC_DISPLAY;
      break;
    }
    delay(100);
  }


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


int lightLevel(String s) {
  if (s == "low") return 0;
  if (s == "medium") return 1;
  if (s == "high") return 2;
  return 1;
}

void matchPlants() {
  Serial.println("Matching plants\n");
  
  for (int i = 0; i < plantCount; i++) {
    plants[i].score = 0;
  }

  for (int i = 0; i < plantCount; i++) {

  // --- TEMPERATURE SCORE ---
  float temp = currentReading.temp_f;
  float minT = plants[i].min_temp;
  float maxT = plants[i].max_temp;
  float midT = (minT + maxT) / 2.0;

  int tempScore;

  if (temp >= minT && temp <= maxT) {
    // inside range -> reward closeness to center
    float dist = abs(temp - midT);
    float maxDist = (maxT - minT) / 2.0;
    tempScore = 100 - (dist / maxDist) * 30; // only small penalty
  } else {
    // outside range -> heavy penalty
    float dist;
    if (temp < minT) dist = minT - temp;
    else dist = temp - maxT;

    tempScore = 70 - dist * 3; // harsher drop
  }

  tempScore = constrain(tempScore, 0, 100);

  // --- HUMIDITY SCORE ---
  float hum = currentReading.humidity;
  float minH = plants[i].min_humidity;
  float maxH = plants[i].max_humidity;
  float midH = (minH + maxH) / 2.0;

  int humScore;

  if (hum >= minH && hum <= maxH) {
    float dist = abs(hum - midH);
    float maxDist = (maxH - minH) / 2.0;
    humScore = 100 - (dist / maxDist) * 30;
  } else {
    float dist;
    if (hum < minH) dist = minH - hum;
    else dist = hum - maxH;

    humScore = 70 - dist * 2;
  }

  humScore = constrain(humScore, 0, 100);

  // --- SUNLIGHT SCORE ---
  int lightScore;

  // Map plant sunlight → level (1–3)
  String plantSunlight = plants[i].sunlight;
  plantSunlight.toLowerCase();

int plantLevel;

if (plantSunlight.indexOf("full") >= 0) plantLevel = 3;
else if (plantSunlight.indexOf("bright") >= 0) plantLevel = 2;
else if (plantSunlight.indexOf("indirect") >= 0) plantLevel = 1;
else plantLevel = 0; // low fallback

int currentLevel;

if (currentReading.lux < 500) currentLevel = 0;        // low
else if (currentReading.lux < 2000) currentLevel = 1;  // indirect
else if (currentReading.lux < 10000) currentLevel = 2; // bright indirect
else currentLevel = 3; // full

int diff = abs(currentLevel - plantLevel);

// smoother curve
lightScore = 100 - (diff * 30);
lightScore = constrain(lightScore, 0, 100);

  // --- FINAL WEIGHTED SCORE ---
  int finalScore =
    (tempScore * 0.4) +
    (humScore * 0.3) +
    (lightScore * 0.3);

  plants[i].score = finalScore;

  Serial.print(plants[i].name);
  Serial.print(": ");
  Serial.println(finalScore);
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
  if (topPlants[0] != -1) {
  int idx = topPlants[0];

  uploadTopPlant(
    plants[idx].id,
    topScores[0],
    "Location",
    currentReading.temp_c,
    currentReading.humidity,
    currentReading.lux
  );
}
}

void uploadTopPlant(String plantId, int score, String location, float temp, float humidity, float light) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, "https://nxvpnuvxrcjaeikjznxt.supabase.co/rest/v1/recommendations");

  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", anonKey);
  http.addHeader("Authorization", String("Bearer ") + anonKey);
  http.addHeader("Prefer", "return=minimal"); // optional but cleaner

  String body = "{";
  body += "\"plant_id\":\"" + plantId + "\",";
  body += "\"location\":\"" + location + "\",";
  body += "\"score\":" + String(score) + ",";
  body += "\"temperature\":" + String(temp, 1) + ",";
  body += "\"humidity\":" + String(humidity, 1) + ",";
  body += "\"light\":" + String(light, 1);
  body += "}";

  int code = http.POST(body);

  Serial.print("Upload response: ");
  Serial.println(code);

  if (code > 0) {
    String response = http.getString();
    Serial.println(response); // helps debugging
  } else {
    Serial.println("Request failed");
  }

  http.end();
}

void collectAndProcessData() {
  const int NUM_SAMPLES = 8;
  float temp_sum = 0;
  float humidity_sum = 0;
  float lux_sum = 0;
  int valid_samples = 0;
  int tile_num = 1;
  int cursor_pixel = 0;

  Serial.println("\nCollecting samples");
  display.clearDisplay();

  for (int i = 0; i < NUM_SAMPLES; i++) {
    float t, h, l;

    if (sht31Available) {
      t = sht31.readTemperature();
      h = sht31.readHumidity();
    } 
    // else {
    //   t = 22.0 + random(-2, 3);
    //   h = 45.0 + random(-5, 6);
    // }

    if (vemlAvailable) {
      l = veml.readLux();
    } 
    // else {
    //   l = 500 + random(-100, 101);
    // }

    if (!isnan(t) && !isnan(h)) {
      temp_sum += t;
      humidity_sum += h;
      lux_sum += l;
      valid_samples++;

    //display loading screen
    if(tile_num == 1){
      display.drawBitmap(0,
        cursor_pixel,
        tile_1, TILE_WIDTH, TILE_HEIGHT, 1);

        tile_num = 2;
    }
    else{
      display.drawBitmap(0,
        cursor_pixel,
        tile_2, TILE_WIDTH, TILE_HEIGHT, 1);

        tile_num = 1;
    }
    cursor_pixel += 16;
    display.display();

      if(cursor_pixel == 64){
        display.clearDisplay();
        cursor_pixel = 0;
      }
    }
    delay(1000);
  }

  if (valid_samples > 0) {
    float avg_temp_c = temp_sum / valid_samples;
    currentReading.temp_f = celsiusToFahrenheit(avg_temp_c);
    currentReading.temp_c = avg_temp_c;
    currentReading.humidity = humidity_sum / valid_samples;
    currentReading.lux = lux_sum / valid_samples;
    currentReading.sunlight = categorizeSunlight(currentReading.lux);

    if(currentReading.lux == 0 || currentReading.temp_c == 0){
      display.clearDisplay();
      display.println("An Issue Occurred: ");
      display.println("Restarting...");
      display.display();
      delay(500);
      digitalWrite(RST, LOW);
    }

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

  for (int i = 0; i < 3; i++) {
    if (topPlants[i] != -1) {
      int plantIdx = topPlants[i];
      
      display.print(i + 1);
      display.print(". ");
      display.print(plants[plantIdx].name);
      display.print(" (");
      display.print(topScores[i]);
      display.println(")");
    }
  }
  
  display.display();
}
float celsiusToFahrenheit(float c) {
  return (c * 9.0 / 5.0) + 32.0;
}

String categorizeSunlight(float lux) {
  if (lux < 500) return "low";
  else if (lux < 2000) return "indirect";
  else if (lux < 10000) return "bright indirect";
  else return "full";
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
          plants[plantCount].id = obj["id"] | "";
          plants[plantCount].name = obj["name"] | "";
          plants[plantCount].min_temp = (obj["min_temp"] | 0) * 9.0 / 5.0 + 32;
          plants[plantCount].max_temp = (obj["max_temp"] | 0) * 9.0 / 5.0 + 32;
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
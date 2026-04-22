#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define LED_PIN1 6;
#define LED_PIN2 7;
#define LED_PIN3 8;


// Pin definitions - using raw GPIO numbers
const int btn1 = 10;   // GPIO7 top
const int btn2 = 17;   // GPIO8 bottom
const int selectbtn = 18;    // GPIO9 (green button with LED)
const int selectLED = 21;
const int ledRed = 7;
const int ledGreen = 6;


#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define OLED_RESET      38
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  
  //active LOW 
  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);
  pinMode(selectbtn, INPUT_PULLUP);

  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(selectLED, OUTPUT);



  Wire.begin(11, 12);
  
  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Display init FAILED!");
    while (1);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
}

void loop() {
  
  // 1. read input for each button
  int btn1State = digitalRead(btn1);
  int btn2State = digitalRead(btn2);
  int selectState = digitalRead(selectbtn);

  // 2. Prepare the display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);

  // 3. print state of button
  if (btn1State == LOW) {
    display.println("BTN 1: PRESSED");
  } else {
    display.println("BTN 1: released");
  }

  if (btn2State == LOW) {
    display.println("BTN 2: PRESSED");
  } else {
    display.println("BTN 2: released");
  }

  if (selectState == LOW) {
    display.println("SELECT: PRESSED");
  } else {
    display.println("SELECT: released");
  }

  // 4. Push everything to LCD screen
  display.display();
}

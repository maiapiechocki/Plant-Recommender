#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include "Adafruit_SHT31.h"
#include "Adafruit_VEML7700.h"

//SHT31 init
Adafruit_SHT31 sht31 = Adafruit_SHT31();

//LCD init
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     38 // Reset pin # (D11/GPIO38)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//VEML7700 init
Adafruit_VEML7700 veml = Adafruit_VEML7700();


void setup() {
  //setup serial
  Serial.begin(115200);
  while(!Serial){
    delay(10);
  }

  //init i2c
  Wire.begin(11,12);

  //connect to VEML7700
  if (!veml.begin()) {
    Serial.println("VEML7700 not found");
    while (1);
  }
  Serial.println("VEML7700 found");

  //init VEML7700
  veml.setLowThreshold(10000);
  veml.setHighThreshold(20000);
  veml.interruptEnable(true);

  //connect to SHT31
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
  Serial.println("SHT31 found");

  //connect LCD
   // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the LCD buffer
  display.clearDisplay();
  start_menu();    // Draw circles (filled)
  delay(20);

}


void loop() {

  //STEP 1:
    //configure a digital pin to wait for button press (green LED button)
    //check schematic for button wiring 
    //flash green led in button on and off while waiting for user input 

  //STEP 2:
    //on button press, turn off button LED and run sensor readings for 
    //allotted amount of time (readings every 1 second for 5 seconds total?)
    //save data into some structure to use for processing in algo
    
    //maybe add some sort of loading screen while data is being captured
    //wire RBG LED to turn red while its taking readings and running algo

  //STEP 3:
    //run algo on data (take averages, convert celcius to Ferinheight, convert
    //to some string based on predetermined value range (example: lux 100-200 = 
    //partial sun, lux 300+ = full light exposure, lux <100: low light), 
    //run some algo to compare to database to come up with few recommendations
    //and save them)
  
  //STEP 4:
    //turn RGB LED to Green to indicate its done
    //display results to user via LCD
    //configure small push buttons to navigate screen (check schematic for wiring)
    //configure green LED button to light up again and to select recommendations 
    //to show more info
    //figure out how we want to allow user to rerun program (double press on green 
    //button?)

  //CODE TO RUN SENSORS 
  //print SHT31 readings 
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (! isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
  } else { 
    Serial.println("Failed to read temperature");
  }
  
  if (! isnan(h)) {  // check if 'is not a number'
    Serial.print("Hum. % = "); Serial.println(h);
  } else { 
    Serial.println("Failed to read humidity");
  }

  //Print VEML readings to serial terminal 
  Serial.print("lux: "); Serial.println(veml.readLux());

  uint16_t irq = veml.interruptStatus();
  if (irq & VEML7700_INTERRUPT_LOW) {
    Serial.println("** Low threshold");
  }
  if (irq & VEML7700_INTERRUPT_HIGH) {
    Serial.println("** High threshold");
  }

  //get readings once a second
  delay(1000);

}


//Displays "Start?" to user on LCD Screen 
void start_menu(void) {
  
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(30, 25);
  display.print("Start?");
  
  display.display();
  
}

The main .ino file requires you setup the arduino IDE correctly and download the proper libraries

Arduino IDE Settings:
  - board: ESP32S3 Dev Module
  - port: your local USB port (should show up as ESP32 Family Device)
  - USB CDC On Boot: Enabled
  - upload speed:115200
    
Open Library Manager:
  - Type SHT31
      - download adafruit SHT31 library 
  - Type VELM7700
      - download adafruit VEML7700 library 
  - Type SSD1306
      - download adafruit SSD1306 library

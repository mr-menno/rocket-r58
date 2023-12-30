/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x64 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define PIN_SWITCH    4
#define PIN_MODE      16
#define PIN_PUMP      17

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ 0b00000000, 0b11000000,
  0b00000001, 0b11000000,
  0b00000001, 0b11000000,
  0b00000011, 0b11100000,
  0b11110011, 0b11100000,
  0b11111110, 0b11111000,
  0b01111110, 0b11111111,
  0b00110011, 0b10011111,
  0b00011111, 0b11111100,
  0b00001101, 0b01110000,
  0b00011011, 0b10100000,
  0b00111111, 0b11100000,
  0b00111111, 0b11110000,
  0b01111100, 0b11110000,
  0b01110000, 0b01110000,
  0b00000000, 0b00110000 };

void displayCenterText(String text) {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;

  display.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);

  // display on horizontal and vertical center
  display.clearDisplay(); // clear display
  display.setCursor((SCREEN_WIDTH - width) / 2, (SCREEN_HEIGHT - height) / 2);
  display.println(text); // text to display
  display.display();
}

bool pumpOn = false;
int _countdown = 0;
int lastMillis = millis();

bool countdown(int interval) {
  if(millis() > (lastMillis + interval)) {
    _countdown--;
    lastMillis = millis();
    return true;
  }
  return false;
}

void displayCountdown() {
  display.clearDisplay();
  display.setTextSize(4); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  if(_countdown>30) {
    displayCenterText("PI: "+String(_countdown-30));
  } else {
    displayCenterText(String(_countdown));
  }
  display.display();
}


int lastPinMode=millis();
char mode='C';
int lastDebug=millis();
int autoReset=millis();

void setModeManual() {
  mode='M';
  autoReset=millis();
  Serial.println("switching to manual");
  display.setTextSize(2); // Draw 2X-scale text
  displayCenterText(F("MANUAL"));
  display.display();
}

void setModeAuto() {
  mode='C';
  _countdown=-36;
  Serial.println("switching to auto");
  display.setTextSize(2); // Draw 2X-scale text
  displayCenterText(F("AUTO"));
  display.display();
}

void debugStats() {
  if(lastDebug+1000<millis()) {
    lastDebug=millis();
    Serial.println("---");
    Serial.print(F("Mode: "));
    Serial.println(mode);
    Serial.print(F("Pin S: "));
    Serial.println(digitalRead(PIN_SWITCH));
    Serial.print(F("Pin M: "));
    Serial.println(digitalRead(PIN_MODE));
    Serial.print(F("Pump State: "));
    Serial.println(pumpOn);
    if(mode=='M') {
      if(digitalRead(PIN_SWITCH)) {
        Serial.println("Pump Off");
      } else {
        Serial.println("Pump On");
      }
    } else if(mode=='C' && _countdown>-30) {
      if(_countdown>0) {
        Serial.print("Countdown: ");
        Serial.println(_countdown);
      } else {
        Serial.print("Countdown: ");
        Serial.println("DONE");
      }
    }
  }
}

void runManual() {
  pumpOn = !digitalRead(PIN_SWITCH);
  display.clearDisplay();
  display.setTextSize(2);
  if(pumpOn) {
    digitalWrite(PIN_PUMP,1);
    displayCenterText("Manual On");
  } else {
    digitalWrite(PIN_PUMP,0);
    displayCenterText("Manual Off");
  }
  display.display();
}

bool countdownActive = false;
void runAuto() {
  if(countdownActive && _countdown>0) {
    if(countdown(1000)) {
      if(_countdown>30) {
        pumpOn=false;
        display.setTextSize(4);
        displayCenterText("PI: "+String(_countdown-30));
        display.display();
      } else {
        pumpOn=true;
        display.setTextSize(4);
        displayCenterText(String(_countdown));
        display.display();
      }
    }
  } else if(countdownActive && countdown(1000)) {
    pumpOn=false;
    display.setTextSize(2);
    displayCenterText("done");
    display.display();
  }
  if(!countdownActive) {
    pumpOn=false;
    display.setTextSize(2);
    displayCenterText("auto");
    display.display();
  }
  if(digitalRead(PIN_SWITCH)) {
    countdownActive=false;
    pumpOn=false;
  } else if(!digitalRead(PIN_SWITCH) && countdownActive==false) {
    pumpOn=false;
    _countdown=36;
    countdownActive=true;
  }
  if(pumpOn) {
    digitalWrite(PIN_PUMP,1);
  } else {
    digitalWrite(PIN_PUMP,0);
  }
}
void runAuto2() {
  if(!countdownActive && !digitalRead(PIN_SWITCH)) {
    countdownActive=true;
    _countdown=36;
  }
  if(_countdown>-30 && countdown(1000) && !digitalRead(PIN_SWITCH)) {
    if(_countdown > 0 && _countdown<=30) {
      // display.clearDisplay();
      displayCountdown();
    } else if(_countdown > -10) {
      displayCenterText("DONE");
      Serial.printf("Done at %d",_countdown);
    } else {
      Serial.println("clear display");
      display.clearDisplay();
      display.display();
    }
  }
  if(_countdown<=-30) {
    // countdownActive
  }
  //control pump
  if(_countdown>0 && _countdown <= 30) {
    digitalWrite(PIN_PUMP,1);
  } else {
    digitalWrite(PIN_PUMP,0);
  }
}

void setup() {
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  delay(2000);
  Serial.println("Setup: Clear Display()");
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  displayCenterText("Rocket R58");
  display.display();
  delay(1000);
  display.clearDisplay();
  pinMode(PIN_SWITCH, INPUT_PULLUP);
  pinMode(PIN_MODE, INPUT_PULLUP);
  pinMode(PIN_PUMP, OUTPUT);
  setModeAuto();
}


void loop() {
  debugStats();
  if(mode=='C') {
    runAuto();
  } else if(mode=='M') {
    runManual();
  }
  if(!digitalRead(PIN_MODE) && lastPinMode+500 < millis()) {
    lastPinMode=millis();
    if(mode=='C') {
      setModeManual();
    } else {
      setModeAuto();
    }
  }
}

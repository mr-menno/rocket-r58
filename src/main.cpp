#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "driver/rtc_io.h"
#include <WiFi.h>

#if !( defined(ESP8266) ||  defined(ESP32) )
  #error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

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

#define PIN_SWITCH    32
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
  // display.clearDisplay(); // clear display
  display.setCursor((SCREEN_WIDTH - width) / 2, (SCREEN_HEIGHT - height) / 2);
  display.println(text); // text to display
  // display.display();
}

//VARS - PUMP
int pumpMaxRun=45;
int pumpStopTime=0;
bool pumpState=false;

int wakeTimeout=120;
int suspendAfter = wakeTimeout*1000;
void wake() {
  suspendAfter=millis()+(wakeTimeout*1000);
}   
void suspend() {
  if(millis()>suspendAfter && millis()<(suspendAfter+100)) {
    // rtc_gpio_pullup_en(GPIO_NUM_32);
    // rtc_gpio_pulldown_dis(GPIO_NUM_32);
    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_32,0);
    display.clearDisplay();
    display.display();
    // rtc_gpio_pullup_en(GPIO_NUM_32);
    // esp_deep_sleep_start();
  }
}  
  

int _countdown = 0;
int lastMillis = millis();

bool firstSkip = true;
bool countdown(int interval) {
  if(millis()<5000 && firstSkip) {
    _countdown = _countdown - (millis()/interval);
    firstSkip=false;
  }
  if(millis() > (lastMillis + interval)) {
    _countdown--;
    lastMillis = millis();
    return true;
  }
  return false;
}

void displayCountdown() {
  //display.clearDisplay();
  display.setTextSize(4); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  if(_countdown>30) {
    display.setTextSize(3);
    displayCenterText("PI: "+String(_countdown-30));
  } else {
    displayCenterText(String(_countdown));
  }
  display.display();
}

char mode='C';
int lastDebug=millis();
int autoReset=millis();

void setModeManual() {
  mode='M';
  autoReset=millis();
  Serial.println("switching to manual");
  // display.setTextSize(2); // Draw 2X-scale text
  // displayCenterText(F("MANUAL"));
  // display.display();
}

void setModeAuto() {
  mode='C';
  _countdown=-36;
  Serial.println("switching to auto");
  // display.setTextSize(2); // Draw 2X-scale text
  // displayCenterText(F("AUTO"));
  // display.display();
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
    Serial.println(pumpState);
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

void runPump() {
  if(millis()>pumpStopTime) {
    digitalWrite(PIN_PUMP,0);
  } else if(pumpState) {
    digitalWrite(PIN_PUMP,1);
  } else {
    digitalWrite(PIN_PUMP,0);
  }
}
void pumpOn() {
  // wake();
  if(pumpState) {
    //NOOP
  } else {
    pumpState=true;
    pumpStopTime=millis()+(pumpMaxRun*1000);
  }
}
void pumpOff() {
  // wake();
  pumpState=false;
  pumpStopTime=0;
}    

int manualReset=60;
void runManual() {
  wake();
  if(millis()>(autoReset+(manualReset*1000))) {
    setModeAuto();
  } 
  wake();
  // display.clearDisplay();
  // display.setTextSize(2);
  
  if(!digitalRead(PIN_SWITCH)) {
    pumpOn();
    autoReset=millis();
    // displayCenterText("Manual On");
  } else {
    pumpOff();
    // displayCenterText("Manual Off");
  }
  // display.display();
}

bool countdownActive = false;

void runAuto() {
  if(countdownActive && _countdown>0) {
    wake();
    if(countdown(1000)) {
      if(_countdown>30) {
        pumpOff();
        // display.setTextSize(4);
        // displayCenterText("PI: "+String(_countdown-30));
        // display.display();
      } else {
        pumpOn();
        // display.setTextSize(4);
        // displayCenterText(String(_countdown));
        // display.display();
      }
    }
  } else if(countdownActive && countdown(1000)) {
    wake();
    pumpOff();
    // display.setTextSize(2);
    // displayCenterText("done");
    // display.display();
  }
  if(!countdownActive) {
    pumpOff();
    // display.setTextSize(2);
    if(millis()<suspendAfter) {
      // displayCenterText("auto");
      // display.display();
    }
  }
  if(digitalRead(PIN_SWITCH)) {
    countdownActive=false;
    pumpOff();
  } else if(!digitalRead(PIN_SWITCH) && countdownActive==false) {
    pumpOff();
    _countdown=36;
    countdownActive=true;
  }
}

int lastPinMode=millis();
void readModeButton() {
  if(!digitalRead(PIN_MODE) && lastPinMode+500 < millis()) {
    lastPinMode=millis();
    if(mode=='C') {
      setModeManual();
    } else {
      setModeAuto();
    }
  }
}

void displayOverlay() {
  if(millis()<suspendAfter) {
    //IP on Screen
    display.setTextSize(1);
    display.setCursor(0,25);
    display.print("IP: ");
    display.print(WiFi.localIP());
  }
}

void drawManual() {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.getTextBounds("Manual Mode", 0, 0, &x1, &y1, &width, &height);
  display.setCursor((SCREEN_WIDTH - width) / 2, 0);
  display.print("Manual Mode");
  display.drawLine(0,8,127,8,SSD1306_WHITE);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  String txt;
  if(pumpState) {
    txt="PUMP ON";
  } else {
    txt="PUMP OFF";
  }
  display.getTextBounds(String(txt), 0, 0, &x1, &y1, &width, &height);
  display.setCursor((SCREEN_WIDTH - width) / 2, 10);
  display.print(String(txt));
}
void drawCountdown() {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;

  if(!countdownActive) {
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.getTextBounds("Coffee Time!", 0, 0, &x1, &y1, &width, &height);
    display.setCursor((SCREEN_WIDTH - width) / 2, 10);
    display.print("Coffee Time!");
  } else if(_countdown>30) {
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.getTextBounds("Pre-Infusion", 0, 0, &x1, &y1, &width, &height);
    display.setCursor((SCREEN_WIDTH - width) / 2, 0);
    display.print("Pre-Infusion");
    display.drawLine(0,10,127,10,SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(3);
    display.getTextBounds(String(_countdown-30), 0, 0, &x1, &y1, &width, &height);
    display.setCursor((SCREEN_WIDTH - width) / 2, 12);
    display.print(String(_countdown-30));
  } else if(_countdown>0) {
    display.setTextSize(1);
    display.getTextBounds("Extraction", 0, 0, &x1, &y1, &width, &height);
    display.setCursor((SCREEN_WIDTH - width) / 2, 0);
    // display.setTextColor(SSD1306_BLACK,SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
    display.print("Extraction");
    display.drawLine(0,10,127,10,SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(3);
    display.getTextBounds(String(_countdown), 0, 0, &x1, &y1, &width, &height);
    display.setCursor((SCREEN_WIDTH - width) / 2, 12);
    display.print(String(_countdown));
  } else {
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(3);
    display.getTextBounds("DONE", 0, 0, &x1, &y1, &width, &height);
    display.setCursor((SCREEN_WIDTH - width) / 2, (SCREEN_HEIGHT - height) / 2);
    display.print("DONE");
  }
}

int lastScreenUpdate=0;
void drawScreen() {
  if(millis()<(lastScreenUpdate+200)) return;
  lastScreenUpdate=millis();
  display.clearDisplay();
  if(mode=='C') {
    drawCountdown();
  }
  if(mode=='M') {
    drawManual();
    // displayOverlay();
  }
  if(!countdownActive) {
    displayOverlay();
  }
  display.display();
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
  rtc_gpio_deinit(GPIO_NUM_32);
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
  runPump();
  readModeButton();
  suspend();
  drawScreen();
}

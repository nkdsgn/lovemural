#include <WDT.h>



#include <Arduino_BuiltIn.h>

#include "WiFiS3.h"
#include "FastLED.h"
#include <EEPROM.h>
//#include <avr/wdt.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SunRise.h>

#define NUM_LEDS_L 74
#define NUM_LEDS_O1 66
#define NUM_LEDS_O2 67
#define NUM_LEDS_V 114
#define NUM_LEDS_E 90
#define NUM_LEDS 411

#define LED_TYPE    WS2811
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

enum Mode { INDIVIDUAL, UNIFIED, WHOLE_LETTER_GRAD };
Mode currentMode = INDIVIDUAL;
unsigned long lastModeSwitch = 0;
const unsigned long modeInterval = 30000; // 30 sec
const unsigned long wholeLetterInterval = 90000; // 90 sec

DEFINE_GRADIENT_PALETTE(primary_gp) {
  0, 255, 0, 0,
  85, 0, 255, 0,
  170, 0, 0, 255,
  255, 255, 0, 0
};

DEFINE_GRADIENT_PALETTE(neon_gp) {
  0, 255, 20, 147,
  85, 57, 255, 20,
  170, 0, 255, 255,
  255, 255, 20, 147
};

CRGBPalette16 redWhiteBluePalette = CRGBPalette16(
  CRGB::Red,    CRGB::Red,    CRGB::White,  CRGB::White,
  CRGB::White,  CRGB::White,  CRGB::Blue,   CRGB::Blue,
  CRGB::Blue,   CRGB::Blue,   CRGB::White,  CRGB::White,
  CRGB::White,  CRGB::White,  CRGB::Red,    CRGB::Red
);

CRGBPalette16 primaryPalette = primary_gp;
CRGBPalette16 neonPalette = neon_gp;
CRGBPalette16 rainbowPalette = RainbowColors_p;

struct Letter {
  int start;
  int count;
  CRGBPalette16* palette;
  uint8_t hueOffset;
  uint8_t letterIdx;
};

Letter letters[] = {
  {0, NUM_LEDS_L, &primaryPalette, 0, 0},
  {NUM_LEDS_L, NUM_LEDS_O1, &neonPalette, 32, 1},
  {NUM_LEDS_L + NUM_LEDS_O1, NUM_LEDS_O2, &rainbowPalette, 64, 1},
  {NUM_LEDS_L + NUM_LEDS_O1 + NUM_LEDS_O2, NUM_LEDS_V, &primaryPalette, 96, 2},
  {NUM_LEDS_L + NUM_LEDS_O1 + NUM_LEDS_O2 + NUM_LEDS_V, NUM_LEDS_E, &neonPalette, 128, 3}
};

const char* ssid = "ASUS_Guest1";
const char* password = "levs12345";

WiFiUDP ntpUDP;
// NTPClient timeClient(ntpUDP, "pool.ntp.org", -14400); // EDT = UTC -4 hours

// Columbus, OH: latitude, longitude
//SunRise sunCalc(39.9612, -82.9988, -4);  // timezone offset for EDT
SunRise sunCalc;

bool muralOn = false;

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  FastLED.setBrightness(204);  // Set to 80% brightness
  WDT.begin(5000);
  pinMode(13, OUTPUT);
  Serial.begin(9600);

  //connectToWiFi();
  //timeClient.begin();
  //timeClient.update();

  FastLED.addLeds<LED_TYPE, 3, COLOR_ORDER>(leds, 0, NUM_LEDS_L).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 5, COLOR_ORDER>(leds, NUM_LEDS_L, NUM_LEDS_O1).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 7, COLOR_ORDER>(leds, NUM_LEDS_L + NUM_LEDS_O1, NUM_LEDS_O2).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 8, COLOR_ORDER>(leds, NUM_LEDS_L + NUM_LEDS_O1 + NUM_LEDS_O2, NUM_LEDS_V).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 11, COLOR_ORDER>(leds, NUM_LEDS_L + NUM_LEDS_O1 + NUM_LEDS_O2 + NUM_LEDS_V, NUM_LEDS_E).setCorrection(TypicalLEDStrip);

  FastLED.setBrightness(150);
  lastModeSwitch = millis();
}

void loop() {
  WDT.refresh();
  //timeClient.update();

  //int currentHour = timeClient.getHours();
  //int currentMinute = timeClient.getMinutes();
  //int totalMinutes = currentHour * 60 + currentMinute;

  //sunCalc.calculate(39.9612, -82.9988, millis() * 1000);
  //int const sunrise = sunCalc.riseTime;
  //int const sunset = sunCalc.setTime;

  // Turn ON at sunset, OFF 2 hours before sunrise
  //if (totalMinutes >= sunset || totalMinutes < (sunrise - 120)) {
  //  muralOn = true;
  //} else {
  //  muralOn = false;
 //}

  //if (!muralOn) {
  //  FastLED.clear(true);
  //  delay(60000); // Check again in 1 minute
  //  return;
  //}

  uint8_t t = millis() / 64;
  if (currentMode == INDIVIDUAL) {
    for (int i = 0; i < 5; i++) {
      Letter& l = letters[i];
      for (int j = 0; j < l.count; j++) {
        leds[l.start + j] = ColorFromPalette(*l.palette, t + l.hueOffset + (j * 255 / l.count));
      }
    }
  } else if (currentMode == UNIFIED){
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = ColorFromPalette(redWhiteBluePalette, t + (i * 255 / NUM_LEDS));
    }
  }
  else {
    for (int i = 0; i < 5; i++) {
      Letter& l = letters[i];
      auto const color = ColorFromPalette(redWhiteBluePalette, t + (l.letterIdx * 255 / 4));
      for (int j = 0; j < l.count; j++) {
        leds[l.start + j] = color;
      }
    }
  }

  FastLED.show();

  auto const elapsed = millis() - lastModeSwitch;
  if (elapsed > modeInterval) {
    if (currentMode == INDIVIDUAL) {
      currentMode = UNIFIED;
      lastModeSwitch = millis();
    }
    else if (currentMode == UNIFIED) {
      currentMode = WHOLE_LETTER_GRAD;
      lastModeSwitch = millis();
    }
    else {
      if (elapsed > wholeLetterInterval) {
        currentMode = WHOLE_LETTER_GRAD;
        lastModeSwitch = millis();
      }
    }
  }
  delay(20);
}
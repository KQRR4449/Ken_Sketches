#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN 2

Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(64);
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  snowflakes(100);
}

void snowflakes(uint8_t wait) {

  // Setup the pixel array
  int pixel[60];
  for (int p = 0; p < 60; p++) {
    pixel[p] = random(0, 255);
  }

  // Run some snowflake cycles
  for (int j = 0; j < 200; j++) {

    // Every five cycles, light a new pixel
    if ((j % 5) == 0) {
      strip.setPixelColor(random(0, 60), 255, 255, 255);
    }

    // Dim all pixels by 10
    for (int p = 0; p < 60; p++) {
      strip.setPixelColor(p, pixel[p], pixel[p], pixel[p] );
      pixel[p] =  pixel[p] - 10;
    }
    strip.show();
    delay(wait);
  }

}


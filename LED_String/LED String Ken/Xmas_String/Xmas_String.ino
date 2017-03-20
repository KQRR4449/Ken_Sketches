/*******************************************************************************************
 *
 * This program produces Christmas light patterns using a string of 60 WS2812
 * serial controlled RGB LEDs. Information on the LED string is below:
 *    Pro Mini Info:      https://www.sparkfun.com/products/11113
 *    LED String Info:    https://www.sparkfun.com/products/12025
 *    Christmas Lights:   http://bit.ly/2hnOw7v
 * 
 * It uses the Adafruit_NeoPixel library. Information is below:
 *    NeoPixel Overview:  https://learn.adafruit.com/adafruit-neopixel-uberguide/overview
 *    NeoPixel Library:   http://bit.ly/2dcL7Kl
 *    Header file:        ..\Arduino\libraries\Adafruit_NeoPixel\Adafruit_NeoPixel.h
 *
 *******************************************************************************************/

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif // __AVR__

#define UNUSED_ANIMATION                        // Currently unused animations.
#define DEBUG                                   // Define for serial debug output.
//#define PWR_TEST                                // Run power test as last animation.
//#define FREE_RAM                                // Report free ram.

//  Constants
const byte          VER_MAJOR       =     1;    // Major version.
const byte          VER_MINOR       =     1;    // Minor version.
const byte          INTERRUPT_PIN   =     2;    // Pushbutton pin.
const byte          PIXEL_PIN       =     3;    // LED string pin.
const byte          LED_PIN         =    13;    // Status LED pin.
const unsigned long DEBOUNCE_TIME   =    60UL;  // Debounce time in ms.
const byte          NUM_PIXELS      =    60;    // Number of LED strig pixels.
const byte          MAX_BRIGHTNESS  =   255;    // Max LED string brightness.
#ifndef PWR_TEST
const byte          ANIMATIONS      =    14;    // Animation number.
#else
const byte          ANIMATIONS      =    15;    // Animation number with powerTest().
#endif // PWR_TEST
const int8_t        RANDOM          =    -1;    // Animation number. -1 form RANDOM.
                                                // Through all animations.
const unsigned long ANI_TIME        = 20000UL;  // Minimum random cycle time.
const byte          CYLON_PIXELS                // Width of cylon lit pixels.
                                    = NUM_PIXELS / 6;
const uint32_t      DARK_COLOR      =           // All LEDs off color.
                                      Adafruit_NeoPixel::Color(0,0,0);
// Snowflake colors.
enum flakeColor {
  whiteFlake                        =     0,    // White snowflakes.
  blueFlake                         =     1     // Blue snowflakes.
};

// Snowflake brigtness algorithms.
enum flakeAlg {
  linear                            =     0,    // linear algorithm.
  parabolic                         =     1     // Parabolic algorithm.
};

//  Variables used to cycle through animations.
bool                oldState        = HIGH;     // Status LED state.
int8_t              animation       = RANDOM;   // Current animation number. -1 for RANDOM.
                                                // Random <= animation < ANIMATIONS.
byte                showType;                   // Current LED show type.
                                                //   0 <= showType < ANIMATIONS.
byte                lastType        = ANIMATIONS; // Set to invalid showType;
byte                rndType;                    // Old random showType.
unsigned long       aniEnd          =   0UL;    // End time of animation cycle.

#ifdef DEBUG
unsigned long       showStart;                  // Current show start time in ms.
unsigned long       showEnd;                    // Current show end time in ms.
#endif // DEBUG

//  Variables used by button interrupt handler.
volatile bool       btnPressed      = false;    // true if button pressed.
byte                btnState;                   // Button state.

//  Construct Adafruit_NeoPixel object to control the LED string.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  #if defined(DEBUG) || defined(PWR_TEST) || defined(FREE_RAM)
  Serial.begin(9600);
  Serial.print("Starting program version ");
  Serial.print(VER_MAJOR);
  Serial.print(".");
  Serial.println(VER_MINOR);
  #endif // DEBUG etal.
  
  pinMode(LED_PIN, OUTPUT);                                 // Status LED.
  randomSeed(analogRead(A0));                               // Init. random number from
                                                            //   noisy unused A0.

  // Set up LED string.
  strip.begin();
  strip.setBrightness(MAX_BRIGHTNESS/4);                    // Set brightness to 1/4 maximum.
  strip.show();                                             // Clear all pixels.

  // Set up INTERRUPT_PIN to handle the pushbutton.
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  // Read the button state before attachInterrupt() in case button is pressed at reset.
  btnState = digitalRead(INTERRUPT_PIN);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), buttonISR, CHANGE);
}

void loop() {
  if (btnPressed == true) {                                 // Button just pressed.
    btnPressed = false;                                     // Clear btnPressed to be ready
                                                            //   for the next press.

    animation++;                                            // Increment animation number.
    if (animation >= ANIMATIONS) {
      animation = RANDOM;
      aniEnd = 0UL;                                         // Force new showType if we
    }                                                       //   just entered random state.   
  }

  if (animation == RANDOM) {                                // Randomly cycle through all show types.
    if (millis() > aniEnd) {                                // Change animation after ANI_TIME.
      aniEnd = millis() + ANI_TIME;                         // aniEnd will properly roll over if
                                                            //   millis() rolls over.

      // Choose new random showType. Make sure not to repeat a showType.
      do {
        rndType = (byte)random(0, ANIMATIONS);
      } while (rndType == showType);
      showType = rndType;
      
      // Clear all the LEDs if the showType changes.
      clearLEDs();
      
      #ifdef DEBUG
      Serial.print("loop: New random showType ");
      Serial.println(showType);
      #endif // DEBUG
    }    
    #ifdef DEBUG
    else {
      Serial.print("loop: Keep random showType ");
      Serial.println(showType);
    }
    #endif // DEBUG
  }
  else {                                                    // Repeat single show type.
    showType = animation;
    
    if (lastType != showType)
    {
      lastType = showType;

      // Clear all the LEDs if the showType changes.
      clearLEDs();

      #ifdef DEBUG
      Serial.print("loop: New regular showType ");
      Serial.println(showType);
      #endif // DEBUG
    }
    #ifdef DEBUG
    else {
      Serial.print("loop: Keep regular showType ");
      Serial.println(showType);
    }
    #endif // DEBUG
  }

  toggle();                                                 // Toggle the status LED.

  #ifdef DEBUG
  showStart = millis();                                     // Time before show in ms. 
  Serial.print("loop: Starting animation = ");
  Serial.print(animation);
  Serial.print(", showType = ");
  Serial.println(showType);
  #endif // DEBUG
  
  startShow(showType);                                      // Run 1 cycle of the current showType.

  #ifdef DEBUG
  showEnd = millis();
  Serial.print("      Duration ");
  Serial.println(showEnd - showStart);
  #endif // DEBUG
}

void startShow(byte i) {
  switch (i) {
    #ifndef UNUSED_ANIMATION
    case 0:   colorWipe(strip.Color(255,   0,   0), 50);    // Red
      break;
    case 1:   colorWipe(strip.Color(  0, 255,   0), 50);    // Green
      break;
    case 2:   colorWipe(strip.Color(  0,   0, 255), 50);    // Blue
      break;
    #endif UNUSED_ANIMATION
    case  0:  cylon(strip.Color(255,  0,  0), 50);          // Red
      break;
    case  1:  cylon(strip.Color(  0,255,  0), 50);          // Red
      break;
    case  2:  cylon(strip.Color(  0,  0,255), 50);          // Red
      break;
    case  3:  cylonRainbow(50);
      break;
    case  4:  theaterChase(strip.Color(127, 127, 127), 50); // White
      break;
    case  5:  theaterChase(strip.Color(255,   0,   0), 50); // Red
      break;
    case  6:  theaterChase(strip.Color(  0, 255,   0), 50); // Green    
      break;
    case  7:  theaterChase(strip.Color(  0,   0, 255), 50); // Blue
      break;
    case  8:  rainbow(20);
      break;
    case  9:  rainbowCycle(20);
      break;
    case 10:  theaterChaseRainbow(50);
      break;
    case 11:  candyChase(100);
      break;
    case 12:  snowflakes(parabolic, whiteFlake, 100);       // Parabolic white.
      break;
    case 13:  snowflakes(parabolic, blueFlake,  100);       // Parabolic blue.
      break;
    #ifndef UNUSED_ANIMATION
    case 14:  snowflakes(linear,    whiteFlake, 100);       // Linear white.
      break;
    case 15:  snowflakes(linear,    blueFlake,  100);       // Linear blue.
      break;
    #endif // UNUSED ANIMATION
    #ifdef PWR_TEST                                         // Put powerTest last if used.
    case ANIMATION - 1:  powerTest();
      break;
    #endif // PWR_TEST
  }
}

#ifndef UNUSED_ANIMATION
// Fill the dots one after the other with a color.
void colorWipe(uint32_t c, uint8_t wait) {
  byte                      i;                              // Index variable.
  
  for (i = 0; i < NUM_PIXELS; i++) {
    strip.setPixelColor(i, c);
    strip.show();
    if (breakableDelay(wait) != true) return;
  }
      
  for (i = 0; i < NUM_PIXELS; i++) {
    strip.setPixelColor(i, DARK_COLOR);
    strip.show();
    if (breakableDelay(wait) != true) return;
  }
}
#endif // UNUSED_ANIMATION

// Run solid color cylon pattern.
void cylon(uint32_t c, uint8_t wait) {
  byte                      i, j;                           // Index variables.

  // Move cylon block to right.
  for (i = 0; i < NUM_PIXELS - CYLON_PIXELS + 1; i++) {
    for (j = 0; j < CYLON_PIXELS; j++ ) {
      strip.setPixelColor(i + j, c);      
    }
    
    strip.show();
    
    if (breakableDelay(wait) != true) return;

    // Clear left pixel to prepare for next cycle.
    strip.setPixelColor(i, DARK_COLOR);
  }
    
  // Move cylon block to left.
  for (i = NUM_PIXELS - CYLON_PIXELS; i < NUM_PIXELS - CYLON_PIXELS + 1; i--) {
    for (j = 0; j < CYLON_PIXELS; j++ ) {
      strip.setPixelColor(i + j, c);      
    }
    
    strip.show();
    
    if (breakableDelay(wait) != true) return;

    // Clear right pixel to prepare for next cycle.
    strip.setPixelColor(i + CYLON_PIXELS - 1, DARK_COLOR);
  }  
}

// Run rainbow color cylon pattern.
void cylonRainbow(uint8_t wait) {
  byte                      i, j;                           // Index variables.
  uint16_t                  wheelNum = 0;                   // Wheel number.

  for (wheelNum = 0; wheelNum < 256; ) {
    // Move cylon block to right.
    for (i = 0; i < NUM_PIXELS - CYLON_PIXELS + 1; i++) {
     for (j = 0; j < CYLON_PIXELS; j++ ) {
        strip.setPixelColor(i + j, Wheel(wheelNum++ & 255));      
      }
    
      strip.show();
    
      if (breakableDelay(wait) != true) return;

      // Clear left pixel to prepare for next cycle.
      strip.setPixelColor(i, DARK_COLOR);
    }
    
    // Move cylon block to left.
    for (i = NUM_PIXELS - CYLON_PIXELS; i < NUM_PIXELS - CYLON_PIXELS + 1; i--) {
      for (j = 0; j < CYLON_PIXELS; j++ ) {
        strip.setPixelColor(i + j, Wheel(wheelNum++ & 255));      
      }
    
      strip.show();
    
      if (breakableDelay(wait) != true) return;

      // Clear right pixel to prepare for next cycle.
      strip.setPixelColor(i + CYLON_PIXELS - 1, DARK_COLOR);
    }
  }
}

// Display a rainbow on the LED string.
void rainbow(uint8_t wait) {
  uint16_t                  i, j;                           // Index variables.
  
  for (j = 0; j < 256; j++) {
    for (i = 0; i < NUM_PIXELS; i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    if (breakableDelay(wait) != true) return;
  }
}

// Slightly different, this makes the rainbow equally distributed throughout.
void rainbowCycle(uint8_t wait) {
  uint16_t            i, j;                                 // Index variables.

  for (j = 0; j < 256 * 5; j++) {                           // 5 cycles of all colors
    for (i = 0; i < NUM_PIXELS; i++) {                      //   on wheel.
      strip.setPixelColor(i, Wheel(((i * 256 / NUM_PIXELS) + j) & 255));
    }
    strip.show();
    if (breakableDelay(wait) != true) return;
  }
}

// Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  byte                      i, j, q;                        // Index variables.
  
  for (j = 0; j < 10; j++) { //do 10 cycles of chasing
    for (q = 0; q < 3; q++) {
      for (i = 0; i < NUM_PIXELS; i = i + 3) {
        strip.setPixelColor(i + q, c);  //turn every third pixel on
      }
      strip.show();

      breakableDelay(wait);

      for (int i = 0; i < NUM_PIXELS; i = i + 3) {
        strip.setPixelColor(i + q, 0);                      //turn every 3rd pixel off.
      }
    }
  }
}

// Theatre-style crawling lights with rainbow effect.
void theaterChaseRainbow(uint8_t wait) {
  uint16_t                  i, j, q;                        // Index variables.
  
  for (j = 0; j < 256; j++) {                               // cycle through all 256 colors
    for (q = 0; q < 3; q++) {                               //   on the wheel.
      for (i = 0; i < NUM_PIXELS; i = i + 3) {
        strip.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
      }
      
      strip.show();

      if (breakableDelay(wait) != true) return;

      for (int i = 0; i < NUM_PIXELS; i = i + 3) {
        strip.setPixelColor(i + q, 0);                      //turn every third pixel off
      }
    }
  }
}

// Show moving candy cane.
void candyChase(uint8_t wait) {
  uint16_t                  i, j, q;                        // Index variables.

  for (j = 0; j < 10; j++) {                                // Do 10 cycles of chasing
    for (q = 0; q < 3; q++) {
      for (i = 0; i < NUM_PIXELS; i++) {
        strip.setPixelColor(i + q, 255, 255, 255);          // Turn every pixel white
      }
      
      for (i = 0; i < NUM_PIXELS; i = i + 3) {
        strip.setPixelColor(i + q, 255, 0, 0);              // Turn every third pixel red
      }
      
      strip.show();

      if (breakableDelay(wait) != true) return;

      for (i = 0; i < NUM_PIXELS; i = i + 3) {
        strip.setPixelColor(i + q, 0UL);                    // Turn every third pixel off.
      }
    }
  }
}

// Show random snowflakes.
void snowflakes(flakeAlg alg, flakeColor flake, uint8_t wait) {
  register unsigned long    brightness;                     // Brightness value.
  register uint16_t         p;                              // Pixel index.
  unsigned long             i;                              // Index variable.
  uint8_t                   pixClr;                         // Pixel color.
  #ifdef DEBUG
  unsigned long         minB          = ~0UL;               // Minimum brightness.
  unsigned long         maxB          =  0UL;               // Maximum brightness.
  #endif // DEBUG

  // Run some snowflake cycles
  for (i = 0; i < 200; i++) {
    for (p = 0; p < NUM_PIXELS; p++) {
      // Calciate brighness level using selected algorithm.
      brightness = random(0, 256);
      switch (alg) {
        case linear:                                        // Linear.
          pixClr = (uint8_t)brightness;
          break;
        case parabolic:                                     // Parabolic.
          pixClr = (uint8_t)((brightness * brightness) / 255);
          break; 
      }

      #ifdef DEBUG
      if (brightness < minB) {
        minB = brightness;
      }
      if (brightness > maxB) {
        maxB = brightness;
      }
      #endif // DEBUG

      // Set selected pixel color using calculated brighness level.
      switch (flake) {
        case whiteFlake:
          strip.setPixelColor(p, pixClr, pixClr, pixClr);   // White flakes.
          break;
        case blueFlake:
          strip.setPixelColor(p,      0,      0, pixClr);   // Blue flakes.
          break;
      }
    }
   
    strip.show();
    
    if (breakableDelay(wait) != true) return;
  }

  #ifdef DEBUG
  Serial.print("snowflakes: minimum brightness ");
  Serial.print(minB);
  Serial.print(", maximum brighness ");
  Serial.println(maxB);
  #endif // DEBUG
}

#ifdef PWR_TEST
// Test LED power.
void powerTest(void) {
  byte                      i;                              // Index variable.
  byte                      numLit;                         // Number of LEDs to light.
 
  // Rapidly blink the status LED to identify we are starting this method.
  //   Toggle even number of times to leave LED in the same state.
  for (i = 0; i < 50; i++) {
    toggle();
    
    if (breakableDelay(100UL) != true) return;
  }

  // Turn on LEDs 1/4 at a time.
  for ( numLit = 0; numLit < NUM_PIXELS + 1; numLit += NUM_PIXELS / 4) {
    for (i = 0; i < numLit; i++ ) {                         // Set i number of lights
      strip.setPixelColor(i, 255, 255, 255);                //   to full white.     
    }

    #ifdef DEBUG
    Serial.print("powerTest: LEDs lit ");
    Serial.println(i);
    #endif

    strip.show();

    if (breakableDelay(15000UL) != true) return;            // Wait for meter to settle.
  }
}
#endif // PWR_TEST

// Input a value 0 to 255 to get a color value.
//   The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  
  if (WheelPos < 85) {                                      // Violet.
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else if (WheelPos < 170) {                                // Cyan.
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  else {                                                    // Yellow.
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

// Toggle ledState and display it on LED_PIN
void toggle(void) {
  static byte ledState  = LOW;                              // LED state LOW or HIGH;
    
  digitalWrite(LED_PIN, ledState = !ledState);
}

// Clear and show LEDs.
void clearLEDs(void) {
  strip.clear();
  strip.show();

  #ifdef DEBUG
  Serial.println("clearLEDs: Clearing all LEDs");
  #endif
}
/*******************************************************************************************
 ********************************* Interrupt handling methods ******************************
 *******************************************************************************************/

/*******************************************************************************************
 * 
 *
 * breakableDelay() - Delays for the milliseconds passed in delay.
 *
 * breakableDelay() will return immediately if btnPressed is true.
 *
 * Inputs
 *  unsigned long delay  - Delay in milliseconds.
 *
 * Returns:
 *  true  - Delay completed.
 *  false - Key pressed.
 *  
 *******************************************************************************************/

bool breakableDelay( unsigned long delay) {
  unsigned long             startTime = millis();           // Start time in ms.

  while (millis() - startTime < delay) {
    if (btnPressed == true) {                               // Return immediately with false
      return false;                                         //   if button pressed.
    }
  }
  
  return true;                                              // Return true if delay finished.
}

/*******************************************************************************************
 * 
 * buttonISR() - Handle interrupt when button is pressed or released.
 *
 * buttonISR() toggles btnState if buttonISR() is entered after the
 * DEBOUNCE_TIME. Calls to buttonISR() before DEBOUNCE_TIME are ignored.
 * It is important that btnState be set to the current state of the
 * button prior to registering the interrupt or the button may work out
 * of phase.
 *
 * Inputs
 *  None.
 *
 * Returns:
 *  None.
 *  
 *******************************************************************************************/

void buttonISR() {
  unsigned long             currentTime;                    // Current time in ms.
  static unsigned long      endTime = 0UL;                  // End debounce time in ms.

  currentTime = millis();
  
  if (endTime < currentTime) {                              // Ignore interrupts caused
                                                            //   by switch bounce.

    endTime = currentTime + DEBOUNCE_TIME;                  // Set lastInterrupt to be ready
                                                            //   for next press or release.

    btnState = !btnState;                                   // Toggle bntState on first
                                                            //   button down or up interrupt.
 
    if (btnState == LOW) {                                  // Set btnState if button
      btnPressed = true;                                    //   just pressed.
    }
  }

  #ifdef FREE_RAM
  Serial.print("buttonISR: free ram ");
  Serial.println(freeRam());
  #endif
}

/*******************************************************************************************
 * 
 * freeRam() - Return the remain free RAM shared by the heap and stack.
 *
 * Inputs
 *  None.
 *
 * Returns:
 *  int _free - The remaining free RAM.
 *  
 *******************************************************************************************/

int freeRam()
{
  extern int                __heap_start, *__brkval;        // Heap data. 
  int v;                                                    // &v is bottom of stack. 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

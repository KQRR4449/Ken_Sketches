// This is optional, but set to false to disable
#define LED_DEBUG true

//#include <LedDebug.h>

#ifndef __LED_DEBUG__
#define __LED_DEBUG__

#if (!defined(LED_DEBUG) || LED_DEBUG)

#ifndef LED_DEBUG_PIN
#define LED_DEBUG_PIN     LED_BUILTIN
#endif

#ifndef LED_DEBUG_DELAY
#define LED_DEBUG_DELAY   50
#endif

#ifndef LED_DEBUG_LENGHT
#define LED_DEBUG_LENGHT  125
#endif

#define __PULSE_LNG(NUM, LENGHT)  Serial.printf("__PULSE_LNG(%d, %d)\n",NUM,LENGHT);\
  for(int __led_debug_count = 0; __led_debug_count < NUM; __led_debug_count++) {\
    digitalWrite(LED_DEBUG_PIN, HIGH);\
    delay(LENGHT);\
    digitalWrite(LED_DEBUG_PIN, LOW);\
    delay(LENGHT);\
  }\
  delay(LED_DEBUG_DELAY)

#define __PULSE_DEF(NUM)          Serial.printf("__PULSE_DEF(%d)\n",NUM);\
                                  __PULSE_LNG(NUM, LED_DEBUG_LENGHT)

#define __PULSE_ONE()             Serial.println("__PULSE_ONE()");\
                                  digitalWrite(LED_DEBUG_PIN, HIGH);\
  delay(LED_DEBUG_LENGHT);\
  digitalWrite(LED_DEBUG_PIN, LOW);\
  delay(LED_DEBUG_LENGHT);\
  delay(LED_DEBUG_DELAY)

#define __PULSE_X(x,NUM,LENGHT,MACRO, ...) pinMode(LED_DEBUG_PIN, OUTPUT);MACRO

#define PULSE(...)                __PULSE_X(,##__VA_ARGS__,\
    __PULSE_LNG(__VA_ARGS__),\
    __PULSE_DEF(__VA_ARGS__),\
    __PULSE_ONE(__VA_ARGS__)\
                                           )
#else
#define PULSE(...)
#endif
#endif // __LED_DEBUG__

// Set this to override the default value
#define LED_DEBUG_PIN 13

void setup() {
  // No setup required
  Serial.begin(9600);
  Serial.println(F("Starting LedDebug_Ken."));
}

void loop() {
  int val = analogRead(A0);

  Serial.println("Before PULSE()");
  // Emits one single pulse
  PULSE();
  Serial.println("After PULSE()");

  delay(2000);

  Serial.println("Before PULSE(5)");
  // Emits multiple pulses
  PULSE(5);
  Serial.println("After PULSE(5)");

  delay(2000);

  Serial.println("Before PULSE(3, 250)");
  // Emits multiple pulses of custom duration
  PULSE(3, 250);
  Serial.println("After PULSE(3, 250)");

  delay(2000);
}

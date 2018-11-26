#include <DHT_U.h>

// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

#include "DHT.h"

#ifdef AVR
const char          *CPU_TYPE   =      "AVR"; // CPU type is AVR.
#elif ESP8266
const char          *CPU_TYPE   =  "ESP8266"; // CPU type is ESP8266.
#elif ESP32
const char          *CPU_TYPE   =    "ESP32"; // CPU type is ESP32.
#endif // CPU_TYPE

//const uint8_t       DHTPIN      =         21; // Huzzah32 pin used.
const uint8_t       DHTPIN      =          4; // Huzzah ESP8266 pin used,
//const uint8_t       DHTPIN      =          2; // Arduino UNO pin used.

// Uncomment whatever type you're using!
//const uint8_t       DHTTYPE     =      DHT11; // DHT 11
//const uint8_t       DHTTYPE     =      DHT21; // DHT 21 (AM2301)
const uint8_t       DHTTYPE     =      DHT22; // DHT 22  (AM2302), AM2321

//const unsigned long DISP_TIME   =    10000UL; // Time between data display in ms.
const unsigned long DISP_TIME   =    60000UL; // Time between display in ms.

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  #ifdef ESP8266
  delay(100);
  #endif // ESP8266
  
  Serial.println("DHTxx test!");

  Serial.print("CPU_TYPE = ");
  Serial.println(CPU_TYPE);
  Serial.print("DHTTYPE  = ");
  Serial.println(DHTTYPE);
  Serial.print("DHTPin   = ");
  Serial.println(DHTPIN);

  dht.begin();
}

void loop() {
  static unsigned long errorCnt     =                      0UL; // Error count.
  static unsigned long firstMillis  =                 millis(); // First loop time in ms.
  static unsigned long nextDisplay  =                      0UL;
  
  // Wait a few seconds between measurements.
  //delay(20000);
  delay(2000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  //Serial.printf("%5u ", ++passes);
  //Serial.printf("%7lu - ", (millis() - firstMillis) / 1000);
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    //Serial.print("Errors: ");
    //Serial.print(++errorCnt);
    //Serial.println(" Failed to read from DHT sensor!");
    Serial.printf("%-7s %7lu - Errors %3lu Failed to read from DHT sensor!\n",
                                                  CPU_TYPE,
                                                  (millis() - firstMillis) / 1000,
                                                  ++errorCnt);
    return;
  }

  // Display results only if dispFlag == true.
if (millis() > nextDisplay) {
    nextDisplay = millis() + DISP_TIME;
  }
  else {
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.printf("%-7s %7lu - Errors %3lu ", CPU_TYPE, (millis() - firstMillis) / 1000, errorCnt);
  Serial.print(" Humid ");
  Serial.print(h);
  Serial.print("% ");
  Serial.print("Temp ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F ");
  Serial.print("Heat ind ");
  Serial.print(hic);
  Serial.print(" *C ");
  Serial.print(hif);
  Serial.println(" *F");

}

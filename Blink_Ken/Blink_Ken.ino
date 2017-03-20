/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */

#define DEBUG                                         // Define for serial debug.

// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int led = 13;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);

  #ifdef DEBUG
  Serial.begin(115200);
  Serial.println("Starting program.");
  #endif // DEBUG
}

// the loop routine runs over and over again forever:
void loop() {
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  #ifdef DEBUG
  Serial.println("Write HIGH");
  #endif // DEBUG
  delay(100);                // wait for 0.1 second
  
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  #ifdef DEBUG
  Serial.println("Write LOW");
  #endif
  delay(1000);               // wait for a second
}


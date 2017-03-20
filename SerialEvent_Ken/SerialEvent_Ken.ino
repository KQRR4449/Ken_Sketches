/*
  Serial Event example

 When new serial data arrives, this sketch adds it to a String.
 When a newline is received, the loop prints the string and
 clears it.

 A good test for this is to try it with a GPS receiver
 that sends out NMEA 0183 sentences.

 Created 9 May 2011
 by Tom Igoe

 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/SerialEvent

 */
// Column locations.
//3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
//       1         2         3         4         5         6         7         8         9     9
//const unsigned long FAKE_LONG     =          10000UL; // Fake variable for column positions.

String                inputString     =             ""; // a string to hold incoming data
bool                  stringComplete  =         false;  // whether the string is complete

const byte            VER_MAJOR     =                1; // Major version.
const byte            VER_MINOR     =                1; // Minor version.

void setup() {
  // initialize serial:
  Serial.begin(9600);
  Serial.printf(F("Starting program version %d.%d, Build date %s %s\n"),  VER_MAJOR,
                                                                          VER_MINOR,
                                                                          __DATE__,
                                                                          __TIME__);
  // reserve 200 bytes for the inputString:
  inputString.reserve(200);
}

void loop() {
  // print the string when a newline arrives:
  if (stringComplete) {
    Serial.print(inputString);
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}



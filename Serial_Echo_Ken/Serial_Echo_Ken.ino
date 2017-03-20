/*
 * Echo characters from serial port.
 */
// Column locations.
//3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
//       1         2         3         4         5         6         7         8         9     9
//const unsigned long FAKE_LONG     =          10000UL; // Fake variable for column positions.
#include <PString.h>

const byte            VER_MAJOR     =                1; // Major version.
const byte            VER_MINOR     =                1; // Minor version.

char                  InBuf[40];                        // Buffer managed by InStr.
PString               InStr(InBuf, sizeof(InBuf));      // Input PString.
bool                  StrComplete   =            false; // whether the string is complete

void setup() {
  // initialize serial:
  Serial.begin(9600);
  Serial.printf(F("Starting Serial_Echo_Ken version %d.%d, Build date %s %s\n"),
                                                                          VER_MAJOR,
                                                                          VER_MINOR,
                                                                          __DATE__,
                                                                          __TIME__);
}

void loop() {
  // print the string when a newline arrives:
  if (StrComplete) {
    Serial.printf(F("String \"%s\"\n"), InBuf);
    // clear the string:
    InStr = "";
    StrComplete = false;
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
    // Get the new byte:
    char inChar = (char)Serial.read();
    // Echo the character.
    Serial.print(inChar);
    // If the incoming character is a newline, set a flag
    // so the main loop can do something about it.
    if (inChar == '\n') {
      Serial.printf(F("\\n char 0x%02x received.\n"), inChar);
      StrComplete = true;
      return;
    }
    if (inChar == '\r') {
      Serial.printf(F("\\r char 0x%02x received.\n"), inChar);
      StrComplete = true;
      return;
    }
    // Add it to the InStr:
    InStr += inChar;
  }
}



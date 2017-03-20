// Column locations.
//3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
//       1         2         3         4         5         6         7         8         9     9
//const unsigned long FAKE_LONG     =          10000UL; // Fake variable for column positions.

// Temp file for quick tests. Clean up when you are done.

#include <Streaming.h>
#include <StreamingW.h>

const byte            VER_MAJOR     =                1; // Major version.
const byte            VER_MINOR     =                1; // Minor version.

// Put constants and global variables here:

void setup() {
  // put your setup code here, to run once:
  // initialize serial communications:
  Serial.begin(115200);
  Serial.printf(F("Starting Temp version %d.%d, Build date %s %s\n"),     VER_MAJOR,
                                                                          VER_MINOR,
                                                                          __DATE__,
                                                                          __TIME__);
  Serial.printf(F("File %s, __LINE__ %d\n"), __FILE__, __LINE__);

}

void loop() {
  // put your main code here, to run repeatedly:
}


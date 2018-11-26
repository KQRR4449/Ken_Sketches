/*
  SD card read/write

 This example shows how to read and write data to and from an SD card file
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK  - pin 13
 ** CS   - pin  4 (for MKRZero SD: SDCARD_SS_PIN)
 **        pin 10 (for AdaFruit Data Logger)

 created   Nov 2010
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe

 This example code is in the public domain.

 */

#include <SPI.h>
#include <SD.h>

const byte  CS_PIN        = 10;                 // Chip select pin.                                                                                                                                                                                                              // Chip select pin.
const char  *SET_FILE    = "set.txt";          // Test file name.

File        sFile;                              // Test File.

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Serial.print("Initializing SD card... ");

  if (!SD.begin(CS_PIN)) {
    Serial.println("SD initialization failed!");
    return;
  }
  Serial.println("SD initialization done.");

  Serial.printf(F("File %s exists %s\n"), SET_FILE, 
                                          SD.exists(SET_FILE) ? "true" : "false");
  
  // Open the file for reading:
  sFile = SD.open(SET_FILE);
  if (sFile) {
    Serial.printf(F("%s before read: position %lu, size %lu.\n"), SET_FILE,
                                                                  sFile.position(),
                                                                  sFile.size());
    Serial.printf("%s:\n", SET_FILE);

    // read from the file until there's nothing else in it:
    while (sFile.available()) {
      Serial.write(sFile.read());
    }
    Serial.printf(F("%s after read:  position %lu\n"),            SET_FILE,
                                                                  sFile.position());
    // close the file:
    sFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.printf("Error opening %s for read.", SET_FILE);
  }
}

void loop() {
  // nothing happens after setup
}



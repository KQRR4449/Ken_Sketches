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
const char  *TEST_FILE    = "test.txt";         // Test file name.

File        myFile;                             // Test File.

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Serial.print("Initializing SD card... ");

  if (!SD.begin(CS_PIN)) {
    Serial.println("SD initialization failed!");
    return;
  }
  Serial.println("SD initialization done.");

  Serial.printf(F("sizeof myFile %u\n"), sizeof(myFile));

  // Delete file.
  SD.remove(TEST_FILE);
  
  Serial.printf(F("File %s exists %s\n"), TEST_FILE, 
                                          SD.exists(TEST_FILE) ? "true" : "false");

  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open(TEST_FILE, FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.printf(F("%s before write: position %lu, size %lu.\n"),  TEST_FILE,
                                                                    myFile.position(),
                                                                    myFile.size());
    Serial.printf(F("Writing to %s...\n"), TEST_FILE);
    myFile.println(("testing 1, 2, 3."));
    Serial.printf(F("%s after write:  position %lu\n"),             TEST_FILE,
                                                                    myFile.position());
    // close the file:
    myFile.close();
    Serial.println("done writing.");
  } else {
    // if the file didn't open, print an error:
    Serial.printf(F("Error opening %s for write."), TEST_FILE);
  }

  // re-open the file for reading:
  myFile = SD.open(TEST_FILE);
  if (myFile) {
    Serial.printf(F("%s before read: position %lu, size %lu.\n"), TEST_FILE,
                                                                  myFile.position(),
                                                                  myFile.size());
    Serial.printf("%s:\n", TEST_FILE);

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    Serial.printf(F("%s after read:  position %lu\n"),            TEST_FILE,
                                                                  myFile.position());
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.printf("Error opening %s for read.", TEST_FILE);
  }
}

void loop() {
  // nothing happens after setup
}



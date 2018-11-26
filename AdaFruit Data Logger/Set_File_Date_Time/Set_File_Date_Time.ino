#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>


const uint8_t SD_CS = 10; // SD chip select
const char    *TEST_FILE    = "TEST_DT.TXT";
RTC_PCF8523 RTC;  // define the Real Time Clock object

File file;  // test file
char timestamp[30];

//------------------------------------------------------------------------------
// call back for file timestamps
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = RTC.now();
  sprintf(timestamp, "%02d:%02d:%02d %2d/%2d/%2d \n", now.hour(),now.minute(),now.second(),now.month(),now.day(),now.year()-2000);
  Serial.print("In dateTime callback ");
  Serial.println(timestamp);
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}

//------------------------------------------------------------------------------
  void setup() {
 
  Serial.begin(115200);
  Wire.begin();
  if (!RTC.begin()) {
    Serial.println("RTC failed");
    while(1);
  };
  // set date time callback function
  SdFile::dateTimeCallback(dateTime);
 
  DateTime now = RTC.now();
  sprintf(timestamp, "%02d:%02d:%02d %2d/%2d/%2d \n",
                now.hour(),now.minute(),now.second(),now.month(),now.day(),now.year()-2000);
  Serial.print("In setup ");
  Serial.println(timestamp);
 
  if (!SD.begin(SD_CS)) {
    Serial.println("SD.begin failed");
    while(1);
  }
  file = SD.open(TEST_FILE, FILE_WRITE);
  file.println("Testing 1,2,3...");
  Serial.println(F("After writing to file."));

  delay(5000);
  file.flush();
  Serial.println(F("After flush."));
 
  delay(5000);
  file.close();
  Serial.println(F("After close."));
  
  Serial.println("Done");
}

//------------------------------------------------------------------------------
void loop() {}

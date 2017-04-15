/***********************************************************************************************
 *
 * This sketch tests the Adafruit Datalogger PCF8523 real time clock.
 * The connections are as follows:
 * 
 * Pin  Name        Mode          Description
 * ----------------------------------------------------------------------------------
 * D3   INT_PIN     INPUT_PULLUP  1 second square wave. Connected to PCF8523 SQW pin.
 * 
 ***********************************************************************************************
  */
// Column locations.
//3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
//       1         2         3         4         5         6         7         8         9     9
//const unsigned long FAKE_LONG     =          10000UL; // Fake variable for column positions.

#include <limits.h>                             // Min and max values of int etc.
#include <RTClibKGW.h>
#include <PString.h>
#include <StreamingW.h>
#include <SD.h>

//#define DEBUG
#define FREE_RAM_FUNC

#define BIT_NUMBER(n)   (1U << (n))

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
   #define Serial SerialUSB
#endif

const byte            VER_MAJOR     =                1; // Major version.
const byte            VER_MINOR     =                2; // Minor version.
const byte            INT_PIN       =                3; // 1 second interrupt input pin.
const byte            INT_LEVEL     =             HIGH; // INT_PIN level to count.
const byte            INT_MODE      =           RISING; // Interrupt mode.
const size_t          IN_SIZE       =               32; // Size of input char buffer.
const byte            CS_PIN        =               10; // Chip select pin.
const char            *SET_FILE     = "set.txt";        // Time set file name.
// OFF_FACTOR[] muliplies Delta before division by Baseline.
// PCF8523.pdf RTC manual section 8.8 page 28.
const long            OFF_FACTOR[2] = {230414L,         // Offset factor 0.
                                       245761L};        // Offset factor 1.
const char *WEEK_DAYS[7] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                            "Thursday", "Friday", "Saturday"};

// TIME_0 is the 1st valid time that DateTime can handle as a UNIX time.
// Call DateTime::DateTime(INT_PIN) to set it to the earliest valid value.
const uint32_t        TIME_0        = SECONDS_FROM_1970_TO_2000;

enum RtcCmd {
  RTC_ADJUST                        =             'a',  // Adjust RTC time.
  RTC_OFFSET                        =             'o',  // Set RTC offset.
  RTC_RESET                         =             'r',  // Reset RTC.
};

// Forward references.
void        printTime(unsigned long secCnt, const DateTime &t);
uint16_t    parseTimeStr(const char *tBuf);
uint16_t    parseOffsetStr(const char *tBuf);
bool        readSetTime(void);
bool        writeSetTime(void);
signed char offsetAdjust(void);
void        setFileTime(uint16_t* date, uint16_t* time);
void        dashedLine(byte cnt, char c = '-');

long                  SecondCnt     =              0UL; // Count of 1 second interrupts.
bool                  RtcInitialized    =        false; // Set true if RTC just initialized.
volatile bool         IntFlag       =            false; // Set true if interrupt occurs.
RTC_PCF8523_KGW       Rtc;                              // PCF8523 real time clock object.
bool                  RtcRst        =           false;  // RTC was reset.
DateTime              SetTime;                          // Time clock was started.
bool                  SetTimeValid  =            false; // Set time read from SD card.
char                  InBuf[IN_SIZE];                   // Buffer managed by PString.
PString               InStr(InBuf, sizeof(InBuf));      // Pstring to hold incoming data.
bool                  StrComplete   =           false;  // Whether the string is complete.
unsigned long         SetStart      =             0UL;  // Time 1st set character arrived.
bool                  SdOK          =           false;  // SD card plugged in.
long                  Baseline      =              0L;  // Setting Baseline.
long                  Delta         =              0L;  // Setting Delta.
Offset_t              Off_t;                            // RTC offset.
TimeSpan              MinBase;                          // Minimum baseline time for 1 second
                                                        // change to be relevent.
#ifdef DEBUG
char                  TimeStamp[30];                    // File timestamp buffer.
#endif // DEBUG

void setup () {
#ifndef ESP8266
  while (!Serial); // for Leonardo/Micro/Zero
#endif

  Serial.begin(9600);
  
  Serial.printf(F("Starting Set_RTC version %d.%d, Build date %s %s\n"),
                                                              VER_MAJOR,
                                                              VER_MINOR,
                                                              __DATE__,
                                                              __TIME__);
  if (! Rtc.begin()) {
    Serial.println("    Couldn't find RTC");
    while (1);
  }
  
  if (SD.begin(CS_PIN) == true) {
    Serial.println("    SD initialization done.");
    SdOK = true;
  }
  else {
    Serial.println(">>> SD initialization failed!");
    SdOK = false;    
  }

  // Read SetTime from SET_FILE. Do this here so the RTC can be initialized
  // using this time if it is not already initialized.
  readSetTime();
  
  if (!Rtc.initialized()) {
    Serial.println(F(">>> RTC is NOT running! Initializing RTC"));
    
    // Set new RTC time. This will start the RTC if it is stopped.  
    Rtc.adjust(SetTime);
    DateTime t = Rtc.now();
    Serial.printf(F("    RTC initialized to Rtc.secondstime %12ld\n"), t.secondstime());
    
    // Turn on the 1 Hz square wave output. This is used to trigger the
    // interrupt service routine attached to the INT_PIN input pin.
    Rtc.writeSqwPinMode(PCF8523_SquareWave1HZ);

    // Set RtcInitialized true to indicate RTC just initialized.
    RtcInitialized = true;
  }
  
  Off_t = Rtc.readOffset();
  Serial.printf(F("    RTC offset %3d, mode %d.\n"), Off_t.getOffset(), Off_t.getMode());

  Serial.printf(F("    SetTime %s read from \"%s\".\n"),
                                                    SetTimeValid ? "successfully" : "NOT",
                                                    SET_FILE);
  Serial.printf(F("    SetTime %2u/%2u/%4u %2u:%02u:%02u.\n"),  SetTime.month(),
                                                                SetTime.day(),
                                                                SetTime.year(),
                                                                SetTime.hour(),
                                                                SetTime.minute(),
                                                                SetTime.second());
  Serial.printf(F("    Baseline %12ld, Delta %12ld, Suggested adjustment %3d.\n"),
                                        Baseline, Delta, offsetAdjust());
  /* 
   *  Min baseline for a 1 second delta to result in a 1 count offset is:
   *  for adj = OFF_FACTOR[mode] * delta / baseline.
   *  base_min = OFF_FACTOR[mode] / delta or OFF_FACTOR[mode] / 1 for delta_min = 1;
   */ 
  MinBase = OFF_FACTOR[Off_t.getMode()];
  Serial.printf(F("    Baseline minimum for 1 second Delta RTC offset mode %d:\n"),
                                                                  Off_t.getMode());
  Serial.printf(F("        %ld seconds or %d days %2d:%02d:%02d h:m:s.\n"),
                                                      MinBase.totalseconds(),
                                                      MinBase.days(),
                                                      MinBase.hours(),
                                                      MinBase.minutes(),
                                                      MinBase.seconds());

  // Done writing header information to Serial. Print a dashed line.
  dashedLine(80);
                                                    
  // set date time callback function
  SdFile::dateTimeCallback(setFileTime);

  // We need a pull-up on the open-collector interrupt pin of the PCF8523
  pinMode(INT_PIN, INPUT_PULLUP);
 
  // Register timerISR to handle 1 seconds square wave.
  // The Rtc updates the time just before the falling edge of INT_PIN.
  attachInterrupt(digitalPinToInterrupt(INT_PIN), timerISR, INT_MODE);
}

void loop () {
  DateTime            now;                              // Set to now when needed.

  // Do nothing if RTC was reset.
  if (RtcRst == true) {
    return;
  }

  // Set the clock to the new time.
  if (StrComplete) {
    switch(InBuf[0]) {
    case RTC_ADJUST:
      parseTimeStr(InBuf);
      break;
    case RTC_OFFSET:
      parseOffsetStr(InBuf);
      break;
    case RTC_RESET:
      Serial.println(F(">>> Resetting RTC."));
      Rtc.reset();
      RtcRst = true;
      return;
      break;
    default:
      Serial.printf(F(">>> Invalid coomand \"%s\"\n"), InBuf);
      break;
    }
    // clear the string.
    InStr.begin();
    StrComplete = false;
  }
  
  if (IntFlag == true) {
    // 1 second interrupt occurred.
    IntFlag = false;

    if (RtcInitialized == true) {
      // RTC time just changed. Skip the 1st interrupt while time stabilizes
      // and clear SecondCnt so it will be set from RTC.
      RtcInitialized = false;
      SecondCnt  = 0U;
      Serial.println(F(">>> RTC just initialized. Skipping this interrupt.\n"));
    }
    else {           
      if (SecondCnt % 60 == 0U) {
        // Print time data.
        unsigned long start = micros();
        now = Rtc.now();
        start = micros() - start;
        printTime(SecondCnt, now);
        Serial.printf(F("    RTC read time %luus.\n"), start);

        // Correct for any drift between secondTime and now.secondstime().
        if ( SecondCnt != now.secondstime() ) {
          Serial.printf(F(">>> Correcting SecondCnt %12ld, RTC %12ld.\n"),
                                                    SecondCnt,
                                                    now.secondstime());
          SecondCnt = now.secondstime();
        }
      }
        
      ++SecondCnt;
    }
  }
}

// The timerISR is triggered at the start of each second by the Interrupt pin on the PCF8523.
void timerISR()
{
  IntFlag = true;
}

void printTime(unsigned long secCnt, const DateTime &t) {
  long                rtcSecs       =   t.secondstime();  // Seconds since RTC start.
  TimeSpan            span          =   t - SetTime;      // Current timespan.
  float               ratio;                              // Span to Baseline ratio.

  Serial.printf(F("Time %10s %2u/%2u/%4u  %2u:%02u:%02u seconds %12ld.\n"),
                                                          WEEK_DAYS[t.dayOfTheWeek()],
                                                          t.month(),
                                                          t.day(),
                                                          t.year(),
                                                          t.hour(),
                                                          t.minute(),
                                                          t.second(),
                                                          rtcSecs);
  
  Serial.printf(F("    %4s, secCnt   %12ld, RTC seconds %12ld, diff %5ld.\n"),
                                                      digitalRead(INT_PIN) ? "HIGH" : "LOW",
                                                      secCnt,
                                                      rtcSecs,
                                                      secCnt - rtcSecs);

  Serial.printf(F("    Time since set %12ld seconds or %5d days %2d:%02d:%02d h:m:s.\n"),
                                                      span.totalseconds(),
                                                      span.days(),
                                                      span.hours(),
                                                      span.minutes(),
                                                      span.seconds());

  Serial.print(F("    Set time span to minimum baseline ratio percentage "));
  ratio = ((float)span.totalseconds() * 100.0) / (float)MinBase.totalseconds();
  Serial << _FLOATW(ratio, 0, 6);
  Serial.println(F("%."));
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  char                inChar;                           // Input character.
  
  while (Serial.available()) {
    if ( SetStart == 0UL) {
      SetStart = millis();
      //Serial.printf(F("SetStart = %12lu\n"), SetStart);
    }
    // get the new byte:
    inChar = (char)Serial.read();
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n' || inChar == '\r') {
      StrComplete = true;
    }
    else {
      // add it to the inputString.
      InStr += inChar;
    }
  }
}

/*
 *  uint16_t parseTimeStr(const char *tBuf)
 *    parseTimeStr() parses the tBuf string and sets Rtc if the string is valid.
 *    
 *  Inputs:
 *    tBuf    - character string holding the date and time. It 
 *              should be in the order "mon day yr hr min sec"
 *              such as "2 13 2017 14 17 30".
 *    
 *  Returns:
 *    A uint16_t bitmask with a bit set for each error in the tBuf date time string.
 *    The error bits are as follows:
 *      Return  Bit - Error
 *      0x0000   NA - No errors found. The RTC was set to the time in tBuf.
 *      0x0001    0 - String cannot be parsed for all parameters.
 *      0x0002    1 - Month invalid.
 *      0x0004    2 - Day invalid.
 *      0x0008    3 - Year invalid.
 *      0x0010    4 - Hour invalid.
 *      0x0020    5 - Minute invalid.
 *      0x0040    6 - Second invalid.
 */

uint16_t parseTimeStr(const char *tBuf) {
  unsigned int        year;                             // Current year.
  unsigned int        month;                            // Current month.
  unsigned int        day;                              // Current day.
  unsigned int        hour;                             // Current hour.
  unsigned int        minute;                           // Current minute.
  unsigned int        second;                           // Current second.
  int                 sRet;                             // sscanf return value.
  uint16_t            retval        =               0U; // Return value bitmask.
  unsigned long       setEnd;                           // Time delay to clock set.
  long                nCnt;                             // New second count.
 
  sRet = sscanf( tBuf, "a%u %u %u %u %u %u",  &month, &day,  &year,
                                              &hour, &minute, &second);

  if (sRet != 6) {
    Serial.printf( F(">>> Invalid adjust string \"%s\"\n"), tBuf);
    retval |= BIT_NUMBER(0);
  }
  else {
    // All values found in tBuf[].
    #ifdef DEBUG
    Serial.printf(F("    Time %c %2u/%2u/%4u  %2u:%02u:%02u.\n"), cmd,
                                                                  month,
                                                                  day,
                                                                  year,
                                                                  hour,
                                                                  minute,
                                                                  second);
    if (month < 1 || month > 12) {
      Serial.printf( F(">>> Invalid month  %2d\n"), month);
      retval |= BIT_NUMBER(1);
    }
    if (day < 1 || day > 31) {
      Serial.printf( F(">>> Invalid day    %2d\n"), day);
      retval |= BIT_NUMBER(2);
    }
    if (year < 2000 || year > 2999) {
      Serial.printf( F(">>> Invalid year %4d\n"), year);
      retval |= BIT_NUMBER(3);
    }
    if (hour > 23) {
      Serial.printf( F(">>> Invalid hour   %2d\n"), hour);
      retval |= BIT_NUMBER(4);
    }
    if (minute > 59) {
      Serial.printf( F(">>> Invalid minute %2d\n"), minute);
      retval |= BIT_NUMBER(5);
    }
      if (second > 59) {
      Serial.printf( F(">>> Invalid second %2d\n"), second);
      retval |= BIT_NUMBER(6);
    }
    #endif // DEBUG
  }

  if (retval == 0U) {
    DateTime nTime(year, month, day, hour, minute, second);
    Rtc.adjust(nTime);
    setEnd = millis();

    // Calculate time to set clock accounting for possible rollover.
    setEnd = SetStart < setEnd ? setEnd - SetStart : setEnd + ULONG_MAX - SetStart;
    SetStart  = 0UL;
    DateTime t(Rtc.now());
    nCnt      = t.secondstime();
    if (SetTimeValid) {
      Delta     = SecondCnt - nCnt;
      Baseline  = nCnt - SetTime.secondstime();
    }
    else {
      Delta     = 0;
      Baseline  = 1;
    }
    SecondCnt = nCnt;
    SetTime   = t;
    writeSetTime();
    Serial.printf(F(">>> Adjusted time %2u/%2u/%4u %2u:%02u:%02u, setting delay %lums.\n"),
                                                                        SetTime.month(),
                                                                        SetTime.day(),
                                                                        SetTime.year(),
                                                                        SetTime.hour(),
                                                                        SetTime.minute(),
                                                                        SetTime.second(),
                                                                        setEnd);
  Serial.printf(F("    Setting Baseline %ld, Delta %ld, Suggested adjustment %d.\n"),
                                                  Baseline, Delta, offsetAdjust());
    #ifdef FREE_RAM_FUNC
    Serial.printf(F("    Free RAM %d\n"), freeRam());
    #endif // FREE_RAM_FUNC
  }
  else {
    Serial.print(F("    Error bit mask returned "));
    Serial << _BINW(retval, 16, '0') << endl;
    Serial.println(F("    Valid string \"amon day yr hr min sec\""));
    Serial.println(F("    eg. \"a2 13 2017 14 17 30\""));
  }
  return retval;
}

/*
 *  uint16_t partOffsetStr(const char *tBuf)
 *    partOffsetStr() parses the tBuf string and sets Rtc offset if the string is valid.
 *    
 *  Inputs:
 *    tBuf    - character string holding the offset and mode. It 
 *              should be in the order "ooffset mode"
 *              such as "o-2 0".
 *    
 *  Returns:
 *    A uint16_t bitmask with a bit set for each error in the tBuf date time string.
 *    The error bits are as follows:
 *      Return  Bit - Error
 *      0x0000   NA - No errors found. The RTC was set to the time in tBuf.
 *      0x0001    0 - Invalid command char.
 *      0x0002    1 - Offset invalid.
 *      0x0004    2 - Mode invalid.
 */

uint16_t parseOffsetStr(const char *tBuf) {
  signed char         offset;                           // New offset value.
  Offset_t::offMode   mode;                             // New offset mode.
  int                 sRet;                             // sscanf return value.
  uint16_t            retval        =               0U; // Return value bitmask.
  unsigned long       setEnd;                           // Time delay to clock set.
 
  sRet = sscanf(tBuf, "o%d %d", &offset, &mode);

  if (sRet != 2) {
    Serial.printf( F(">>> Invalid offset string \"%s\"\n"), tBuf);
    retval |= BIT_NUMBER(0);
  }
  else {
    // All values found in tBuf[].
    #ifdef DEBUG
    Serial.printf(F("    Offset %c %3d mode %02u.\n"), cmd, offset, mode);
    #endif // DEBUG
    if (offset < -64 || offset > 63) {
      Serial.printf( F(">>> Invalid offset %3d\n"), offset);
      retval |= BIT_NUMBER(1);
    }
    if (mode < 0 || mode > 1) {
      Serial.printf( F(">>> Invalid mode    %2d\n"), mode);
      retval |= BIT_NUMBER(2);
    }
  }

  if (retval == 0U) {
    Off_t.setOffset(offset);
    Off_t.setMode(mode);
    Rtc.writeOffset(Off_t);
    setEnd = millis();

    // Calculate time to set clock accounting for possible rollover.
    setEnd = SetStart < setEnd ? setEnd - SetStart : setEnd + ULONG_MAX - SetStart;
    SetStart = 0UL;

    Off_t = Rtc.readOffset();
    Serial.printf(F(">>> Offset %3d mode %u setting delay %lums.\n"), Off_t.getOffset(),
                                                                      Off_t.getMode(),
                                                                      setEnd);
    Serial.printf(F("    RTC Val 0x%02x "), Off_t.getRTCVal());
    Serial << _BINW(Off_t.getRTCVal(), 8, '0') << endl;
    
    #ifdef FREE_RAM_FUNC
    Serial.printf(F("    Free RAM %d\n"), freeRam());
    #endif // FREE_RAM_FUNC
  }
  else {
    Serial.print(F("    Error bit mask returned "));
    Serial << _BINW(retval, 16, '0') << endl;
    Serial.println(F("    Valid string \"ooffset mode\""));
    Serial.println(F("    eg. \"o-2 0\""));
  }
  return retval;
}

bool readSetTime(void) {
  unsigned int        year;                             // Current year.
  unsigned int        month;                            // Current month.
  unsigned int        day;                              // Current day.
  unsigned int        hour;                             // Current hour.
  unsigned int        minute;                           // Current minute.
  unsigned int        second;                           // Current second.
  int                 sRet;                             // sscanf return value.
  bool                retval        =            false; // Return value bitmask.
  char                tBuf[IN_SIZE];                    // Buffer managed by PString.
  PString             tStr(tBuf, sizeof(tBuf));         // Pstring to hold incoming data.
  File                sFile;                            // Set file.
  char                c;                                // Character read.

  SetTime      = DateTime(F(__DATE__), F(__TIME__));
  SetTimeValid = false;
  Baseline     = 1L;
  Delta        = 0L;
  
  if (SdOK == false) {
    Serial.println(F(">>> SetTime not read, SD not initialized\n"));
    return false;
  }

  sFile = SD.open(SET_FILE, FILE_READ);

  if (sFile == false) {    
    Serial.printf(F(">>> File \"%s\" failed to open for read.\n"), SET_FILE);
    return false;
  }

  while (sFile.available()) {
    c = sFile.read();
    if (c == '\n') {
      break;
    }
    tStr += c;
  }
  sFile.close();
  
  sRet = sscanf( tBuf, "%u %u %u %u %u %u %ld %ld", &month, &day,  &year,
                                                    &hour, &minute, &second,
                                                    &Baseline, &Delta);
  if (sRet != 8) {
    Serial.printf( F(">>> Read innvalid adjust string \"%s\"\n"), tBuf);
    return false;    
  }
  
  #ifdef BLOMP
  Serial.printf(F("    readSetTime() %2u/%2u/%4u %2u:%02u:%02u, "), month,
                                                                    day,
                                                                    year,
                                                                    hour,
                                                                    minute,
                                                                    second);
  Serial.printf(F("Baseline %ld, Delta %ld.\n"), Baseline, Delta);
  #endif // BLOMP

  
  SetTime = DateTime(year, month, day, hour, minute, second);
  SetTimeValid = true;
  return true;
}

bool      writeSetTime(void) {
  File                sFile;                            // Set file.

  if (SdOK == false) {
    Serial.println(F(">>> SetTime not written, SD not initialized\n"));
    return false;
  }
  
  SD.remove(SET_FILE);
  
  sFile = SD.open(SET_FILE, FILE_WRITE);

  if (!sFile) {    
    Serial.printf(F(">>> File \"%s\" failed to open for write.\n"), SET_FILE);
    return false;
  }

  sFile.printf(F("%u %u %u %u %u %u %ld %ld\n"),
                                        SetTime.month(), SetTime.day(),    SetTime.year(),
                                        SetTime.hour(),  SetTime.minute(), SetTime.second(),
                                        Baseline,        Delta);
  sFile.close();
  Serial.printf(F("    Wrote \"%u %u %u %u %u %u %ld %ld\" to \"%s\"\n"),
                                        SetTime.month(), SetTime.day(),    SetTime.year(),
                                        SetTime.hour(),  SetTime.minute(), SetTime.second(),
                                        Baseline,        Delta,            SET_FILE);

  return true;
}

signed char offsetAdjust(void) {
  long                adj;                              // Adjustment value.

  if (!SetTimeValid) {
    return (signed char)0;
  }

  adj = (OFF_FACTOR[Off_t.getMode()] * Delta) / Baseline;
  //Serial.printf(F("    offsetAdjust()       raw adjustment %3ld\n"), adj);
  adj += Off_t.getOffset();
  //Serial.printf(F("    offsetAdjust() corrected adjustment %3ld\n"), adj);
  if ( adj < -64) {
    adj = -64;
  }
  else if ( adj > 63) {
    adj = 63;
  }
  //Serial.printf(F("    offsetAdjust() retval    adjustment %3ld\n"), adj);
  return (signed char)adj;
}

//------------------------------------------------------------------------------
// call back for file timestamps.

void setFileTime(uint16_t* date, uint16_t* time) {
  DateTime now = Rtc.now();
  #ifdef DEBUG
  sprintf(TimeStamp, "%02d:%02d:%02d %2d/%2d/%2d \n", now.hour(),now.minute(),now.second(),
                                                      now.month(),now.day(),now.year()-2000);
  Serial.print("In dateTime callback ");
  Serial.println(TimeStamp);
  #endif // DEBUG
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}
void dashedLine(byte cnt, char c) {
  while( cnt-- > 0) {
    Serial.write(c);
  }

  Serial.println();
}

#ifdef FREE_RAM_FUNC
int freeRam()
{
  extern int          __heap_start;                     // Heap start.
  extern int          *__brkval;                        // Break value. 
  int                 v;                                // &v is bottom of stack.
  
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);  
}
#endif // FREE_RAM_FUNC


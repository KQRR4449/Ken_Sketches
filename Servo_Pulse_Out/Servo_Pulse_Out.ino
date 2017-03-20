/*
 * Servo_Pulse_Out - Sketch to send one, ten or multiple servo pulses.
 * 
 * This sketch is designed to test the Servo_Test servo pulse reciever by sending
 * 1, 10, or a continuous string of various pulse widths.
 */
// Column locations.
//3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
//       1         2         3         4         5         6         7         8         9     9
//const unsigned long FAKE_LONG     =          10000UL; // Fake variable for column positions.

#include <PString.h>

//#define DEBUG

const byte            VER_MAJOR     =                1; // Major version.
const byte            VER_MINOR     =                1; // Minor version.
const size_t          IN_SIZE       =               32; // Size of input char buffer.
unsigned int          DEAD_T        =            40000; // Dead time in us.
byte                  SERVO_PIN     =                4; // Servo out pin.
const byte            REPS_FOREVER  =               10; // Sequential reps per loop
                                                        // when sequence repeats forever.

// Forward references.
void parseStr(void);
void serialEvent(void);

enum TstType {
  IDLE_T                            =              'i', // Idle.
  NOM_T                             =              'n', // 1500us nominal. 
  MIN_T                             =              'm', // 1200us minimum.
  MAX_T                             =              'M', // 1800us maximum.
  LOW_T                             =              'l', //  800us too low.
  HIGH_T                            =              'h', // 2200us too high.
  USER_T                            =              'u', // User set pulse time.
};

struct TstTimes {
  TstType             type;                             // Test type.
  unsigned int        width;                            // Pulse high time.
  const char          *title;                           // Title of test time.
};

const struct TstTimes TST_TBL[] =
{
  { IDLE_T,    0UL, "idle"          },
  { NOM_T,  1500UL, "nominal"       },
  { MIN_T,  1200UL, "minimum"       },
  { MAX_T,  1800UL, "maximum"       },
  { LOW_T,   300UL, "too low"       },
  { HIGH_T, 2700UL, "too high"      },
  { USER_T, 1500UL, "user defined"  },
};
const byte TST_TBL_SIZE = sizeof(TST_TBL) / sizeof(TstTimes);

enum TBL_INDEX {
  TBL_IDLE                          =                0, // Idle index.
  TBL_NOM                           =                1, // Nominal index.
  TBL_MIN                           =                2, // Minimum index.
  TBL_MAX                           =                3, // Maximum index.
  TBL_LOW                           =                4, // Too low index.
  TBL_HIGH                          =                5, // Too high index.
  TBL_USER                          =                6, // User defined index.
};

char                  InBuf[IN_SIZE];                   // Buffer managed by PString.
PString               InStr(InBuf, sizeof(InBuf));      // Pstring to hold incoming data.
bool                  StrComplete   =           false;  // Whether the string is complete.
TstTimes              CurTst        = TST_TBL[TBL_IDLE];// Current test time.
int                   CurReps     =                  0; // Current number of pulses.
unsigned int          CurWidth;                         // Current pulse width.
unsigned int          CurDead;                          // Current dead time.
bool                  CurForever;                       // If true. Repeat forever.

void setup() {
  // initialize serial:
  Serial.begin(9600);
  Serial.printf(F("Starting Servo_Pulse_Out version %d.%d, Build date %s %s\n"),
                                                                      VER_MAJOR,
                                                                      VER_MINOR,
                                                                      __DATE__,
                                                                      __TIME__);
    pinMode(SERVO_PIN, OUTPUT);
    digitalWrite(SERVO_PIN, LOW);
}

void loop() {
  byte                i;                                // Index variable.
  static bool         curChanged  =             false;  // If true, times just changed;
  
  // print the string when a newline arrives:
  if (StrComplete) {
    parseStr();
    // clear the string:
    InStr.begin();
    StrComplete = false;
    curChanged  = true;
  }

  for (i = 0; i < CurReps; i++) {
    digitalWrite(SERVO_PIN, HIGH);
    delayMicroseconds(CurWidth);

    digitalWrite(SERVO_PIN, LOW);
    delayMicroseconds(CurDead);

    #ifdef DEBUG
    Serial.printf(F("Sending width %5u, dead %5u, rep %3u\n"), CurWidth, CurDead, i+1);
    delay(1000);
    #endif // DEBUG   
  }

  if (curChanged && !CurForever) {
    Serial.printf(F("Ending   %12s width %5u, dead %5u, reps %3d.\n"),
                                                            CurTst.title,
                                                            CurWidth,
                                                            CurDead,
                                                            CurReps);
    CurTst      = TST_TBL[TBL_IDLE];
    CurReps     = 0;
    CurWidth    = 0;
    CurDead     = 0;
  }
  
  curChanged  = false; 
}

void parseStr(void) {
  unsigned int        pWidth;                           // Pulse width.
  int                 reps;                             // Pulse reps.
  char                cChar;                            // Command character.
  byte                i;                                // Index variable.
  
  CurTst      = TST_TBL[TBL_IDLE];
  CurReps     = 0;
  CurWidth    = 0;
  CurDead     = 0;
  CurForever  = false;
  
  if (InBuf[0] == IDLE_T) {
     ;
  }
  else if (sscanf(InBuf, "%u %d", &pWidth, &reps) == 2) {
    CurTst      = TST_TBL[TBL_USER];
    CurWidth    = pWidth;
    CurReps     = reps;
    CurDead     = DEAD_T - CurWidth;
  }
  else if (sscanf(InBuf, "%c %d", &cChar, &reps) == 2) {
    for(i = 0; i < TST_TBL_SIZE; i++) {
      if (TST_TBL[i].type == cChar) {
        break;
      }
    }

    if (i < TST_TBL_SIZE) {     
      CurTst      = TST_TBL[i];
      CurReps     = reps;
      CurWidth    = CurTst.width;
      CurDead     = DEAD_T - CurWidth;
    }
    else {
      Serial.printf(F(">>> Invalid string \"%s\".\n"), InBuf);
    }
  }
  else {
    Serial.printf(F(">>> Invalid string \"%s\".\n"), InBuf);
  }

  if (CurTst.type != IDLE_T && CurReps <= 0) {
    CurReps     = REPS_FOREVER;
    CurForever  = true;
  }
  Serial.printf(F("Starting %12s width %5u, dead %5u, reps %3d, forever %5s.\n"),
                                                            CurTst.title,
                                                            CurWidth,
                                                            CurDead,
                                                            CurReps,
                                                            CurForever ? "true" : "false");
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent(void) {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
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



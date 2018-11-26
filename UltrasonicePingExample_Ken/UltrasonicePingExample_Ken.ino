// ---------------------------------------------------------------------------
// Example NewPing library sketch that does a ping about 10 times per second.
// ---------------------------------------------------------------------------

#include <limits.h>
#include <NewPing.h>

// Uncomment the following line to simulate ping data.
//#define DBG_KGW // Set to simulate ultrasound raming up and down.

const uint8_t         TRIGGER_PIN   =               A4; // Trigger pin on the ultrasonic sensor.
const uint8_t         ECHO_PIN      =               A5; // Echo pin on the ultrasonic sensor.
const uint16_t        UNK_DISTANCE  =               0U; // Echo not returned value.
const uint16_t        MIN_DISTANCE  =               1U; // Minimum valid distance in centimeters.
const uint16_t        MAX_DISTANCE  =             300U; // Maximum valid distance in centimeters.
                                                        // Maximum sensor distance is
                                                        //   rated at 400-500cm.
                                                        
const byte            VER_MAJOR     =                1; // Major version in CV 7
const byte            VER_MINOR     =                2; // Minor version in CV 112
const uint8_t         LED_OVR       =               A3; // LED to show distance over limit.
const uint8_t         LED_UNK       =               A2; // LED to show distance unknown.
const uint16_t        LIMIT         =             102U; // Midpoint distance while seated in cm.
const uint16_t        HYSTERISIS    =               5U; // Hysterisis delta.
const uint8_t         IIR_DIV       =              16U; // IIR filter divisor (<= 131).

static uint16_t       DistLimit     = LIMIT-HYSTERISIS; // Current distance limit.

NewPing Sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);     // NewPing setup of pins
                                                        //  and maximum distance.
                                                       
// State machine states. These must begin with 0 followed by 1, etc.
// S_State_Tbl must match the order of the StateNum enum.
enum StateNum {
  S_INIT                            =                0, // Initial state.
  S_UNK                             =                1, // Unknown state.
  S_NEAR                            =                2, // Near the device.
  S_FAR                             =                3, // Far from the device.
};

StateNum              CurState      =           S_INIT; // Current state.

struct S_State {
  StateNum             state;                           // Ping state.
  char                 *sName;                          // State name to printt.
};
const S_State S_State_Tbl[] = {
  {       S_INIT,   "S_INIT"  },                        // 0: Initial state.
  {        S_UNK,   "S_UNK",  },                        // 1: Unknown ping state.
  {       S_NEAR,   "S_NEAR", },                        // 2: Near the desk state.
  {        S_FAR,   "S_FAR",  },                        // 3: Far from desk state.
};
const int8_t S_State_Tbl_Size = sizeof(S_State_Tbl)/sizeof(S_State);

struct P_Range {
  uint16_t            pLow;                             // Lowest ping time.
  uint16_t            pHigh;                            // Highest ping time.
  char                *pMsg;                            // Message to print.
};
const P_Range P_Range_Tbl[] = {
  {           0U,   0U,           "Unknown"          }, // Unknown ping time.
  { MIN_DISTANCE,   LIMIT-20,     "Close",           }, // Seated close.
  {     LIMIT-20,   LIMIT-11,     "Minus 20-11",     }, // Seated -11 to -20.
  {     LIMIT-10,   LIMIT- 6,     "Minus 10- 6",     }, // Seated -10 to  -6.
  {     LIMIT- 5,   LIMIT- 5,     "Minus  5",        }, // Seated - 5.
  {     LIMIT- 4,   LIMIT- 4,     "Minus  4",        }, // Seated - 4.
  {     LIMIT- 3,   LIMIT- 3,     "Minus  3",        }, // Seated - 3.
  {     LIMIT- 2,   LIMIT- 2,     "Minus  1",        }, // Seated - 2.
  {     LIMIT- 1,   LIMIT- 1,     "Minus  1",        }, // Seated - 1.
  {     LIMIT,      LIMIT,        "Limit",           }, // Seated limit.
  {     LIMIT+ 1,   LIMIT+ 1,     "Plus   1",        }, // Away   + 1.
  {     LIMIT+ 2,   LIMIT+ 2,     "Plus   2",        }, // Away   + 2.
  {     LIMIT+ 3,   LIMIT+ 3,     "Plus   3",        }, // Away   + 3.
  {     LIMIT+ 4,   LIMIT+ 4,     "Plus   4",        }, // Away   + 4.
  {     LIMIT+ 5,   LIMIT+ 5,     "Plus   5",        }, // Away   + 5.
  {     LIMIT+ 6,   LIMIT+10,     "Plus   6-10",     }, // Away   + 6 - 10.
  {     LIMIT+11,   LIMIT+20,     "Plus  11-20",     }, // Away   +11 - 200.
  {     LIMIT+21,   MAX_DISTANCE, "Far away"         }, // Away far.
};
const int8_t P_Range_Size = sizeof(P_Range_Tbl)/sizeof(P_Range);

class PingStats {
  public:
    PingStats(void) {
      pIir = 0UL;
      clearStats();
    }

    // Simple get methods.
    uint16_t get_iir(void) {  return pIir; }

    void clearStats(void) {
      uint8_t       i;                                  // Index variable.

      pCnt = 0UL;
      p0   = 0UL;
      pSum = 0UL;

      for ( i = 0; i < P_Range_Size; i++) {
        pTbl[i] = 0UL;
      }
    }

    void updateStats(uint16_t p) {
      uint8_t     i;                                    // Index variable.
      uint16_t    pTime;                                // Corrected ping time.

      incCnt(pCnt);

      if (p == 0) {
        incCnt(p0);
        pTime = MAX_DISTANCE;
        //pTime = p;
      }
      else {
        pTime = p;
      }

      if ((pSum + pTime) >= pSum) {
        pSum += pTime;
      }
      else {
        pSum = ULONG_MAX;
      }

      iir(pTime);

      for (i = 0; i < P_Range_Size; i++) {
        if (P_Range_Tbl[i].pLow <= pIir && pIir <= P_Range_Tbl[i].pHigh) {
          incCnt(pTbl[i]);
          return;
        }
      }
      Serial.printf(F("\n\n******* ERROR: No range entry found for %3u\n\n"), pIir);
    }

    uint16_t average(void) const {
      if (pCnt == 0UL) {
        return 0U;
      }
      else if ((pCnt < ULONG_MAX) && (pSum < ULONG_MAX)) {
        return (uint16_t)(pSum / pCnt);
      }
      else {
        return USHRT_MAX;
      }
    }

    void printStats(void) const{
      uint8_t         i;                                // Index variable.

      Serial.printf(F("\n\n Stats: Pings %10lu, Unk %10lu, Sum %10lu, "),
                                                          pCnt,
                                                          p0,
                                                          pSum);

      Serial.printf(F("Avg %3u, IIR %3u DistLimit %3u, "),average(), pIir, DistLimit);

      Serial.printf(F("State %s\n"), S_State_Tbl[CurState].sName);

      for (i = 0; i < P_Range_Size; i++) {
        Serial.printf(F("        Range %3ucm - %3ucm, Count %10lu - %s\n"),
                                                                  P_Range_Tbl[i].pLow,
                                                                  P_Range_Tbl[i].pHigh,
                                                                  pTbl[i],
                                                                  P_Range_Tbl[i].pMsg);
      }
      Serial.println();
    }

  private:
    void incCnt(uint32_t &cnt) {
      if (cnt < ULONG_MAX) {
        ++cnt;
      }
    }

    // Calculate IIR. Note: IIR_DIV must be less than 131 to fit in uint16_t.
    void iir(uint16_t p) {
      //pIir = (p + (pIir * (IIR_DIV - 1))) / IIR_DIV;
      uint16_t        mult;

      mult = p + ((pIir * IIR_DIV ) - pIir);
      pIir = mult / IIR_DIV;
      if ((mult % IIR_DIV) >= (IIR_DIV / 2)) {
        ++pIir;
      }
    }

    uint32_t          pTbl[P_Range_Size];               // Range count table.
    uint32_t          pCnt;                             // Count of received pings.
    uint32_t          p0;                               // Count of received 0 pings.
    uint32_t          pSum;                             // Sum of non-zero pings.
    uint16_t          pIir;                             // IIR filtered pings.
};

PingStats Stats;

void setup() {
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.

  pinMode(LED_OVR, OUTPUT);
  pinMode(LED_UNK, OUTPUT);

  Serial.printf(F("Starting Ultrasound Test Version %d.%d, Build date %s %s\n"),
                                                            VER_MAJOR,
                                                            VER_MINOR,
                                                            __DATE__,
                                                            __TIME__);
  Serial.printf(F("MAX_DISTANCE %3u, RANGE %3u - %3u, IIR_DIV %3u\n"),
                                                            MAX_DISTANCE,
                                                            LIMIT - HYSTERISIS,
                                                            LIMIT + HYSTERISIS,
                                                            IIR_DIV);
  Serial.println(F("Cmds: s - Print stats, n - Toggle EOL"));
  #ifdef DBG_KGW
  Serial.println();
  Serial.println(F("********** WARNING: DATA IS SIMULATED **********"));
  #endif // DBG_KGW
  Serial.println(F("-------------------------------------"));
  Serial.println();
}

void loop() {
  #ifdef DBG_KGW
  static bool         cntUp         =            false; // DEBUG count up if true.
  static uint16_t     dist_cm       =     MIN_DISTANCE; // Distance in cm.
  #else
  uint16_t            dist_cm;                          // Distance in cm.
  #endif
  uint16_t            curIir;                           // Current IIR value.
  static bool         crEOL         =            false; // Use CR for ping EOL if set.
  static bool         doPings       =            false; // Show ping time every cycle.
  StateNum            newState;                         // New distance state.
  int                 c;                                // Received character.

  delay(100); // Wait between in ms. 29ms should be the shortest delay between pings.

  // DEBUG start
  #ifdef DBG_KGW // Simulate ultrasound ramping up and down.
  if (cntUp == true) {
    if (dist_cm < MAX_DISTANCE) {
      ++dist_cm;
    }
    else {
      cntUp = false;
      --dist_cm;
    }
  }
  else { // cntUp == false.
    if ( dist_cm > 0) {
      --dist_cm;
    }
    else {
      cntUp =  true;
      ++dist_cm;
    }
  }
  #else
  dist_cm = Sonar.ping_cm();
  //dist_cm = Sonar.convert_cm(Sonar.ping_median());
  #endif

  // Update the statistics.
  Stats.updateStats(dist_cm);
  curIir = Stats.get_iir();

  // Get single character Serial commands.
  if (Serial.available()) {
    c = Serial.read();
    //Serial.printf("DEBUG: '%c' %2d received\n", (char)c, c);
    // Get the new byte and process it.
    switch ((char)c) {
    case 'n':
    case 'N':
      // Toggle crEOL.
      crEOL = !crEOL;
      //Serial.printf("DEBUG: crEOL = %s\n", crEOL ? "true" : "false");
      break;
    case 's':
    case 'S':
      // Print and reset stats.
      Stats.printStats();
      Stats.clearStats();
      break;
    case 'p':
    case 'P':
      // Toggle doPings.
      doPings = !doPings;
      //Serial.printf("DEBUG: doPings = %s\n", doPings ? "true" : "false");
      break;
    default:
      break;
    }
  }

  // Check the disance. 0 is returned when the distance is too long to read.
  if (curIir == UNK_DISTANCE) {
    //Serial.println("    unknown");
    //digitalWrite(LED_OVR, LOW);
    //digitalWrite(LED_UNK, HIGH);
    newState = S_UNK;
  }
  else if (curIir <= DistLimit) {
    //Serial.println("yes");
    //digitalWrite(LED_OVR, LOW);
    //digitalWrite(LED_UNK, LOW);
    newState = S_NEAR;
  }
  else
  {
    //Serial.println("********** NO **********");
    //digitalWrite(LED_OVR, HIGH);
    //digitalWrite(LED_UNK, LOW);
    newState = S_FAR;
  }

  if (newState != CurState) {
    CurState =   newState;
    Serial.printf(F("IIR:  %3ucm: "),  curIir);
    switch (CurState) {
      case S_UNK:
        Serial.println(F("unknown"));
        digitalWrite(LED_OVR, LOW);
        digitalWrite(LED_UNK, HIGH);
        DistLimit = LIMIT - HYSTERISIS;
        break;
      case S_NEAR:
        Serial.println(F("seated"));
        digitalWrite(LED_OVR, LOW);
        digitalWrite(LED_UNK, LOW);
        DistLimit = LIMIT + HYSTERISIS;
        break;
      case S_FAR:
        Serial.println(F("********** NO **********"));
        digitalWrite(LED_OVR, HIGH);
        digitalWrite(LED_UNK, LOW);
        DistLimit = LIMIT - HYSTERISIS;
        break;
      default:
        Serial.printf(F("XXXXXXXXXX Invalid state %d XXXXXXXXXX\n"), CurState);
        digitalWrite(LED_OVR, HIGH);
        digitalWrite(LED_UNK, HIGH);
        DistLimit = LIMIT - HYSTERISIS;
        break;
    }
  }

  if (doPings == true) {
    Serial.printf(F("Ping: %3ucm, IIR %3ucm, Limit %3ucm, - %-8s%c"),
              dist_cm, curIir, DistLimit, S_State_Tbl[CurState].sName, crEOL ? '\r' : '\n');
  }
}

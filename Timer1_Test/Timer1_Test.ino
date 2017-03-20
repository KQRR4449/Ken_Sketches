#include <TimerOne.h>
#include <Streaming.h>
#include <StopWatch.h>

//#define DEBUG_ISR                                             // Debug ISR.

// This example uses the timer interrupt to blink an LED
// and also demonstrates how to share a variable between
// the interrupt and the main program.


const byte              VER_MAJOR       =                  1; // Major version.
const byte              VER_MINOR       =                  1; // Minor version.
const unsigned long     US_PER_SEC      =            1000000; // Microseconds per second.
const unsigned long     INT_TIME        =     1 * US_PER_SEC; // 1 second interrupt interval.
const byte              INT_CNT         =                  1; // Interrupt count to action.

bool                    LedState        =                LOW; // LED on/off state.
bool                    FirstLoop       =               true; // True for first tick.

// Timer1 variables.
volatile byte           TimerCnt        =                 1; // Counts before display.
                                                             // Initially wait 1 second.
volatile bool           OverrunFlag     =             false; // True if interrupt not handled
                                                             // before next tick.

StopWatch               MyWatch;                              // StopWatch object.

void setup(void) {
  Serial.begin(9600);
  Serial.printf(F("Starting Timer1_Test version %d.%d, Build date %s %s\n"),
                                                              VER_MAJOR,
                                                              VER_MINOR,
                                                              __DATE__,
                                                              __TIME__);
  Serial << F("INT_TIME ") << INT_TIME << F(", INTERVAL ") << INT_CNT << endl;

  // Setup timer 1 to call timerISR() once a second.
  Timer1.initialize(INT_TIME);
  Timer1.attachInterrupt(timerISR);

  // Print time at end of setup().
  Serial << F("Setup end  ") << millis() << F("ms") << endl;
}

void loop(void) {
  static byte           loops           = 0;                  // Loops per interrupt.
  unsigned long         theTime;                              // Elapsed time in ms.
    
  if (FirstLoop == true) {
    // Get the elapsed ms. as soon as loop starts.
    theTime   = millis();
    FirstLoop = false;

    // Print time just before setting up the Timer 1 interrupt.
    Serial << F("Loop start ") << millis() << F("ms") << endl;
  }
  
  // Interrupt has just occurred.
  if (TimerCnt == 0) {
    // Get the elapsed ms. as soon as we enter.
    theTime   = millis();
    
    TimerCnt = INT_CNT ;

    blinkLED();

    //MyWatch.getTime();  // This extraneous call is necessary to stop spill 
    Serial.printf(F("%s - loops %3u, %10lums\n"), MyWatch.getTime(),
                                                  loops,
                                                  theTime);
    MyWatch.tick();
    loops     = 0;
  } // End if (TimerCnt == 0)

  delay(10);

  ++loops;
}

// Blink the LED.
void blinkLED(void)
{
  LedState = !LedState;
  digitalWrite(LED_BUILTIN, LedState);
}

// Timer 1 Interrupt Service Routine.
void timerISR(void) {
  #ifdef DEBUG_ISR
  Serial << F("ISR ") << millis() << F(", TimerCnt ") << TimerCnt << endl;
  #endif // DEBUG_ISR

  if (TimerCnt > 0) {
    --TimerCnt;
  }
  else {
    OverrunFlag = true;
  }
}



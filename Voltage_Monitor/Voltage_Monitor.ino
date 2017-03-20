/*
 *  Voltage monitor program. The program reads analog input once a minute.
 *  It uses this value to calculate the min, maxm and average values and prints
 *  the results once a minute. Pressing the button prints the results immediately.
 *  The brightness of the LED connected to the ANALOG_OUT_PIN will show the Voltage level.
 *  
 *  The following pins are used:
 *    A5 - ANALOG_IN_PIN  Analog input connected to Voltage to monitor (0 to 5V).
 *     3 - ANALOG_OUT_PIN PWM output connected to LED. LED brightness shows Voltage level.
 *     2 - INTERRUPT_PIN  Connected to purshbutton.
 */

#include <TimerOne.h>
#include <Streaming.h>
#include <StopWatch.h>

//#define DEBUG                                           // Set to debug program.
//#define DEBUG_ISR                                       // Set to debug ISR program.
//#define FREE_RAM                                        // Report free ram.
#define PWM_OUT                                         // PWM to ANALOG_OUT_PIN.

const byte          VER_MAJOR       =               1; // Major version.
const byte          VER_MINOR       =               3; // Minor version.
const int           ANALOG_IN_PIN   =              A5; // Analog input pin.
const int           ANALOG_OUT_PIN  =               3; // Analog PWM output pin.
const byte          INTERRUPT_PIN   =               2; // Pushbutton pin
const int           MIN_VALUE       =               0; // Min analog value.
const int           MAX_VALUE       =            1023; // Max analog value.
const byte          SHIFT_VAL       =               3; // IIR shift value (divide by 8).
                                                       // Cannot exceed 6 for 10 bit A to D.
const float         VOLT_RATIO      = 5.0 / MAX_VALUE; // Count to Voltage ratio.
const unsigned long AD_WAIT         =             2UL; // A to D conversion wait ms.
const unsigned long US_PER_SEC      =       1000000UL; // Microseconds per second.
const unsigned long ONE_SECOND      =  1 * US_PER_SEC; // 1 second interrupt interval.
const byte          INT_CNT         =               1; // Interrupt count to action.
                                                       // Set to wait 1 seconds initially.
const byte          PRINT_TIME      =              60; // Data print time in INTERVALS.
const unsigned long DEBOUNCE_TIME   =              50; // Debounce time in ms.

bool                firstLoop       =            true; // True if just starting loop().     
int                 sensorValue;                       // Value read from analog input.
int                 minValue        =       MAX_VALUE; // Minimum analog value.
int                 maxValue        =       MIN_VALUE; // Maximum analog value.
int                 minOverall      =       MAX_VALUE; // Minimum analog value.
int                 maxOverall      =       MIN_VALUE; // Maximum analog value.
unsigned int        avg             =              0U; // IIR average reading.
byte                printTime       =      PRINT_TIME; // Print at beginning of run.
#ifdef PWM_OUT 
int                 outputValue;                       // Analog out value.
#endif // PWM_OUT

//  Variables used by button interrupt handler.
volatile bool       btnPressed      =          false; // true if button pressed.
byte                btnState;                         // Button state.

// Timer1 variables.
volatile byte       timerCnt      =                1; // Counts before display.
                                                      // Initially wait 1 second.
volatile bool       overrunFlag   =            false; // True if interrupt not handled
                                                      // before next tick.

StopWatch           theWatch;                         // Elapsed time.

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(115200);
  Serial.printf(F("Starting Voltage Monitor version %d.%d, Build date %s %s\n"),
                                                                          VER_MAJOR,
                                                                          VER_MINOR,
                                                                          __DATE__,
                                                                          __TIME__);
    
  /*
   * The following lines set up the button interrupt variables and attach
   * the interrupt handler. Make sure to set btnState prior to attaching the
   * interrupt in case the button is pressed when the program resets.
   */
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  btnState = digitalRead(INTERRUPT_PIN);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), buttonISR, CHANGE);
}

void loop() {
  if (firstLoop == true) {
    firstLoop = false;
    
    // Wait 5 seconds for A to D to settle then start timer interrupt.
    delay(5000UL);
    
    // Setup timer 1 to call timerISR() once a second.
    Timer1.initialize(ONE_SECOND);
    Timer1.attachInterrupt(timerISR);

    // Return from loop immediately.
    return;
}
  // read the analog in value:
  sensorValue = analogRead(ANALOG_IN_PIN);

  // Find the current and overall min and max analog values.
  minValue   = (sensorValue <   minValue) ? sensorValue : minValue;
  maxValue   = (sensorValue >   maxValue) ? sensorValue : maxValue;
  minOverall = (sensorValue < minOverall) ? sensorValue : minOverall;
  maxOverall = (sensorValue > maxOverall) ? sensorValue : maxOverall;

  // Find IIR average. Note: this calculate avg << SHIFT_VAL to maintain accuracy.
  // For a max sensorValue of 1023, SHIFT_VAL cannot exceed 6.
  avg = sensorValue + avg - (avg >> SHIFT_VAL);

  #ifdef PWM_OUT 
  // Map sensorValue (0 - 1023) to analog out value (0 - 255).
  outputValue = map(sensorValue, 0, 1023, 0, 255);
  // Update the analog out value.
  analogWrite(ANALOG_OUT_PIN, outputValue);
  #endif // PWM_OUT

  if (btnPressed == true) {                           // Button just pressed.
    btnPressed = false;

    printData();
  }
  
  if (timerCnt == 0) {                                // Print data periodically.
    timerCnt = INT_CNT;
    
    #ifdef DEBUG
    Serial << F("loop: printTime ") << printTime << endl;
    #endif // DEBUG

    if (++printTime >= PRINT_TIME) {
      printTime = 0;
      printData();

      // Reset current min and max values.
      minValue = MAX_VALUE;
      maxValue = MIN_VALUE;
    }
    
    theWatch.tick();
  }

  // wait 2 milliseconds before the next loop
  // for the analog-to-digital converter to settle
  // after the last reading:
  delay(AD_WAIT);
}

void printData() {
  #ifdef DEBUG
  // Note: printf split in 2 because very long command blows up.      
  Serial.printf(F("avg %4u, min %4d, max %4d, minOverall %4d, maxOverall %4d "),
                                                                          avg >> SHIFT_VAL,
                                                                          minValue,
                                                                          maxValue,
                                                                          minOverall,
                                                                          maxOverall);
  Serial.printf(F("pwm, %3d, %10lu\n"),
                                                                          outputValue,
                                                                          millis());
  #endif // DEBUG
    
  if (overrunFlag == true) {
    Serial.printf(F("%s - >> Overrun occurred\n"), theWatch.getTime());      
  }
  Serial.printf(F("%s - Average "), theWatch.getTime());
  Serial << _FLOAT((float)(avg >> SHIFT_VAL) * VOLT_RATIO, 3);
  Serial << F(", Period ") << _FLOAT((float)minValue  * VOLT_RATIO, 3);
  Serial << F(" - ")              << _FLOAT((float)maxValue  * VOLT_RATIO, 3);
  //Serial.print("                                  ");
  Serial << F(" Overall ")  << _FLOAT((float)minOverall * VOLT_RATIO, 3);
  Serial << F(" - ")             << _FLOAT((float)maxOverall * VOLT_RATIO, 3) << endl;
}

// Timer 1 Interrupt Service Routine.
void timerISR(void) {
  #ifdef DEBUG_ISR
  Serial << F("ISR ") << millis() << F(", timerCnt ") << timerCnt << endl;
  #endif

  if (timerCnt > 0) {
    --timerCnt;
  }
  else {
    overrunFlag = true;
  }
}

/*******************************************************************************************
 * 
 * buttonISR() - Handle interrupt when button is pressed or released.
 *
 * buttonISR() toggles btnState if buttonISR() is entered after the
 * DEBOUNCE_TIME. Calls to buttonISR() before DEBOUNCE_TIME are ignored.
 * It is important that btnState be set to the current state of the
 * button prior to registering the interrupt or the button may work out
 * of phase.
 *
 * Inputs
 *  None.
 *
 * Returns:
 *  None.
 *  
 *******************************************************************************************/

void buttonISR() {
  unsigned long             currentTime;                    // Current time in ms.
  static unsigned long      endTime = 0UL;                  // End debounce time in ms.

  currentTime = millis();
  
  if (endTime < currentTime) {                              // Ignore interrupts caused
                                                            //   by switch bounce.

    endTime = currentTime + DEBOUNCE_TIME;                  // Set lastInterrupt to be ready
                                                            //   for next press or release.

    btnState = !btnState;                                   // Toggle bntState on first
                                                            //   button down or up interrupt.
 
    if (btnState == LOW) {                                  // Set btnState if button
      btnPressed = true;                                    //   just pressed.
    }
  }

  #ifdef FREE_RAM
  Serial.print("buttonISR: free ram ");
  Serial.println(freeRam());
  #endif
}

/*******************************************************************************************
 * 
 * freeRam() - Return the remaining free RAM shared by the heap and stack.
 *
 * Inputs
 *  None.
 *
 * Returns:
 *  int _free - The remaining free RAM.
 *  
 *******************************************************************************************/

#ifdef FREE_RAM
int freeRam()
{
  extern int                __heap_start;                   // Heap start.
  extern int                *__brkval;                      // Break value. 
  int                       v;                              // &v is bottom of stack.
  
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);  
}
#endif


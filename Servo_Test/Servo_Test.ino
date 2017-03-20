/*******************************************************************************************
 *
 * This sketch interfaces servo DCC decoders to the decoder tester.
 * The connections are as follows:
 * 
 * Pin  Mode            Description
 * ------------------------------------------------
 * D2   INPUT_PULLUP    Pushbutton.
 * D3   INPUT           DCC servo decoder input.
 * D4   OUTPUT          Error LED.
 * D5   OUTPUT          Out A. LOW for Lenz -, SEND.EXE S 0, Forward.
 * D6   OUTPUT          Out B. Inverse of Out A.
 * D13  OUTPUT          State LED. Follows Out A.
 * 
 *******************************************************************************************/

#include <limits.h>                             // Min and max values of int etc.
#include <Streaming.h>                          // Overloaded << for Print class.

//#define FREE_RAM                                // Report free ram. Also sets DEBUG.
//#define DEBUG                                   // Print DEBUG output. Also sets ERROR_ONLY.
#define ERROR_ONLY                              // Define to print errors only.

#ifdef FREE_RAM                                 // Force DEBUG & ERROR_ONLY if FREE_RAM
#define DEBUG                                   //   defined to maximize memory usage.
#define ERROR_ONLY
#endif // FREE_RAM

#ifdef DEBUG                                    // Force ERROR_ONLY if DEBUG defined
#define ERROR_ONLY                              //   to print debug and error information.
#endif // DEBUG


const byte          VER_MAJOR       =     1;    // Major version.
const byte          VER_MINOR       =     4;    // Minor version.
const byte          INTERRUPT_PIN   =     2;    // Pushbutton pin.
const byte          S_IN_PIN        =     3;    // Servo signal input pin.
const byte          ERROR_PIN       =     4;    // Error pin.
const byte          OUT_A           =     5;    // Output A.
const byte          OUT_B           =     6;    // Output B.
const byte          S_OUT_PIN       =    13;    // Servo state output pin.
const unsigned long DEBOUNCE_TIME   =    50UL;  // Debounce time in ms.
const unsigned long SERVO_MIN       =   500UL;  // Minimum valid servo time microseconds.
const unsigned long SERVO_MAX       =  2500UL;  // Maximum valid servo time microseconds.
const unsigned long SERVO_MID       =  1500UL;  // Servo midpoint in microseconds.

// Servo signal input state machine states.
enum inputState {
  IN_INIT                           =     'I',  // Initial state after boot or button press.
  IN_LOW                            =     'L',  // Input state is LOW.
  IN_HIGH                           =     'H',  // Input state is HIGH.
  IN_HIGH_ERROR                     =     'E',  // Input state is HIGH too long.
};

// Error numbers.
enum errNum {
  ERR_NONE                          =     0,    // No error.
  ERR_SHORT                         =     1,    // Pulse too short.
  ERR_LONG                          =     2,    // Pulse too long.
};
const byte          NUM_ERRORS      =     3;    // Number of errors.               

#ifdef ERROR_ONLY
// Table of error messages.
const char          *ERROR_TBL[] = {
  "    None",                                   // 0 ERR_NONE.
  "Underrun",                                   // 1 ERR_SHORT.
  " Overrun"                                    // 2 ERR_LONG.
};
#endif // ERROR_ONLY

#ifdef DEBUG
// Midpoint values to display.
enum midVal {
  MID_NONE                          =   'X',    // No midpoint value.
  MID_LOW                           =   'L',    // Low midpoint value.
  MID_HIGH                          =   'H'     // High midpoint value.
};
#endif // DEBUG

// Forward reference methods with optional arguments.
void errorHandler(const errNum err, const char state, const bool fatal = false);

char                sState          = IN_INIT;   // State machine state.
unsigned long       start           = 0UL;       // Start time in microseconds.
unsigned long       maxEnd          = 0UL;       // Maximum pulse end time.
errNum              lastError       = ERR_NONE;  // Last error, if any.
unsigned long       width;                       // Pulse width in microseconds.

#ifdef DEBUG
char                lastMid         = MID_NONE;  // Last midpoint state init invalid.
#endif // DEBUG

#ifdef ERROR_ONLY
unsigned long       minWidth        = ULONG_MAX; // Min servo pulse width.
unsigned long       maxWidth        = 0UL;       // Max servo pulse width.
// Error count table.
unsigned long       errCnt[NUM_ERRORS] = {0UL, 0UL, 0UL};
#endif // ERROR_ONLY


//  Variables used by button interrupt handler.
volatile bool       btnPressed      = false;      // true if button pressed.
byte                btnState;                     // Button state.

void setup() {
  #ifdef ERROR_ONLY
  Serial.begin(9600);
  Serial << F(">> Starting Servo_Test version ") << VER_MAJOR << F(".") << VER_MINOR;
  Serial << F(", Pulse range ") << SERVO_MIN << F(" to ") << SERVO_MAX << F(" us.") << endl;
  #endif // ERROR_ONLY

  pinMode(S_IN_PIN, INPUT);
  pinMode(S_OUT_PIN, OUTPUT);
  pinMode(ERROR_PIN, OUTPUT);
  pinMode(OUT_A, OUTPUT);
  pinMode(OUT_B, OUTPUT);

  setOutputs(LOW);                                          // Set servo output pins LOW.
  digitalWrite(ERROR_PIN, LOW);                             // Turn off error LED.
      
  /*
   * The following lines sets up the button interrupt variables and attaches
   * the interrupt handler. Make sure to set btnState prior to attaching the
   * interrupt in case the button is pressed when the program resets.
   */
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  btnState = digitalRead(INTERRUPT_PIN);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), buttonISR, CHANGE);
}

void loop() {
  if (btnPressed == true) {                                 // Button just pressed.
    btnPressed = false;

    if (lastError != ERR_NONE) {                            // Returning from an error.
      #ifdef ERROR_ONLY
      Serial << F("Clearing error: lastError ") << ERROR_TBL[lastError] << endl;
      #endif ERROR_ONLY

      #ifdef DEBUG
      lastMid     = MID_NONE;
      #endif // DEBUG

      // Reinitialize state variables to IN_INIT
      //   reset outputs, and clear the error.
      lastError   = ERR_NONE;
      sState      = IN_INIT;

      setOutputs(LOW);                                      // Set servo output pins LOW.
      digitalWrite(ERROR_PIN, LOW);                         // Turn off error LED.
    } // End returning from an error.
    
    #ifdef ERROR_ONLY
    displayErrorData();
    clearErrorData();
    #endif // ERROR_ONLY
  } // End button just pressed.

  // State machine to follow servo input transitions, measure pulse width,
  //   and set outputs appropriately.
  switch (sState) {
    
    // Initial state. Must synchronize with rising edge of input signal.
    case IN_INIT:
      // Initialize sState and timers.
      sState = IN_LOW;                                      // Normally go to IN_LOW.
      start  = micros();
      maxEnd = start + SERVO_MAX;
      
      // Wait for input to go LOW if it is initially HIGH.
      // This synchronizes the state machine with the input pulse train.
      while (digitalRead(S_IN_PIN) == HIGH) {
        if (micros() > maxEnd) {                            // Input HIGH too long.
          sState = IN_HIGH_ERROR;                           // Go to IN_HIGH_ERROR.
          break;         
        }
      }
      break; // End IN_INIT.

    // Input is currently LOW. Wait for it to go high.
    case IN_LOW:
      if (digitalRead(S_IN_PIN) == HIGH) {                  // Input just went HIGH.
        // Set start and maxEnd timer at start of HIGH pulse.
        start  = micros();
        maxEnd = start + SERVO_MAX;
        
        sState = IN_HIGH;                                   // Go to IN_HIGH
      }      
      break; // End IN_LOW.

    // Input is currently HIGH.
    case IN_HIGH:
      if (digitalRead(S_IN_PIN) == LOW) {                   // Input just went LOW.
          if (setMidpint() != true) {                       // Pulse width too short.
            errorHandler(ERR_SHORT, IN_HIGH);               // Pulse too short.
          }
          sState = IN_LOW;                                  // Go to IN_LOW.
      }
      else if (micros() > maxEnd) {                         // Pulse HIGH for too long.
        sState = IN_HIGH_ERROR;                             // Go to IN_HIGH_ERROR.
      }
      break; // End IN_HIGH.

    // Input is currently HIGH for too long.
    case IN_HIGH_ERROR:
      if (digitalRead(S_IN_PIN) == LOW) {                   // Input just went LOW.        
        setMidpint();
        
        #ifdef ERROR_ONLY
        Serial << F("State E input low. Width ") << width << endl;
        #endif
        
        errorHandler(ERR_LONG, IN_HIGH_ERROR);
        sState = IN_LOW;
      }
      break; // End IN_HIGH_ERROR.
      
  } // End state machine.
}

// Set 3 outputs.
void setOutputs(const uint8_t level) {
  digitalWrite(S_OUT_PIN,  level);                           // Servo state LED.
  digitalWrite(OUT_A,      level);                           // Balanced pair A.
  digitalWrite(OUT_B,     !level);                           // Balanced pair B.
}

bool setMidpint() {
  #ifdef DEBUG
  char                      midpoint;                       // Midpoint state.
  #endif // DEBUG
  
  width = micros();                                         // Set pulse end time.

  // Calculate width accounting for possible rollover.
  width = start < width ? width - start : width + ULONG_MAX - start;
  
  if (width <= SERVO_MID) {
    #ifdef DEBUG
    midpoint = MID_LOW;
    #endif // DEBUG
    setOutputs(LOW);
  }
  else {
    #ifdef DEBUG
    midpoint = MID_HIGH;
    #endif // DEBUG
    setOutputs(HIGH);
  }

  #ifdef ERROR_ONLY
  minWidth = width < minWidth ? width : minWidth;
  maxWidth = width > maxWidth ? width : maxWidth;
  #endif // ERROR_ONLY

  #ifdef DEBUG
  if (lastMid != midpoint) {
    lastMid = midpoint;
    Serial << F("loop: mindpoint ") << midpoint << endl;
  }
  #endif // DEBUG
    
  if (width >= SERVO_MIN) {                                 // Pulse is long enough.    
    return true;
  }
  else {                                                    // Pulse is too short.
    return false;
  }
}

// Error handler. Display error until button is pressed.
void errorHandler(const errNum err, const char state, const bool fatal = false) {
  byte                      i;                              // Index variable.

  lastError = err;                                          // Set last error.

  #ifdef ERROR_ONLY
  ++errCnt[err];
  Serial << F("errorHandler: time ") << millis() << F(" ms, state ") << state;
  Serial << F(", ") << ERROR_TBL[err] << endl;
  displayErrorData();
  #endif // ERROR_ONLY

  // Turn on error LED and return immediately if the error is not fatal.
  if (fatal != true) {
    digitalWrite(ERROR_PIN, HIGH);
    return;
  }
  else {  
    if (err == ERR_NONE) {                                  // Called with no error.
      while (true) {                                        //   Blink rapidly.
      digitalWrite(ERROR_PIN, HIGH);
       if (breakableDelay(100) != true) return;
        digitalWrite(ERROR_PIN, LOW);
        if (breakableDelay(100) != true) return;
      }
    }
    else {                                                  // Called with nonzero error.
      while (true) {
        for (i = 0; i < err; i++) {                         // Blink err number short blinks.
          digitalWrite(ERROR_PIN, HIGH);
          if (breakableDelay(100) != true) return;
          digitalWrite(ERROR_PIN, LOW);
          if (breakableDelay(300) != true) return;
        }
        if (breakableDelay(2000) != true) return;           // Delay after blinks.
      }
    }
  }
}

#ifdef ERROR_ONLY
// Display error counts.
void displayErrorData() {
  byte                      i;                              // Index variable.

  Serial.printf(F("Error counts:\n    "));
  for ( i = 1; i < NUM_ERRORS; i++) {
    Serial.printf(F("%s %11lu"), ERROR_TBL[i], errCnt[i]);
    if (i == NUM_ERRORS - 1) {
      Serial.println('.');
    }
    else {
      Serial.print(F(", "));
    }
  }

  Serial.printf(F("    minWidth %11lu, maxWidth %11lu.\n"), minWidth, maxWidth);
}

// Clear error data.
void clearErrorData() {
  byte                      i;                              // Index variable.

  for ( i = 0; i < NUM_ERRORS; i++) {
      errCnt[i] = 0UL;
  }

  minWidth  = ULONG_MAX;                                  // Min servo pulse width.
  maxWidth  = 0UL;                                        // Max servo pulse width.
}
#endif // ERROR_ONLY
  
/*******************************************************************************************
 ********************************* Interrupt handling methods ******************************
 *******************************************************************************************/

/*******************************************************************************************
 * 
 *
 * breakableDelay() - Delays for the milliseconds passed in delay.
 *
 * breakableDelay() will return immediately if btnPressed is true.
 *
 * Inputs
 *  unsigned long delay  - Delay in milliseconds.
 *
 * Returns:
 *  true  - Delay completed.
 *  false - Key pressed.
 *  
 *******************************************************************************************/

bool breakableDelay(unsigned long delay) {
  unsigned long             endTime = millis() + delay;     // Start time in ms.

  while (millis() < endTime) {
    if (btnPressed == true) {                               // Return immediately with false
      return false;                                         //   if button pressed.
    }
  }
  
  return true;                                              // Return true if delay finished.
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
  Serial << F("buttonISR: free ram ") << freeRam() << endl;
  #endif // FREE_RAM
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
#endif // FREE_RAM


/*
 *  Performance_Test
 *  Test the CPU core perfrormance of the Arduino.
 *  The results are sent to the Serial output at 9600 BAUD.
 */

// Constants.
const unsigned long   NUM_LOOPS   = 100000UL;   // Number of inner loops.
const unsigned long   TST_MAX     =     10UL;   // Number tests before summary.
const int             BUF_SIZ     =      128;   // Text buffer size.

// Global variables.
unsigned long         startTime;                // Start time in ms.
unsigned long         endTime;                  // End time in ms.
unsigned long         execTime;                 // Execution time in ms.
int                   tstLoop;                  // Number of test loops.
unsigned long         tstSum;                   // Summation of test times.
char                  txtBuf[BUF_SIZ];          // Text buffer.

// the setup routine runs once when you press reset:
void setup()
{
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  Serial.println( "Performance_Test Version 1.0" );
  
  // Print the test header.
  sprintf( txtBuf, "Average test time for %d test cycles", TST_MAX );
  Serial.println( txtBuf );
  Serial.println( "------------------------" );
  
  // Make sure Serial data has been sent.
  Serial.flush();
}

// the loop routine runs over and over again forever:
void loop()
{
  // Make variables volatile so optimizer won't delete useless code.
  volatile unsigned long  i;                      // Index variable.
  volatile unsigned long  rslt;                   // Result variable.
  
  // Get starting time in ms.
  startTime = millis();

  for ( tstLoop = 0, tstSum = 0UL; tstLoop < TST_MAX; tstLoop++ )
  {
    // Get starting time in ms.
    startTime = millis();

    for ( i = 0; i < NUM_LOOPS; i++ )
    {
      // Do some math operations to kill time.
      rslt = i * ( i + 63 );
      rslt = rslt / (i + 1);
    }

    endTime = millis();
    execTime = endTime - startTime;
    tstSum += execTime;

    sprintf( txtBuf, "Test %2d: Time %9lu", tstLoop + 1, execTime );
    Serial.println( txtBuf );    
    Serial.flush();
  }

  // Print the average of the test runs.
  Serial.println( "------------------------" );
  sprintf( txtBuf, "Average:      %9lu", tstSum / tstLoop );
  Serial.println( txtBuf );
  
  // Stop after the summary.
  while ( true ) {} // Loop forever until reset.
}




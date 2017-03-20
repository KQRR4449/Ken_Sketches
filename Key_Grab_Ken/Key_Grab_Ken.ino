/*
Updated Sainsmart LCD Button Shield for Arduino
Key Grab Version 2.0
Written by Kenw

Based on:
  Sainsmart LCD Shield for Arduino
  Key Grab v0.2
  Written by jacky
  www.sainsmart.com

Displays the currently pressed key on the LCD screen.

DFR_Key Key Codes (in left-to-right order):

Wait    - SAMPLE_WAIT
None    - NO_KEY
Select  - SELECT_KEY
Left    - LEFT_KEY
Up      - UP_KEY
Down    - DOWN_KEY
Right   - RIGHT_KEY
MAX_KEY - Maximum valid key
*/

#include <LiquidCrystal.h>
#include <DFR_Key.h>

void ftoa(float Value, char* Buffer);
char keyToChar( int keyNum);

// Constants kenw
const double  KEY_REF = 5.0;
const double  AD_TO_V = ((double)1023 / KEY_REF);

//Pin assignments for SainSmart LCD Keypad Shield
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
//---------------------------------------------

DFR_Key keypad;


int   curKey        = -1;
int   newKey;
char  keyBuf[17];               // Key text buffer.
int   keyRaw;                   // Raw analog key.
float keyVolt;                  // Key Voltage.
char  voltBuf[8];               // Key Voltage char string.
int   backlightPin  = 10;       // Backlight control pin.

void setup()
{
  pinMode( backlightPin, OUTPUT );
  digitalWrite( backlightPin, true );

  lcd.begin( 16, 2 );

  lcd.clear();
  lcd.setCursor( 0, 0 );
  lcd.print( "Key Grab v2.0" );
  delay( 2500 );

  lcd.clear();
  lcd.setCursor( 0, 0 );
  lcd.print( "Backlight Off" );
  delay( 2500 );

  digitalWrite( backlightPin, false );
  delay( 2500 );
  digitalWrite( backlightPin, true );

  /*
  OPTIONAL
  keypad.setRate(x);
  Sets the sample rate at once every x milliseconds.
  Default: REFRESH_MIN
  */
  if ( keypad.setRate( REFRESH_MIN ) != ERR_OK )
  {
    lcd.clear();
    lcd.setCursor( 0, 0 );
    lcd.print( "setRate() Error" );
    delay( 2500 );
  }
}

void loop()
{
  /*
   * keypad.getKey();
   * Grabs the current key.
   * Returns a non-zero integer corresponding to the pressed key,
   * OR
   * Returns NO_KEY for no keys pressed,
   * OR
   * Returns SAMPLE_WAIT when no key is available to be sampled.
   */
  newKey = keypad.getKey();

  if (newKey != SAMPLE_WAIT && newKey != curKey)
  {
    curKey = newKey;
    lcd.clear();
    lcd.setCursor( 0, 0 );
    lcd.print( "Current Key:" );
    lcd.setCursor( 0, 1 );
    keyRaw = keypad.getKeyAD();
    keyVolt = (double)keyRaw / AD_TO_V;
    dtostrf( keyVolt, 5, 3, voltBuf );
    sprintf( keyBuf, "%d %c %4d %5sV",
             curKey, keyToChar( curKey ), keyRaw, voltBuf );
    lcd.print( keyBuf );
  }
}

/********************************************************************
 *
 *    keyToChar - Convet key number to key character.
 *
 ********************************************************************
 *
 *    This method converts a numeric key number
 *    returned from keypad.getKey() to a character
 *    representing the key number.
 *
 ********************************************************************/
const char  KEY_CHARS[7] = "NSLUDR";
char        keyToChar( int keyNum )
{
  if ( keyNum >= MIN_KEY && keyNum <= MAX_KEY ) {
    // Return a valid key character.
    return KEY_CHARS[ keyNum ];
  }
  else {
    // Return the invalid key '?'.
    return '?';
  }
}


/***********************************************************************************************
 *
 * This sketch tests the Serial.printf() method added to print.h in Arduino core.
 * A copy of the methods to add to print.h are contained in this sketch long with
 * tests of the Serial.printf() function.
 * 
 ***********************************************************************************************
 */

#ifdef BLOMP_ORIG
#include <stdarg.h>
#define PRINTF_BUF 80 // define the tmp buffer size (change if desired)
   void printf(const char *format, ...)
   {
   char buf[PRINTF_BUF];
   va_list ap;
        va_start(ap, format);
        vsnprintf(buf, sizeof(buf), format, ap);
        for(char *p = &buf[0]; *p; p++) // emulate cooked mode for newlines
        {
                if(*p == '\n')
                        write('\r');
                write(*p);
        }
        va_end(ap);
   }
#ifdef F // check to see if F() macro is available
   void printf(const __FlashStringHelper *format, ...)
   {
   char buf[PRINTF_BUF];
   va_list ap;
        va_start(ap, format);
#ifdef __AVR__
        vsnprintf_P(buf, sizeof(buf), (const char *)format, ap); // progmem for AVR
#else
        vsnprintf(buf, sizeof(buf), (const char *)format, ap); // for the rest of the world
#endif
        for(char *p = &buf[0]; *p; p++) // emulate cooked mode for newlines
        {
                if(*p == '\n')
                        write('\r');
                write(*p);
        }
        va_end(ap);
   }
#endif
#endif // BLOMP_ORIG

// Uncomment next line to test local printf copy.
#define BLOMP_KGW
#ifdef  BLOMP_KGW
class Printf_KGW : public Print
{
public:
  virtual size_t write(uint8_t var) {
    return Serial.write(var);
  }
  
    // KGW Start
    // http://playground.arduino.cc/Main/Printf
    #include <stdarg.h>
    #define PRINT_HAS_PRINTF true
    static const size_t PRINTF_LENGTH = 79; // Maximum printf length not counting \0 (change if desired).
        
    int printf(const char *format, ...)
    {
      int       rtn;
      char      buf[PRINTF_LENGTH + 1];
      va_list   ap;
      va_start(ap, format);
      rtn = vsnprintf(buf, sizeof(buf), format, ap);
      for(char *p = &buf[0]; *p; p++) // emulate cooked mode for newlines
      {
        if(*p == '\n')
        {
          write('\r');
        }
        write(*p);
      }
      va_end(ap);
      return rtn <= PRINTF_LENGTH ? rtn : -1;
    }
    
    #ifdef F // check to see if F() macro is available
    int printf(const __FlashStringHelper *format, ...)
    {
      int       rtn;
      char      buf[PRINTF_LENGTH + 1];
      va_list   ap;
      va_start(ap, format);
      #if defined(__AVR__) || defined(ESP8266)
      rtn = vsnprintf_P(buf, sizeof(buf), (const char *)format, ap);  // progmem for AVR
      #else
      rtn = vsnprintf(buf, sizeof(buf), (const char *)format, ap);    // for the rest of the world
      #endif // __AVR__
      for(char *p = &buf[0]; *p; p++) // emulate cooked mode for newlines
      {
        if(*p == '\n')
        {
          write('\r');
        }
        write(*p);
      }
      va_end(ap);
      return rtn <= PRINTF_LENGTH ? rtn : -1;
    }
   #endif // F
   // KGW End

};
#endif // BLOMP_KGW

const byte          VER_MAJOR       =     1;    // Major version.
const byte          VER_MINOR       =     1;    // Minor version.

#ifdef BLOMP_KGW
Printf_KGW          printfKGW;
#endif // BLOMP_KGW

void setup() {
  int               rtn;                        // Return value.
  
  Serial.begin(115200);
  #ifdef ESP8266
  delay(100);
  Serial.println();
  #endif // ESP8266
  Serial.printf(F("Starting Printf_Test version %d.%d, Build date %s %s\n"),  VER_MAJOR,
                                                                                VER_MINOR,
                                                                                __DATE__,
                                                                                __TIME__);
  #ifdef PRINT_HAS_PRINTF 
  Serial.printf(F("PRINT_HAS_PRINTF defined. Value %s.\n"),
                      PRINT_HAS_PRINTF ? "true" : "false");
  #endif // PRINT_HAS_PRINTF
  Serial.printf("PRINTF_LENGTH = %d\n", Print::PRINTF_LENGTH );

  // Test local printf() copy.
  #ifdef BLOMP_KGW
  rtn = printfKGW.printf(  "printfKGW RAM string strlen 31\n");
  Serial.printf("printfKGW RAM rtn = %d\n", rtn);
  rtn = printfKGW.printf(F("printfKGW PGM string strlen 31\n"));
  Serial.printf("printfKGW PGM rtn = %d\n", rtn);
  #endif // BLOMP_KGW
  
  Serial.printf(  "printf RAM: Verison %d.%d\n",  VER_MAJOR, VER_MINOR);
  Serial.printf(F("printf PGM: Verison %d.%d\n"), VER_MAJOR, VER_MINOR);
  
  rtn = Serial.printf("123456789");
  Serial.printf("\n    rtn = %d\n", rtn);
  //                   0        1         2         3         4         5         6         7         8  
  rtn = Serial.printf("OK 4567890123456789012345678901234567890123456789012345678901234567890123456789");
  Serial.printf("\n    rtn = %d\n", rtn);
  rtn = Serial.printf("LONG 678901234567890123456789012345678901234567890123456789012345678901234567890");
  Serial.printf("\n    rtn = %d\n", rtn);

  Serial.printf(F("Ending setup. millis() = %lu\n"), millis());
}

void loop() {}



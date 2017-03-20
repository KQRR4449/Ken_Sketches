#include <Streaming.h>
#include <StreamingW.h>
#include <float.h>

//#define DEBUG                                           // Define for dbugging.

const signed char     ZERO_EXP      =             -127; // Float 0 exponent.
const signed char     NAN_EXP       =              128; // Float NAN exponent.

union FloatUl {
  float         fl;
  unsigned long ul;
};

void printCharLine(size_t charCnt = 80U, char c = '-');
void buildFloat(bool isPos, signed char exp, unsigned long sig, FloatUl &inF);

//_MAX_FLOAT_P  4294967040.00
//_MAX_FLOAT_L  4294967040    0xFFFFFF00 1111 1111 1111 1111 1111 1111 0000 0000
//                                       1234 5678 9012 3456 7890 1234 5678 9012
//                                       0          1           2            3
//1.1111111111111111111111100000000 base 2 raised to the 31st power base 2
//                                  or 0x00FFFFFF << 8

void setup() {
  const float           _MAX_FLOAT_P  =   4294967040.0;
  const  unsigned long  _MAX_FLOAT_UL =   (unsigned long)_MAX_FLOAT_P;

  float               flt             =  1.0 + 1.0/3.0; // 1/3 float variable.
  float               f;
  FloatUl             fTmp;                             // Temp variable.
  unsigned long       u;

  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  Serial.println(F("Starting test program"));

  Serial << F("FLT_MANT_DIG ") << FLT_MANT_DIG << F(" FLT_DIG ") << FLT_DIG << endl;
  Serial << F("FLT_MIN_EXP ") << FLT_MIN_EXP << F(" FLT_MAX_EXP ") << FLT_MAX_EXP << endl;

  printCharLine();
  Serial << F(" _MAX_FLOAT_P ") << _FLOATW(_MAX_FLOAT_P, 2, 14) << endl;
  Serial << F(" _MAX_FLOAT_L ") << _ULONGW(_MAX_FLOAT_UL,   11);
  Serial << F("    0x")         << _HEXW(  _MAX_FLOAT_UL,    8, '0');
  Serial << F(" ")              << _BINW(  _MAX_FLOAT_UL,   32, '0') << endl;
  u = 0xFFFFFF << 8;
  Serial << F("0xFFFFFF << 8 ") << _ULONGW(            u,   11);
  Serial << F("    0x")         << _HEXW(              u,    8, '0');
  Serial << F(" ")              << _BINW(              u,   32, '0') << endl;

  printFloat( 0.0,                "0.0");
  printFloat(-0.0,               "-0.0");
  printFloat( 1.0,                "1.0");
  printFloat(-1.0,               "-1.0");
  printFloat( 0.5,                "0.5");
  printFloat( 0.25,              "0.25");
  printFloat(FLT_MAX,         "FLT_MAX");
  printFloat(FLT_MIN,         "FLT_MIN");
  printFloat(FLT_EPSILON, "FLT_EPSILON");
  printFloat(_MAX_FLOAT_P,      "FLT_P");
  printFloat( 1.0/0.0,        "1.0/0.0");
  printFloat( -1.0/0.0,      "-1.0/0.0");
  printFloat( 12.375,          "12.375");
  buildFloat(true,         3, 0x460000UL, fTmp);
  printFloat(  fTmp.fl,  "build 12.375");

  buildFloat(true,  ZERO_EXP, 0x000000UL, fTmp);
  printFloat(  fTmp.fl,    "build +0.0");
  buildFloat(false, ZERO_EXP, 0x000000UL, fTmp);
  printFloat(  fTmp.fl,    "build -0.0");
  buildFloat(false, ZERO_EXP, 0x000001UL, fTmp);
  printFloat(  fTmp.fl,    "build denormal");
  buildFloat(true,  NAN_EXP,  0x000001UL, fTmp);
  printFloat(  fTmp.fl,     "build nan");
  buildFloat(true,  NAN_EXP,  0x000000UL, fTmp);
  printFloat(  fTmp.fl,    "build infinity");
  buildFloat(true,      127,  0x7FFFFFUL, fTmp);
  printFloat(  fTmp.fl,     "build 127");
  buildFloat(true,      126, 0x7FFFFFUL, fTmp);
  printFloat(  fTmp.fl,     "build 126");
  buildFloat(true,        1, 0x000000UL, fTmp);
  printFloat(  fTmp.fl,       "build 1");
  buildFloat(true,        0, 0x000000UL, fTmp);
  printFloat(  fTmp.fl,       "build 0");
  buildFloat(true,       -1, 0x000000UL, fTmp);
  printFloat(  fTmp.fl,      "build -1");
  buildFloat(true,     -125, 0x000000UL, fTmp);
  printFloat(  fTmp.fl,    "build -125");
  buildFloat(true,     -126, 0x000000UL, fTmp);
  printFloat(  fTmp.fl,    "build -126");

}

void loop() {
  // put your main code here, to run repeatedly:

}

// Print float and unsigned long union of the float.
void printFloat(float f, const char *idStr) {
  FloatUl                   flUl;                       // Union to hold float bits.
  char                      sign;                       // Sign of f.
  byte                      exShift;                    // Exponent before substraction.
  signed char               exp;                        // Exponent.
  unsigned long             sig;                        // Significand.
  const __FlashStringHelper *msg;                       // Float tyep message.
  flUl.fl = f;
  
  printCharLine();
  Serial << _PCHARW(   idStr,    16) << ' ' << _FLOATW(f,       8, 20);
  Serial << F(" float ")                    << _FLOATW(flUl.fl, 8, 20);
  Serial << F(" ul 0x")                     << _HEXW(  flUl.ul,     8, '0') << endl;

  sign    =   flUl.ul &  0x80000000 ? '-' : '+';
  exShift = (flUl.ul & ~0x80000000) >> 23;
  exp     = exShift - 127;
  sig     =   flUl.ul &  0x007FFFFF;

  // Set msg to show type of float.
  if (exp == -127) {
    if (sig == 0UL) {
      // Value is zero.
      msg = F("zero");
    }
    else {
      // Value is denormal.
      msg =  F("denormal");
    }
  }
  else if (exp == -128) {
    if (sig == 0UL) {
      msg = F("infinity");
    }
    else {
      msg = F("not a number");
    }
  }
  else {
    msg = F("normalized");
  }

  Serial << F("sign ")  << sign;
  Serial << F(" exp ")  << _INTW(exp, 5) << F(" sig 0x") << _HEXW(sig, 6, '0');
  Serial << F(" 1.")    << _BINW(sig, 23, '0');
  Serial << F(" type ") << _PCHARFW(msg, 12);
  Serial.println();
}

// Print a line of characters.
void printCharLine(size_t charCnt = 80U, char c = '-') {
  while (charCnt-- > 0U) {
    Serial.write(c);
  }
  Serial.println();
}

// Build a float from unsigned long union.
void buildFloat(bool isPos, signed char exp, unsigned long sig, FloatUl &inF) {
  byte                tmpB;                             // Temp byte;
  unsigned long       tmpUl;                            // Temp unsigned long.
  
  // Set the sign bit.
  if (isPos ==  true) {
    // Sign is positive.
    inF.ul = 0UL;
  }
  else {
    // Sign is negative.
    inF.ul = 0x80000000UL;
  }

  tmpB  = exp + 127;
  tmpUl = (unsigned long)tmpB;
  
  // Set the exponent.
  inF.ul |= (tmpUl & 0xFF) << 23;

  #ifdef DEBUG
  printCharLine(80, 'X');
  Serial << F("tmpB 0x") << _HEXW(tmpB, 2, '0') << F(", tmpUL 0x") << _HEXW(tmpUl, 8, '0');
  Serial << F(" shif 23 ") << _BINW( inF.ul, 32, '0');
  Serial.println();
  #endif // DEBUG

  // Set the significand.
  inF.ul |= sig & 0x007FFFFF;
}




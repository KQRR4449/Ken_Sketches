template <class T>
class MyPair {
  private:
    T values [2];
    
  public:
    MyPair(T first, T second)
    {
      values[0]=first; values[1]=second;
    }

    T getVal(byte i) { return values[i]; }

    T& operator[] (byte i) {
          return values[i];
      }
};

// This template creates struct WrapInt with N set to a constanst value.
// Handy to subclass a class with one or more constant values.
template <int N> class WrapInt
{
  public:
    WrapInt() {
      ++_cnt;
      Serial.printf(F("Ctor <%d>WrapInt() Cnt = %d\n"), N, _cnt);
    }

    int getCnt(void) { return _cnt; }

  protected:
    static int _cnt;
};

template< int N >
int WrapInt<N>::_cnt = 0; // initializer is optional//template <int N>


template <class T, int N> struct WrapT
{
  T   val;
  int iVal;

  WrapT( T v, int w) : val(v), iVal(w)
  {
    Serial.print("WrapT<");
    Serial.print(val);
    Serial.print(", ");
    Serial.print(N);
    Serial.print("> iVal = ");
    Serial.println(iVal);
  }
};
// Note: template<> and method declaration must be on the same line.
template <typename T> void mySwap(T& a, T& b) {  
   T c(a);   
   a = b;   
   b = c;  
}

template <typename myType> myType getMax(myType a, myType b) {
 return (a>b?a:b);
}

// Streaming << template for all objects derived from the Print class.
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

MyPair <int> thePair(3, 4);

void setup() {
    Serial.begin(9600);
    Serial.print("getMax(2, 1) ");
    Serial.println(getMax(2, 1));
    int i = 1;
    int j = 2;
    mySwap(i, j);
    Serial.printf("mySwap(1, 2) = %d, %d\n", i, j);
    Serial.printf("thePair getVal(0) getVal{1) = %d, %d\n", thePair.getVal(0), thePair.getVal(1));
    Serial.printf("thePair [] overload = %d, %d\n", thePair[0], thePair[1]);
    Serial << "<< operator " << 1 << "\n";
    Serial.println("Next line");
    Serial.println(4, 3);

    // Create custom struct with N preset.
    WrapInt<0> myX0;
    Serial.printf(F("myX0 Cnt = %d\n"), myX0.getCnt());
    WrapInt<1> myX1;

    // Test WrapT<class T, int N>
    WrapT<long, 4>myWrapl4(-1L, 3);

}

void loop() {
  // put your main code here, to run repeatedly:

}

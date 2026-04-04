#include<stdbool.h>
#include<stdint.h>
//First 100 digits of famous constants
#define pi "3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679"
#define e  "2.7182818284590452353602874713526624977572470936999595749669676277240766303535475945713821785251664274"

typedef struct{
    char* Digits; //absolute value of the number
    unsigned int NrOfDigits;  //size
    bool IsNegative; //true if it is negative 
}BigNumber;

typedef struct{
   BigNumber *Mantissa; //significand
   int Exponent; 
}BigFloatNumber;  // BigFloatNumber= significand* 10^Exponent ;Exemple: 123.45= 12345*10^(-2)

BigFloatNumber *FromBigNumber(BigNumber *Number); //Conversion from Int to Float

BigNumber* Init(char* value); //Construct a BigNum from a string
BigFloatNumber *InitFloat(char *value); //Construct a BigFloatNum from a string
void FreeMemory(BigNumber *Number);  //Destroys the BigNumber
void FreeMemoryFloat(BigFloatNumber *Number); //Destroys the BigFloatNumber

BigNumber* Sum(BigNumber* Number1, BigNumber* Number2);  //Number1+Number2
BigNumber* Subtract(BigNumber* Number1, BigNumber* Number2); //Number1-Number2
BigNumber* Multiply(BigNumber* Number1, BigNumber* Number2); //Number1*Number2
BigFloatNumber *MultiplyFloat(BigFloatNumber* Number1,BigFloatNumber *Number2);
BigNumber* LongDivision(BigNumber* Dividend, BigNumber* Divisor,BigNumber* Remainder); // Divident/Divizor
BigNumber* Power(BigNumber*Number,BigNumber *Power); //Return a new BIGINT equal to Number^Power , if Power is negative return 0, if power is Positive apply Exponentiation by squaring algoritm
BigNumber* Modulo(BigNumber * Dividend, BigNumber *Modulus);  // Divident mod Modulus 

BigNumber* FromUnsignedIntegerToBigNum(unsigned int number); //Construct a BigINT from an unsigned integer
BigNumber* FromSignedIntegerToBigNum(int number); //Construct a BigINT from an signed integer
BigNumber* FromUnsignedInt128ToBigNum(__uint128_t Number); //Construct a BigINT from an uint128
BigNumber* Factorial(unsigned int Number); //Calculates the factorial of an unsigned integer {(n+1)!=(n+1)*n! ,0!=1}

BigFloatNumber* MultiplyFloat(BigFloatNumber* Number1,BigFloatNumber *Number2); //Number1*Number2
BigFloatNumber* SumFloat(BigFloatNumber* Number1,BigFloatNumber* Number2); //Number1+Number2
BigFloatNumber* SubtractFloat(BigFloatNumber* Number1, BigFloatNumber* Number2); //Number1-Number2
BigFloatNumber* DivizionSetPrecision(BigFloatNumber *Divident,BigFloatNumber *Divizor, unsigned int precision); //Number1/Number2 where Precision means number of decimal digits that the user wants
BigFloatNumber* PowerFloat(BigFloatNumber *Number,BigFloatNumber *Power,unsigned int precision);  //Number^Power where Precision means number of decimal digits that the user wants if powers is not a natural number
BigFloatNumber* SquareRoot(BigFloatNumber* Number, unsigned int precision); //Gives sqrt(Number) with precision digits
BigFloatNumber* InverseSquareRoot(BigFloatNumber* Number, unsigned int precision);  //Gives 1/sqrt(Number) with precision digits
BigFloatNumber* Inverse(BigFloatNumber *Number,unsigned int precision);// Gives 1/Number with precision number of digits

int BigNumberCompare(BigNumber* Number1, BigNumber* Number2); //return 1 if Number1>Number2, -1 if Number1<Number2 , 0 if  Number1==Number2
int BigNumberCompareAbsoluteValue(BigNumber* Number1, BigNumber* Number2);  //return 1 if |Number1|>|Number2|, -1 if |Number1|<|Number2| , 0 if  |Number1|==|Number2|

void Increment(BigNumber* Number); //Increments the value by 1
void RoundFloat(BigFloatNumber *Number, unsigned int Precision); //Modifies the number in memory by rounding to (precision) number of digits 

bool IsEven(BigNumber *Number); //return true if the number is even by evaluating the first digit
bool IsOdd(BigNumber *Number); //return true if the number is odd by returning opposite of IsEven
bool IsNegative(BigNumber *Number); //return true if the number is a negative BIGINT
bool IsEqual(BigNumber *Number1,BigNumber *Number2); //returns true if there are equal, false otherwise

char *ToString(BigNumber *Number); //Converts a BigNumber to a String
void PrintBigNumber(BigNumber *Number);  //Prints the BigNumber
void PrintBigFloatNumber(BigFloatNumber *Number); //Prints the BigFloatNumber

#if defined(__SIZEOF_INT128__) 
    BigNumber* FromUnsignedLongLongToBigNum(unsigned long long int Number); //Construct a BigINT from an unsigned long long int integer #endif
#endif 
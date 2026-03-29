#include<stdbool.h>

typedef struct{
    char* Digits; //absolute value of the number
    unsigned int NrOfDigits;  //size
    bool IsNegative; //true if it is negative 
}BigNumber;

BigNumber* Init(char* value); //Construct a BigNum from a string
void FreeMemory(BigNumber *Number);  //Destroys the BigNumber

BigNumber* Sum(BigNumber* Number1, BigNumber* Number2);  //Number1+Number2
BigNumber* Subtract(BigNumber* Number1, BigNumber* Number2); //Number1-Number2
BigNumber* Multiply(BigNumber* Number1, BigNumber* Number2); //Number1*Number2
BigNumber* LongDivision(BigNumber* Dividend, BigNumber* Divisor,BigNumber* Remainder); // Divident/Divizor
BigNumber* FromUnsignedIntegerToBigNum(unsigned int number); 
BigNumber* FromSignedIntegerToBigNum(int number);
BigNumber* Factorial(unsigned int Number); //Calculates the factorial of an unsigned integer {(n+1)!=(n+1)*n! ,0!=1}

int BigNumberCompare(BigNumber* Number1, BigNumber* Number2); //return 1 if Number1>Number2, -1 if Number1<Number2 , 0 if  Number1==Number2
int BigNumberCompareAbsoluteValue(BigNumber* Number1, BigNumber* Number2);  //return 1 if |Number1|>|Number2|, -1 if |Number1|<|Number2| , 0 if  |Number1|==|Number2|

void Increment(BigNumber* Number); //Increments the value by 1

char *ToString(BigNumber *Number); //Converts a BigNumber to a String
void PrintBigNumber(BigNumber *Number);  //Prints the BigNumber

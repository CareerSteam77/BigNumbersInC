typedef struct{
    char* Digits;
    unsigned int NrOfDigits;
}BigNumber;

BigNumber* Init(char* value);
BigNumber* Sum(BigNumber* Number1, BigNumber* Number2);
BigNumber* Multiply(BigNumber* Number1, BigNumber* Number2);
BigNumber* FromUnsignedIntegerToBigNum(unsigned int number);
int BigNumberCompare(BigNumber* Number1, BigNumber* Number2);
BigNumber* Factorial(unsigned int Number);
char *ToString(BigNumber *Number);
void PrintBigNumber(BigNumber *Number);
void FreeMemory(BigNumber *Number);
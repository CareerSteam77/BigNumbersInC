#include "BigNumber.h"
#include "BigNumberCriptography.h"
#include <stdio.h>
#define Size 100
#define PrimeSize 250
int main(void)
{
    BigNumber* Nr1=Init("2550000123123999434341231405999999999999999999999999999999999999999999999999999922222222222222");
    BigNumber* Nr2=Init("12345678910987654321");
    BigNumber* Nr3=Init("2322986111");
    BigNumber* NrRand=GenerateRandomPositiveBigNumber(Size);
    BigNumber* NrRandPrime=GeneratePrime(PrimeSize);
    BigNumber* Nr4=ModularExponetiation(Nr1,Nr2,Nr3);
    BigNumber* Nr5=GCD(Nr1,Nr2);
    BigNumber* Nr6=LCM(Nr1,Nr2);
    BigNumber* X; BigNumber* Y;BigNumber* Nr7=ExtendedEuclidean(Nr1,Nr2,&X,&Y);

    printf("After Initialization INT\n");

    printf("Random Positive BigNumber of size %d is ",Size);PrintBigNumber(NrRand);printf("\n");
    printf("Random Prime BigNumber of size %d is ",PrimeSize);PrintBigNumber(NrRandPrime);printf("\n");
    printf("Nr3 is prime:%s",MillerRabin(Nr3,30)?"true":"false");printf("\n");
    printf("Nr1 is:");PrintBigNumber(Nr1);printf("\n");
    printf("Nr2 is:");PrintBigNumber(Nr2);printf("\n");
    printf("Nr3 is:");PrintBigNumber(Nr3);printf("\n");
    printf("(Nr1^Nr2) mod Nr3 is:");PrintBigNumber(Nr4);printf("\n");
    printf("Steins Binary Algorithm Result Gcd(N1,Nr2) is:");PrintBigNumber(Nr5);printf("\n");
    printf("Extended Euclidian Result (with Bezout Coeff) is:");PrintBigNumber(Nr7);PrintBigNumber(X);PrintBigNumber(Y);printf("\n");
    printf("lcm(Nr1,Nr2) is :");PrintBigNumber(Nr6);

    FreeMemory(NrRand);
    FreeMemory(Nr1);
    FreeMemory(Nr2);
    FreeMemory(Nr3);
    FreeMemory(Nr4);
    FreeMemory(Nr5);
    FreeMemory(Nr6);
    FreeMemory(Nr7);FreeMemory(X);FreeMemory(Y);

    printf("After Cleanup\n");
}
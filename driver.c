#include<stdio.h>
#include "BigNumber.h"
int main(int argc,char **argv)
{
    //A test driver for BigNumber;
    BigNumber* Nr1=Init("1213434534534534534534445645645634534597349573485734583745834758934579384579834579287489123734823573984753846783095720934739478528347208347398472034720183472985728347203472835720384");
    BigNumber* Nr2=Init("11");
    BigNumber* Nr3=Sum(Nr1,Nr2);
    BigNumber* Nr4=Subtract(Nr1,Nr2);
    BigNumber* Nr5=FromUnsignedIntegerToBigNum(1234567);
    BigNumber* Nr6=Multiply(Nr1,Nr2);
    BigNumber* Remainder=Init("1");
    BigNumber* Nr7=LongDivision(Nr1,Nr2,Remainder);

    BigFloatNumber* NrF1=InitFloat(pi);
    BigFloatNumber* NrF2=InitFloat(e);
    BigFloatNumber* NrF4=FromBigNumber(Nr1);
    BigFloatNumber* NrF3=MultiplyFloat(NrF1,NrF2);
    BigFloatNumber* NrF5=SumFloat(NrF1,NrF2);
    BigFloatNumber* NrF6=SubtractFloat(NrF1,NrF2);
    PrintBigFloatNumber(NrF4);printf("\n");
    printf("pi:");PrintBigFloatNumber(NrF1);printf("\n");
    printf("e:");PrintBigFloatNumber(NrF2);printf("\n");
    printf("pi*e:");PrintBigFloatNumber(NrF3);printf("\n");
    printf("pi+e:");PrintBigFloatNumber(NrF5);printf("\n");
    printf("pi-e:");PrintBigFloatNumber(NrF6);printf("\n");
    printf("After Initialization\n");

    printf("FirstNumber:");PrintBigNumber(Nr1);printf("\n");
    printf("SecondNumber:");PrintBigNumber(Nr2);printf("\n");
    printf("Sum:");PrintBigNumber(Nr3);printf("\n");
    printf("Subtract:");PrintBigNumber(Nr4);printf("\n");
    printf("Product:");PrintBigNumber(Nr6);printf("\n");
    printf("Quotient:");PrintBigNumber(Nr7);printf("\n");
    printf("Remainder:");PrintBigNumber(Remainder);printf("\n");
  
    printf("After Printing the Numbers\n");

   // printf("COMPARE:%d \n",BigNumberCompare(Nr1,Nr2));
   // printf("Product and Sum of :\n%s\nand\n%s\nis\n%s\n%s\n",ToString(Nr1),ToString(Nr2),ToString(Nr3),ToString(Nr5));
    //printf("Factorial of 3000 is :\n%s\n",ToString(Factorial(3000)));
    FreeMemory(Nr1);
    FreeMemory(Nr2);
    FreeMemory(Nr3);
    FreeMemory(Nr4);
    FreeMemory(Nr5);
    FreeMemory(Nr6);
    FreeMemory(Nr7);

    FreeMemoryFloat(NrF1);
    FreeMemoryFloat(NrF2);
    FreeMemoryFloat(NrF3);
    FreeMemoryFloat(NrF4);
    FreeMemoryFloat(NrF5);
    printf("After Memory Free\n");
    return 0;
}
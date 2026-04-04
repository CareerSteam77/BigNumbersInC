#include<stdio.h>
#include "BigNumber.h"
int main(int argc,char **argv)
{
    //A test driver for BigNumber;
    BigNumber* Nr1=Init("121");
    BigNumber* Nr2=Init("6");
    BigNumber* Nr3=Sum(Nr1,Nr2);
    BigNumber* Nr4=Subtract(Nr1,Nr2);
    BigNumber* Nr5=FromUnsignedIntegerToBigNum(2);
    BigNumber* Nr6=Multiply(Nr1,Nr2);
    BigNumber* Remainder=Init("1");
    BigNumber* Nr7=LongDivision(Nr1,Nr2,Remainder);
    BigNumber *Nr8=Power(Nr1,Nr2);
    BigNumber* Nr9=Modulo(Nr1,Nr2);

    printf("After Initialization INT\n");


    BigFloatNumber* NrF1=InitFloat(pi);
    BigFloatNumber* NrF2=InitFloat(e);
    BigFloatNumber* NrF4=FromBigNumber(Nr1);
    BigFloatNumber* NrF3=MultiplyFloat(NrF1,NrF2);
    BigFloatNumber* NrF5=SumFloat(NrF1,NrF2);
    BigFloatNumber* NrF6=SubtractFloat(NrF1,NrF2);
    BigFloatNumber* NrF7=DivizionSetPrecision(NrF1,NrF2,3);
    BigFloatNumber* NrF8=PowerFloat(NrF1,NrF2,30);
    BigFloatNumber* NrF9=SquareRoot(NrF1,30);
    BigFloatNumber* NrF10=InverseSquareRoot(NrF1,30);
    BigFloatNumber* NrF11=Inverse(NrF1,30);

    printf("After Initialization FLOAT\n");

    PrintBigFloatNumber(NrF4);printf("\n");
    printf("pi:");PrintBigFloatNumber(NrF1);printf("\n");
    printf("e:");PrintBigFloatNumber(NrF2);printf("\n");
    printf("pi*e:");PrintBigFloatNumber(NrF3);printf("\n");
    printf("pi+e:");PrintBigFloatNumber(NrF5);printf("\n");
    printf("pi-e:");PrintBigFloatNumber(NrF6);printf("\n");
    printf("pi/e:");PrintBigFloatNumber(NrF7);printf("\n");
    printf("pi^e:");PrintBigFloatNumber(NrF8);printf("\n");
    printf("sqrt(pi):");PrintBigFloatNumber(NrF9);printf("\n");
    printf("1/sqrt(pi):");PrintBigFloatNumber(NrF10);printf("\n");
    printf("1/pi:");PrintBigFloatNumber(NrF11);printf("\n");


    printf("FirstNumber:");PrintBigNumber(Nr1);printf("Is even?:%d",IsEven(Nr1));printf("\n");
    printf("SecondNumber:");PrintBigNumber(Nr2);printf("Is odd?:%d",IsOdd(Nr2));printf("\n");
    printf("Sum:");PrintBigNumber(Nr3);printf("\n");
    printf("Subtract:");PrintBigNumber(Nr4);printf("\n");
    printf("Product:");PrintBigNumber(Nr6);printf("\n");
    printf("Quotient:");PrintBigNumber(Nr7);printf("\n");
    printf("Remainder from Divizion:");PrintBigNumber(Remainder);printf("\n");
    printf("Power:");PrintBigNumber(Nr8);printf("NrOfDigits %d",Nr8->NrOfDigits);printf("\n");
    printf("Remainder from Modulo:");PrintBigNumber(Nr9);printf("\n");
  
    printf("After Printing the Numbers \n");

   // printf("COMPARE:%d \n",BigNumberCompare(Nr1,Nr2));
    //printf("Product and Sum of :\n%s\nand\n%s\nis\n%s\n%s\n",ToString(Nr1),ToString(Nr2),ToString(Nr3),ToString(Nr5));
    //printf("Factorial of 3000 is :");PrintBigNumber(Factorial(3000));
    FreeMemory(Nr1);
    FreeMemory(Nr2);
    FreeMemory(Nr3);
    FreeMemory(Nr4);
    FreeMemory(Nr5);
    FreeMemory(Nr6);
    FreeMemory(Nr7);
    FreeMemory(Nr8);
    FreeMemory(Nr9);

    FreeMemoryFloat(NrF1);
    FreeMemoryFloat(NrF2);
    FreeMemoryFloat(NrF3);
    FreeMemoryFloat(NrF4);
    FreeMemoryFloat(NrF5);
    FreeMemoryFloat(NrF6);
    FreeMemoryFloat(NrF7);
    FreeMemoryFloat(NrF8);
    FreeMemoryFloat(NrF9);
    FreeMemoryFloat(NrF10);
    FreeMemoryFloat(NrF11);
    
    printf("After Memory Free\n");
    return 0;
}
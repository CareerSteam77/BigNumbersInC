#include<stdio.h>
#include "BigNumber.h"
int main(int argc,char **argv)
{
    //A test driver for BigNumber;
    BigNumber* Nr1=Init("12345678998765432112345678998765432100110011000010");
    BigNumber* Nr2=Init("12345678998765432112345678998765432100110011000010");
    BigNumber* Nr3=Sum(Nr1,Nr2);
    BigNumber* Nr4=FromUnsignedIntegerToBigNum(1234567);
    BigNumber* Nr5=Multiply(Nr1,Nr2);
    printf("After Initialization\n");

    //PrintBigNumber(Nr1);
   // PrintBigNumber(Nr2);
    //PrintBigNumber(Nr3);
    //PrintBigNumber(Nr4);
   // PrintBigNumber(Nr5);
    printf("After Printing the Numbers\n");

    printf("COMPARE:%d \n",BigNumberCompare(Nr1,Nr2));
    printf("Product and Sum of :\n%s\nand\n%s\nis\n%s\n%s\n",ToString(Nr1),ToString(Nr2),ToString(Nr3),ToString(Nr5));
    printf("Factorial of 3000 is :\n%s\n",ToString(Factorial(3000)));
    FreeMemory(Nr1);
    FreeMemory(Nr2);
    FreeMemory(Nr3);
    FreeMemory(Nr4);
    FreeMemory(Nr5);
    printf("After Memory Free\n");
    //printf("\n%d",BigNumberCompare(Nr1,Nr2));
    return 0;
}
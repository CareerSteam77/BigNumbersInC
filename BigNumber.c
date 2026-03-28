#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<stdbool.h>
#include<ctype.h>

typedef struct{
    char* Digits;  //Digits are stored in reverse order for easier arithmetic operations
    unsigned int NrOfDigits;  //Numbers can have a max number of digits of 4,294,967,295 (max value of unsigned int)
}BigNumber;

bool VerifyStringIsNumber(const char *Value)
{
    if(Value==NULL)
       return false;
    for(unsigned int index=0;index<strlen(Value);index++)
       {
        if(isdigit(Value[index]==false))
          return false;
       }
    return true;
}

BigNumber* Init(const char* Value)   //Takes a string, checks if it is NULL or NAN and construct a BigNumber
{
    if(VerifyStringIsNumber(Value)==false)
      {
        return NULL;
      }

    BigNumber* Number=malloc(sizeof(BigNumber));
    if(Number==NULL)
      {
         perror("Memory Allocation for BigNumber failed");
        exit(-1);
      }
    
    Number->NrOfDigits=strlen(Value);
    Number->Digits=malloc(sizeof(char)*(Number->NrOfDigits+1));
    if(Number->Digits==NULL)
      {
        perror("Memory Allocation for BigNumber Digits failed");
        exit(-1);
      }
    
    for(unsigned int index=0;index<Number->NrOfDigits;index++)
      {
        Number->Digits[index]=Value[Number->NrOfDigits-index-1]; //Reverse the string into Digits
      }
    Number->Digits[Number->NrOfDigits]='\0';
    return Number;
}

BigNumber *PrivateConstructor(char *Digits,unsigned int NrOfDigits)  //Construct a BigNumber WITHOUT reversing the string, used in Arithemic operations
{
   BigNumber* Number=malloc(sizeof(BigNumber));
   if(Number==NULL)
      {
        perror("Memory Allocation for BigNumber failed");
        exit(-1);
      }
    
    Number->NrOfDigits=NrOfDigits;
    Number->Digits=Digits;
    
    return Number;
}

int BigNumberCompare(BigNumber* Number1, BigNumber* Number2)  //return 1 if Number1>Number2, -1 if Number1<Number2 , 0 if  Number1==Number2
{
   if(Number1->NrOfDigits>Number2->NrOfDigits)  
        return 1;
   if(Number1->NrOfDigits<Number2->NrOfDigits)
        return -1;
    
   unsigned int Index=0;
   for(Index=0;Index<Number1->NrOfDigits;Index++)
     {
        if(Number1->Digits[Number1->NrOfDigits-Index-1] > Number2->Digits[Number2->NrOfDigits-Index-1])
          return 1;
        if(Number1->Digits[Number1->NrOfDigits-Index-1] < Number2->Digits[Number2->NrOfDigits-Index-1])
          return -1;
     }
    
    return 0;
}

BigNumber* Sum(BigNumber* Number1, BigNumber* Number2) 
{
    unsigned int MaxSize=0;
    if(Number1->NrOfDigits>=Number2->NrOfDigits)
      MaxSize=Number1->NrOfDigits;
    else
      MaxSize=Number2->NrOfDigits;

    char *RezultSumString=malloc(sizeof(char)*(MaxSize+2)); //PrivateConstructor will take ownership of the memory
    unsigned int NrOfDigits=MaxSize;
    unsigned int index=0;
    short int carry=0;
    short int curent_digit=0;
    for(index=0;index<MaxSize;index++)
      {
        curent_digit=carry+(index<Number1->NrOfDigits ? (Number1->Digits[index]-'0') : 0) + (index<Number2->NrOfDigits ? (Number2->Digits[index]-'0') : 0);
        carry=curent_digit/10;
        curent_digit=curent_digit%10;
        RezultSumString[index]=curent_digit+'0';
      }
    
    if(carry>0)
      {
        RezultSumString[index]='1';
        RezultSumString[index+1]='\0';
        NrOfDigits++;
      }
    else
      {
        RezultSumString[index]='\0';
      }

    BigNumber* RezultSum=PrivateConstructor(RezultSumString,NrOfDigits); 
    return RezultSum;
}


BigNumber* Multiply(BigNumber* Number1, BigNumber* Number2) //O(Size(Number1)+Size(Number2))
{
    char *RezultProductString=calloc(Number1->NrOfDigits+Number2->NrOfDigits+1,sizeof(char));
    unsigned int MaxPossibleDigits=Number1->NrOfDigits+Number2->NrOfDigits+1;

    unsigned int i=0,j=0,index=0;
    for(i=0;i<Number1->NrOfDigits;i++)
      {
         for(j=0;j<Number2->NrOfDigits;j++)
           {
              index=i+j;
              RezultProductString[index]+=((Number1->Digits[i]-'0') * ((Number2->Digits[j]-'0')));
              RezultProductString[index+1]+=RezultProductString[index] /10;
              RezultProductString[index]=RezultProductString[index]%10;
           }
      }
    
    unsigned int NrOfDigits=MaxPossibleDigits;
    while (NrOfDigits > 1 && RezultProductString[NrOfDigits- 1] == 0) //Removing Trailing 0`s from calloc
    {
        NrOfDigits--;
    }

    for (i = 0; i < NrOfDigits; i++)
    {
        RezultProductString[i] += '0'; //Transforming Digits to Characters
    }

    RezultProductString[NrOfDigits] = '\0';
    RezultProductString=realloc(RezultProductString,sizeof(char)*(NrOfDigits+1));
    BigNumber* RezultProduct=PrivateConstructor(RezultProductString,NrOfDigits); 

    return RezultProduct;
}

BigNumber* FromUnsignedIntegerToBigNum(unsigned int number)
{
    //4,294,967,295 (max value of unsigned int) -10 digits needed +'\0'
    char* Digits=malloc(sizeof(char)*11);
    short int digit=0;
    short int NrOfDigits=0;
    while(number>0)  //construcs number in reverse order directly
      {
        digit=number%10;
        Digits[NrOfDigits]=digit+'0';
        number/=10;
        NrOfDigits++;
      }
    
    Digits[NrOfDigits]='\0';
    return PrivateConstructor(Digits,NrOfDigits);
}



BigNumber* Factorial(unsigned int Number)
{
    BigNumber* FactorialRezult=Init("1");
    unsigned int index=2;
    for(index=1;index<=Number;index++)
      {
         FactorialRezult=Multiply(FactorialRezult,FromUnsignedIntegerToBigNum(index));
      }
    return FactorialRezult;
}

char *ToString(BigNumber *Number)
{
    char *String=malloc(Number->NrOfDigits+1);
    unsigned int index=0;
    for(index=0;index<Number->NrOfDigits;index++) //as it is stored in reverse we need to display in reverse
      String[index]=Number->Digits[Number->NrOfDigits-index-1];
    String[index]='\0';
    return String;
}

void FreeMemory(BigNumber *Number)
{
    free(Number->Digits);
    free(Number);
}

void PrintBigNumber(BigNumber *Number)
{
    for(unsigned int index=0;index<Number->NrOfDigits;index++) //as it is stored in reverse we need to display in reverse
      printf("%c",Number->Digits[Number->NrOfDigits-index-1]);
    printf(" ");
}

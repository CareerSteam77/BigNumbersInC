#include "Platform.h"

#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<stdbool.h>
#include<ctype.h>
#include<pthread.h>

#define BurnikelZiegler_BOUND 64  //At how many digits should Burnikel-Ziegler be used instead of standard division

#define Karatsuba_BOUND 128       //At how many digits should Karatsuba be used instead of standard multiplication
#define ToomCook3Way_BOUND 1024   //At how many digits should ToomCook3Way be used for multiplication instead of Karatsuba

typedef struct{
    char* Digits;  //Digits are stored in reverse order for easier arithmetic operations
    unsigned int NrOfDigits;  //Numbers can have a max number of digits of 4,294,967,295 (max value of unsigned int)
    bool IsNegative; //True if the number is negative, false otherwise
}BigNumber;

typedef struct{
   BigNumber *Mantissa; //significand
   long int Exponent; 
}BigFloatNumber;  // BigFloatNumber= significand* 10^Exponent ;Exemple: 123.45= 12345*10^(-2)

typedef struct {
    BigNumber *Number1;
    BigNumber *Number2;
    BigNumber *Result;
    unsigned short int Iterations;
}ThreadArgumentKaratsuba;

typedef struct {
    BigNumber *Number1;
    BigNumber *Number2;
    BigNumber *Result;
    unsigned short int Iterations;
}ThreadArgumentToomCook;

typedef struct{
    BigNumber *Number;
    BigNumber *Result;
    unsigned short int Iterations;
}ThreadArgumentKaratsubaSquared;

typedef struct{
    BigNumber *Number;
    BigNumber *Result;
    unsigned short int Iterations;
}ThreadArgumentToomCookSquared;


//Using the Compress Function 
//Exponent < 0: It has decimals
//Exponent == 0: It is an integer ending in 1-9 
//Exponent > 0: It is an integer ending in 0`s 


//CONSTANTS
static BigFloatNumber* INTERNAL_GLOBAL_PI = NULL;
static BigFloatNumber* INTERNAL_GLOBAL_LN10 = NULL;

//MAXIMUM PRECISION THAT THE LIBRARY CAN CALCULATE AFTER INITIALIZATION ,note this can change if the user recalls InitializeBigNumberSupport() but it will recalculate the constants
static unsigned int INTERNAL_GLOBAL_PRECISION = 0;


static void CleanTrailingZeros(BigNumber* Number)  //Eliminates Unecesary 0`s from the number
{
    while (Number->NrOfDigits>1 && (Number->Digits[Number->NrOfDigits - 1] == '0'))
    {
      Number->NrOfDigits--;
    }
    Number->Digits[Number->NrOfDigits] = '\0';
}

static void CompressFloatInPlace(BigFloatNumber *Number) //Every tail 0 gets removed and added to the exponent thus shortening the Mantissa lenght facilitating faster arithmetic operations
{
  //Doesnt produce a new number, change happens in Memory  Time Complexity of O(NrOfDigits)
    if (Number == NULL || Number->Mantissa == NULL || Number->Mantissa->Digits == NULL) return;

    if (Number->Mantissa->NrOfDigits == 1 && Number->Mantissa->Digits[0] == '0')
    {
        Number->Exponent = 0;
        return;
    }

    unsigned int CountZeros = 0;
    while (CountZeros< Number->Mantissa->NrOfDigits && Number->Mantissa->Digits[CountZeros] == '0')
    {
        CountZeros++;
    }

    if (CountZeros==0) return;

    if (CountZeros==Number->Mantissa->NrOfDigits)
    {
        Number->Mantissa->Digits[0] = '0';
        Number->Mantissa->Digits[1] = '\0';
        Number->Mantissa->NrOfDigits = 1;
        Number->Mantissa->IsNegative = false;
        Number->Exponent = 0;
        return;
    }

    unsigned int new_nr_digits = Number->Mantissa->NrOfDigits-CountZeros;
    for (unsigned int i = 0; i < new_nr_digits; i++)
    {
        Number->Mantissa->Digits[i] = Number->Mantissa->Digits[i+CountZeros];
    }

    Number->Mantissa->Digits[new_nr_digits] = '\0';
    Number->Mantissa->NrOfDigits = new_nr_digits;

    Number->Exponent +=CountZeros;
}

void SwapNumbersInMemory(BigNumber **Number1,BigNumber **Number2)
{
   BigNumber *Temporal=*Number1;
   *Number1=*Number2;
   *Number2=Temporal;
}

void SwapNumbersInMemoryFloat(BigFloatNumber **Number1,BigFloatNumber **Number2)
{
   BigFloatNumber *Temporal=*Number1;
   *Number1=*Number2;
   *Number2=Temporal;
}

static bool VerifyStringIsNumber(const char *Value)
{
  //check for string of the form "-x1x2x3..." and "x1x2x3..." where xi is a digit
    if(Value==NULL)
       return false;

    if(!isdigit(Value[0]) && Value[0]!='-')
       return false;
       
    for(unsigned int index=1;index<strlen(Value);index++)
       {
        if(isdigit(Value[index])==false)
          return false;
       }

    return true;
}

static bool VerifyStringIsDecimal(const char *Value)
{
  //check for string of the form "-x1x2..'.'x3..." and "x1x2..'.'x3..." where xi is a digit and it need to have one decimal
    if(Value==NULL || Value[0]=='\0')  //check for NULL and Empty string
       return false;

    if(!isdigit(Value[0]) && Value[0]!='-')
       return false;
    
    short int DecimalPointCounter=0; //it should be always 1 or 0 ,otherwise Init fails  
    for(unsigned int index=1;index<strlen(Value);index++)
       {
        if(isdigit(Value[index])==false && (Value[index]!='.')  && (Value[index]!=','))
          return false;

        if(Value[index]=='.' || Value[index]==',')
          DecimalPointCounter++;
       }
      
    if(DecimalPointCounter>1)
      return false;

    return true;
}

BigNumber* Init(const char* Value)   //Takes a string, checks if it a valid number and construct a BigNumber
{
    if(VerifyStringIsNumber(Value)==false) //checks if the string is not NULL and an integer
      {
        return NULL;
      }

    BigNumber* Number=malloc(sizeof(BigNumber));
    if(Number==NULL)
      {
        perror("Memory Allocation for BigNumber failed");
        exit(-1);
      }
    
    if(Value[0]!='-')
      {
        Number->IsNegative=false;
        Number->NrOfDigits=strlen(Value);
        Number->Digits=malloc(sizeof(char)*(Number->NrOfDigits+1)); //store NrOfDigits +'\0'
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
    else
    {
        Number->IsNegative=true;
        Number->NrOfDigits=strlen(Value)-1; //Consider negative sign
        Number->Digits=malloc(sizeof(char)*(Number->NrOfDigits+1));  //store NrOfDigits +'\0'
        unsigned int index=0;
        if(Number->Digits==NULL)
          {
            perror("Memory Allocation for BigNumber Digits failed");
            exit(-1);
          }
         for(index=0;index<Number->NrOfDigits;index++)
          {
            Number->Digits[index] = Value[Number->NrOfDigits-index]; //Reverse the string into Digits
          }
        Number->Digits[index]='\0';
        return Number;
        }
}

BigFloatNumber *InitFloat(char *Value)
{
     if(VerifyStringIsDecimal(Value)==false)
       {
          return NULL;
       }
      
     BigFloatNumber *Number=malloc(sizeof(BigFloatNumber));
     if(Number==NULL)
       {
          perror("Allocating Memory for BigFloat Failed");      //allocating memory for Mantissa and BigFloat
          exit(-1);
       }
    
    char *DigitsSignificand=malloc(sizeof(char)*strlen(Value));
    unsigned int FractionalDigits=0;
    unsigned int index=0;
    unsigned int indexDigitsSignificand=0;
    bool StartCounting=false;
    for(index=0;index<strlen(Value);index++)
      {
          if(Value[index]=='.')
            StartCounting=true;
          else
            {
              DigitsSignificand[indexDigitsSignificand]=Value[index];
              if(StartCounting==true)
                FractionalDigits++;
              indexDigitsSignificand++;
            }
      }
    DigitsSignificand[indexDigitsSignificand]='\0';
    int first_valid_digit = 0;
    while (DigitsSignificand[first_valid_digit] == '0' && DigitsSignificand[first_valid_digit + 1] != '\0')
    {
        first_valid_digit++;
    }


    Number->Exponent=-FractionalDigits;
    Number->Mantissa=Init(&DigitsSignificand[first_valid_digit]);
    
    free(DigitsSignificand);
    CompressFloatInPlace(Number);
    return Number;
}

void FreeMemory(BigNumber *Number)
{
    if (Number == NULL) return;
    free(Number->Digits);
    free(Number);
}

BigNumber* CloneBigNumber(BigNumber* Original)  //Functions Clones the Original by allocating new memory keeping the old one intact
{
    if (Original == NULL) return NULL;
    
    BigNumber* Copy=malloc(sizeof(BigNumber));
    if(Copy==NULL)
      {
        perror("Allocating Memory for CopyINT failed");
        exit(-1);
      }
    
    Copy->NrOfDigits=Original->NrOfDigits;
    Copy->IsNegative=Original->IsNegative;
    
    Copy->Digits=malloc(Original->NrOfDigits + 1);
    if(Copy->Digits==NULL)
      {
        perror("Allocating Memory for Copy->Digits failed");
        exit(-1);
      }
    strcpy(Copy->Digits,Original->Digits);
    
    return Copy;
}

BigFloatNumber* CloneBigNumberFloat(BigFloatNumber* Original)  //Functions Clones the Original by allocating new memory keeping the old one intact
{
    if (Original == NULL) return NULL;
    
    BigFloatNumber* Copy=malloc(sizeof(BigFloatNumber));
    if(Copy==NULL)
      {
        perror("Allocating Memory for CopyFLOAT failed");
        exit(-1);
      }
    Copy->Mantissa=malloc(sizeof(BigNumber));
    if(Copy->Mantissa==NULL)
     {
        perror("Allocating Memory for CopyFLOAT->Mantissa failed");
        exit(-1);
     }
    Copy->Mantissa->NrOfDigits=Original->Mantissa->NrOfDigits;  //copy NrOfDigits
    Copy->Mantissa->IsNegative=Original->Mantissa->IsNegative;  //copy IsNegative
    
    Copy->Mantissa->Digits=malloc(Original->Mantissa->NrOfDigits + 1);    // copy the digits
    if(Copy->Mantissa->Digits==NULL)
      {
        perror("Allocating Memory for Copy->Digits->Mantissa failed");
        exit(-1);
      }
    strcpy(Copy->Mantissa->Digits,Original->Mantissa->Digits);

    Copy->Exponent = Original->Exponent;  //copy the exponent

    return Copy;
}

BigFloatNumber *FromBigNumber(BigNumber *Number) //Construct a new Float from Number, making a conversion by copying the digits and setting exponent to 0
{
  if(Number==NULL)
    {
      perror("ERROR:You are Trying to convert a NULL");
      exit(-3);
    }

  BigFloatNumber *NumberFloat=malloc(sizeof(BigFloatNumber));
  if(NumberFloat==NULL)
    {
      perror("Allcating Memory for NumberFloat Failed");
      exit(-1);
    }
  NumberFloat->Mantissa=malloc(sizeof(BigNumber));
  if(NumberFloat->Mantissa==NULL)
    {
      perror("Allcating Memory for NumberFloat->Mantissa Failed");
      exit(-1);
    }

  NumberFloat->Mantissa->Digits=malloc(sizeof(char)*(Number->NrOfDigits+1));
  if(NumberFloat->Mantissa->Digits ==NULL)
    {
      perror("Allcating Memory for NumberFloat->Mantissa->Digits Failed");
      exit(-1);
    }
  
  strcpy(NumberFloat->Mantissa->Digits,Number->Digits);  //copy the digits and store in Mantissa
  NumberFloat->Mantissa->IsNegative=Number->IsNegative;
  NumberFloat->Mantissa->NrOfDigits=Number->NrOfDigits;
  NumberFloat->Exponent=0;  

  CompressFloatInPlace(NumberFloat);
  return NumberFloat;
}

void FreeMemoryFloat(BigFloatNumber *Number)
{
   if (Number == NULL) return;
   FreeMemory(Number->Mantissa);
   free(Number);
}

static void ShiftRightNPositions(BigNumber *Number,unsigned int N) //For multiplication by positive integer powers of 10 and for Long Division
{  
  //DOESNT PRODUCE A NEW BIGINT, changes the argument in memory

   if(N<=0) return; //no change needed
   if (Number->NrOfDigits == 1 && Number->Digits[0] == '0') return; //!!!Dont Shift the Number if it is 0,

   char *ExpandedDigits = realloc(Number->Digits, Number->NrOfDigits + N + 1);
    if (ExpandedDigits == NULL)
    {
        perror("Memory reallocation failed in ShiftRightNPositions");
        exit(-1);
    }
    
    memmove(ExpandedDigits + N, ExpandedDigits, Number->NrOfDigits + 1); //Move the digits to the right N positions
    memset(ExpandedDigits, '0', N); //Assing the zerous at the start

    Number->Digits = ExpandedDigits;
    Number->NrOfDigits += N;
}

void MultiplyByNegativeOne(BigNumber *Number)
{
  Number->IsNegative=!Number->IsNegative;
}

BigNumber *PrivateConstructor(char *Digits,unsigned int NrOfDigits,bool IsNegative)  //Construct a BigNumber WITHOUT reversing the string, used in Arithemic operations
{
   BigNumber* Number=malloc(sizeof(BigNumber));
   if(Number==NULL)
      {
        perror("Memory Allocation for BigNumber failed");
        exit(-1);
      }
    
    Number->NrOfDigits=NrOfDigits;
    Number->Digits=Digits;
    Number->IsNegative=IsNegative;
    
    return Number;
}

BigFloatNumber *PrivateConstructorFloat(BigNumber* Mantissa,long int Exponent) 
{
   BigFloatNumber* Number=malloc(sizeof(BigFloatNumber));
   if(Number==NULL)
      {
        perror("Memory Allocation for BigFloatNumber failed");
        exit(-1);
      }
    
    Number->Mantissa=Mantissa;
    Number->Exponent=Exponent;
    
    CompressFloatInPlace(Number);
    return Number;
}

bool IsEven(BigNumber *Number) //return true is the number is even by evaluating the first digit
{
   if(Number==NULL)
     return true;
    
   char FirstDigit=Number->Digits[0];
   if(FirstDigit=='0' || FirstDigit=='2'|| FirstDigit=='4' || FirstDigit=='6' || FirstDigit=='8' )
     return true;
   else
     return false;
}

bool IsOdd(BigNumber *Number) //return true is the number is odd by returning opposite of IsEven
{
   return !IsEven(Number);
}

bool IsNegative(BigNumber *Number) //return true if the number is negative
{
   return Number->IsNegative;
}

bool IsEqual(BigNumber *Number1,BigNumber *Number2)  //returns true if there are equal, false otherwise
{
    if(Number1->IsNegative!=Number2->IsNegative)
       return false;
    else
      {
          if(Number1->NrOfDigits!=Number2->NrOfDigits)
             return false;
          
          unsigned int index=0;
          for(index=0;index<Number1->NrOfDigits;index++)
             {
                  if(Number1->Digits[index]!=Number2->Digits[index])
                     return false;
             }

          return true;
      }
}

int BigNumberCompareAbsoluteValue(BigNumber* Number1, BigNumber* Number2)  //return 1 if |Number1|>|Number2|, -1 if |Number1|<|Number2| , 0 if  |Number1|==|Number2|
{
  if(Number1->NrOfDigits>Number2->NrOfDigits)  
    return 1;
  if(Number1->NrOfDigits<Number2->NrOfDigits)
    return -1;
  
  for(unsigned int Index=0;Index<Number1->NrOfDigits;Index++)
    {
       if(Number1->Digits[Number1->NrOfDigits-Index-1] > Number2->Digits[Number2->NrOfDigits-Index-1])
          return 1;
        if(Number1->Digits[Number1->NrOfDigits-Index-1] < Number2->Digits[Number2->NrOfDigits-Index-1])
          return -1;
    }
  
  return 0;
}

int BigNumberCompare(BigNumber* Number1, BigNumber* Number2)  //return 1 if Number1>Number2, -1 if Number1<Number2 , 0 if  Number1==Number2
{
   if(Number1->IsNegative && !Number2->IsNegative)
      return -1;
   if(!Number1->IsNegative && Number2->IsNegative)
      return 1;
  
   if(!Number1->IsNegative && !Number2->IsNegative)
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

    if(!Number1->IsNegative && !Number2->IsNegative)
    {
      if(Number1->NrOfDigits>Number2->NrOfDigits)  
        return -1;
      if(Number1->NrOfDigits<Number2->NrOfDigits)
        return 1;
    
      unsigned int Index=0;
      for(Index=0;Index<Number1->NrOfDigits;Index++)
      {
        if(Number1->Digits[Number1->NrOfDigits-Index-1] > Number2->Digits[Number2->NrOfDigits-Index-1])
          return -1;
        if(Number1->Digits[Number1->NrOfDigits-Index-1] < Number2->Digits[Number2->NrOfDigits-Index-1])
          return 1;
      }

      return 0;
    }
  return 0;
}

void Increment(BigNumber* Number) // Increments in Memory, Time Complexity of O(1) in AVG ,realloc rarely happens
{
    int carry = 1;
    unsigned int index = 0;
    unsigned short int curent_digit=0;
    while (carry>0 && index<Number->NrOfDigits)
    {
        curent_digit=(Number->Digits[index] - '0') + carry;   
        if(curent_digit>9)
        {
            Number->Digits[index] ='0';
            carry = 1;
        }
        else
        {
            Number->Digits[index] =curent_digit + '0';
            carry = 0;
        }
        index++;
    }

    if (carry > 0)
    {
        Number->NrOfDigits++;
        
        // Reallocate memory to make room for the new digit and null terminator
        Number->Digits = realloc(Number->Digits, Number->NrOfDigits + 1);
        if (Number->Digits == NULL)
        {
            perror("Memory reallocation failed during Increment");
            exit(-1);
        }
        
        Number->Digits[Number->NrOfDigits - 1] = '1';
        Number->Digits[Number->NrOfDigits] = '\0';
    }
}

void RoundFloat(BigFloatNumber *Number, unsigned int MaxPrecision) //Modifies the Number given in the Argument by Rounding to MaxPrecision Digits ,Complexity is O(NrOfDigits)
{
    if (Number == NULL || Number->Mantissa == NULL) return;

    long int CurentPrecision=-Number->Exponent;

    if ( (CurentPrecision<=MaxPrecision) || (Number->Exponent >= 0)) // Either the number is an integer or it doesnt have enough digits
    {
        return; 
    }

    long int DigitsToChop =CurentPrecision-MaxPrecision; //  Calculate how many digits we need to chop off from the least significant side
    if (DigitsToChop >= Number->Mantissa->NrOfDigits)
    {
        // Number goes to 0 if we need to chop to many digits
        Number->Mantissa->Digits[0] = '0';
        Number->Mantissa->Digits[1] = '\0';
        Number->Mantissa->NrOfDigits = 1;
        Number->Mantissa->IsNegative = false;
        Number->Exponent = -MaxPrecision;
        return;
    }

    // Save the highest digit we are about to delete so we can evaluate rounding
    char RoundingDigit = Number->Mantissa->Digits[DigitsToChop - 1];

    //Shift the remaining digits down to index 0, overwriting the chopped ones
    unsigned int NewNrOfDigits = Number->Mantissa->NrOfDigits -DigitsToChop;
    for (unsigned int index = 0; index < NewNrOfDigits; index++)
    {
        Number->Mantissa->Digits[index] = Number->Mantissa->Digits[index+DigitsToChop];
    }
    
    //Drop the new null-terminator and update the digits
    Number->Mantissa->Digits[NewNrOfDigits] = '\0';
    Number->Mantissa->NrOfDigits = NewNrOfDigits;
    Number->Exponent = -MaxPrecision;

    if (RoundingDigit >= '5')
    {
      Increment(Number->Mantissa);
    }
}

BigNumber* Sum(BigNumber* Number1, BigNumber* Number2) 
{
  if (Number1 == NULL || Number2 == NULL) return NULL;

  if(Number1->IsNegative==Number2->IsNegative) //if they have the same sign |a+b|=|-(a+b)| 
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
    BigNumber* RezultSum=PrivateConstructor(RezultSumString,NrOfDigits,Number1->IsNegative); 
    return RezultSum;
  }

  else  //they have different signs |a-b|=|b-a|  we calculate (max(|a|,|b|)-min(|a|,|b|)) and consider Comparing them for Sign
  {
    bool IsNegative = false;
    int AbsoluteCompareResult = BigNumberCompareAbsoluteValue(Number1, Number2);
    if (AbsoluteCompareResult==1) 
    {
        // |Number1| > |Number2|, so Number1 dictates the sign
        IsNegative = Number1->IsNegative;
    } 
    else if (AbsoluteCompareResult==-1) 
    {
        // |Number1| < |Number2|, so Number2 dictates the sign
        IsNegative = Number2->IsNegative;
        
        // Swap local pointers so Number1 is always the larger one for the math loop
        SwapNumbersInMemory(&Number1, &Number2);
    }
    else 
    {
        //|Number1| == |Number2| Sign remains false.
        IsNegative = false;
    }
    
    unsigned int MaxSize=0;
    if(Number1->NrOfDigits>=Number2->NrOfDigits)
      MaxSize=Number1->NrOfDigits;
    else
      MaxSize=Number2->NrOfDigits;
    
    char *ResultStringSubtract=malloc(sizeof(char)*MaxSize+1);
    unsigned int index=0;
    int curent_digit=0;
    int burrow=0;
    for(index=0;index<MaxSize;index++)
      {
        curent_digit=-burrow + (index<Number1->NrOfDigits ? (Number1->Digits[index]-'0') : 0) - (index<Number2->NrOfDigits ? (Number2->Digits[index]-'0') : 0);
        if(curent_digit<0)
          {
            burrow=1;
            ResultStringSubtract[index]=(curent_digit+10)+'0';
          }
        else
          {
            burrow=0;
             ResultStringSubtract[index]=(curent_digit)+'0';
          }
      }
    
    unsigned int NrOfDigits = MaxSize;

    while (NrOfDigits > 1 && (ResultStringSubtract[NrOfDigits - 1] == '0')) //Removing Trailing 0`s
    { 
        NrOfDigits--;
    }
    ResultStringSubtract[NrOfDigits]='\0';

    BigNumber* RezultSubtract=PrivateConstructor(ResultStringSubtract,NrOfDigits,IsNegative); 
    return RezultSubtract;
  }
}

BigFloatNumber* SumFloat(BigFloatNumber* Number1,BigFloatNumber* Number2) 
{
   if (Number1 == NULL || Number2 == NULL) return NULL;

   //We need to normalize the number with the bigger exponent than use Sum on the Mantissas
   //For normalization we will use the ShiftRightNpositions

  //If One of the Numbers is 0, Clone the other and return the clone
  if (Number1->Mantissa->NrOfDigits == 1 && Number1->Mantissa->Digits[0] == '0')
    {
        return CloneBigNumberFloat(Number2);
    }
    if (Number2->Mantissa->NrOfDigits == 1 && Number2->Mantissa->Digits[0] == '0')
    {
        return CloneBigNumberFloat(Number1);
    }

   long int RezultExponent;
   if(Number1->Exponent<Number2->Exponent)
      {
        RezultExponent=Number1->Exponent;
        unsigned int shift=Number2->Exponent-Number1->Exponent;
        BigNumber *CloneSecond=CloneBigNumber(Number2->Mantissa);
        ShiftRightNPositions(CloneSecond,shift);  //Normalizing the Mantissa
        
        BigNumber*RezultSumMatissa=Sum(Number1->Mantissa,CloneSecond);
        BigFloatNumber *RezultSumNumber=malloc(sizeof(BigFloatNumber));
        if(RezultSumNumber==NULL)
          {
            perror("Allocating Memory for RezultSumNumber failed");
            exit(-1);
          }
        
        RezultSumNumber->Mantissa=RezultSumMatissa;
        RezultSumNumber->Exponent=RezultExponent;
        
        FreeMemory(CloneSecond);
        CompressFloatInPlace(RezultSumNumber);
        return RezultSumNumber;
      }
     else
      {
          if(Number1->Exponent>Number2->Exponent)
          {
            RezultExponent=Number2->Exponent;
            unsigned int shift=Number1->Exponent-Number2->Exponent;
            BigNumber *CloneFirst=CloneBigNumber(Number1->Mantissa);
            ShiftRightNPositions(CloneFirst,shift);  //Normalizing the Mantissa
        
            BigNumber*RezultSumMatissa=Sum(Number2->Mantissa,CloneFirst);
            BigFloatNumber *RezultSumNumber=malloc(sizeof(BigFloatNumber));
            if(RezultSumNumber==NULL)
              {
                perror("Allocating Memory for RezultSumNumber failed");
                exit(-1);
              }
        
            RezultSumNumber->Mantissa=RezultSumMatissa;
            RezultSumNumber->Exponent=RezultExponent;
        
            FreeMemory(CloneFirst);
            CompressFloatInPlace(RezultSumNumber);
            return RezultSumNumber;
          }
        else  //Exponents are equal ,NO NORMALIZATION needed
        {
            RezultExponent=Number2->Exponent;   
            BigNumber*RezultSumMatissa=Sum(Number2->Mantissa,Number1->Mantissa);
            BigFloatNumber *RezultSumNumber=malloc(sizeof(BigFloatNumber));
            if(RezultSumNumber==NULL)
              {
                perror("Allocating Memory for RezultSumNumber failed");
                exit(-1);
             }
        
            RezultSumNumber->Mantissa=RezultSumMatissa;
            RezultSumNumber->Exponent=RezultExponent;
            
            CompressFloatInPlace(RezultSumNumber);
            return RezultSumNumber;
        }
    }
  
  return NULL;
}

BigNumber* Subtract(BigNumber* Number1, BigNumber* Number2)
{
  //Is equivalent to Number1+MultiplyByNegativeOne(Number2)  a-b == a+ (-b)
  if (Number1 == NULL || Number2 == NULL) return NULL;
    
  BigNumber* Clone2 = CloneBigNumber(Number2); //Clone the number in order to make subtraction Thread Safe
  MultiplyByNegativeOne(Clone2); 
    
  BigNumber *Rezult = Sum(Number1, Clone2);
    
  FreeMemory(Clone2); 
  return Rezult;
}

BigFloatNumber* SubtractFloat(BigFloatNumber* Number1, BigFloatNumber* Number2)
{
  if (Number1 == NULL || Number2 == NULL) return NULL;
  //Is equivalent to Number1+MultiplyByNegativeOne(Number2)  a-b == a+ (-b)
  
  BigFloatNumber*Clone2=CloneBigNumberFloat(Number2); //Clone the number in order to make subtraction with floats Thread Safe
  MultiplyByNegativeOne(Clone2->Mantissa); 

  BigFloatNumber *Rezult=SumFloat(Number1,Clone2);
  
  FreeMemoryFloat(Clone2);
  CompressFloatInPlace(Rezult);
  return Rezult;
}

static BigNumber* StandardSquare(BigNumber* Number) 
{
    if (Number == NULL) return NULL;

    unsigned int MaxPossibleDigits = Number->NrOfDigits * 2; // x^2 has exactly 2N digits maximum
    
    bool IsNegative = false; // x^2 is always positive

    unsigned int *Accumulator = calloc(MaxPossibleDigits, sizeof(unsigned int));
    if (Accumulator == NULL)
    {
        perror("Allocating Memory For Accumulator inside of StandardSquare FAILED");
        exit(-1);
    }

    for(unsigned int i = 0; i < Number->NrOfDigits; i++)
    {
        unsigned int digit_i = Number->Digits[i] - '0';     
        Accumulator[i + i] += digit_i * digit_i;   
        for(unsigned int j = i + 1; j < Number->NrOfDigits; j++)
        {
            unsigned int digit_j = Number->Digits[j] - '0';
            Accumulator[i + j] += (digit_i * digit_j) << 1; 
        }
    }
    
    unsigned int carry = 0, current_sum = 0;
    for (unsigned int i = 0; i < MaxPossibleDigits; i++)
    {
        current_sum = Accumulator[i] + carry;
        Accumulator[i] = current_sum % 10;  
        carry = current_sum / 10; 
    }

    unsigned int NrOfDigits = MaxPossibleDigits;
    while (NrOfDigits > 1 && Accumulator[NrOfDigits - 1] == 0) 
    {
        NrOfDigits--;
    }

    char *RezultProductString = malloc(sizeof(char) * (NrOfDigits + 1));
    for (unsigned int i = 0; i < NrOfDigits; i++)
    {
        RezultProductString[i] = Accumulator[i] + '0'; 
    }
    RezultProductString[NrOfDigits] = '\0';
    
    BigNumber* RezultProduct = PrivateConstructor(RezultProductString, NrOfDigits, IsNegative); 

    free(Accumulator);
    return RezultProduct;
}

static BigNumber* StandardMultiply(BigNumber* Number1, BigNumber* Number2) //O(Size(Number1)*Size(Number2))
{
    unsigned int MaxPossibleDigits=Number1->NrOfDigits+Number2->NrOfDigits+1;
    bool IsNegative = (Number1->IsNegative != Number2->IsNegative);

    unsigned int *Accumulator = calloc(MaxPossibleDigits, sizeof(unsigned int));
    if (Accumulator == NULL)
     {
       perror("Alocating Memory For Accumulator inside of StandardMultiply FAILED");
       exit(-1);
     }

    unsigned int i=0;unsigned int j=0;
    for(i=0;i<Number1->NrOfDigits;i++)
    {
        unsigned int digit1=Number1->Digits[i]-'0';
        for(j=0;j<Number2->NrOfDigits;j++)
        {
            unsigned int digit2 = Number2->Digits[j]-'0';
            Accumulator[i+j] += digit1*digit2;
        }
    }
    
    unsigned int carry = 0; unsigned int current_sum=0;
    for (i=0;i< MaxPossibleDigits;i++)
    {
        current_sum = Accumulator[i] + carry;
        Accumulator[i] = current_sum % 10;  
        carry = current_sum / 10; 
    }

    unsigned int NrOfDigits = MaxPossibleDigits;
    while (NrOfDigits > 1 && Accumulator[NrOfDigits - 1] == 0) 
    {
        NrOfDigits--;
    }

    char *RezultProductString = malloc(sizeof(char) * (NrOfDigits + 1));
    if (RezultProductString == NULL)
    {
        perror("Allocating Memory For RezultProductString FAILED");
        free(Accumulator);
        exit(-1);
    }

    for (i = 0; i < NrOfDigits; i++)
    {
        RezultProductString[i] = Accumulator[i] + '0'; 
    }

    RezultProductString[NrOfDigits] = '\0';
    BigNumber* RezultProduct=PrivateConstructor(RezultProductString,NrOfDigits,IsNegative); 

    free(Accumulator);
    return RezultProduct;
}

static BigNumber* Karatsuba(BigNumber *Number1,BigNumber *Number2)
{
    if((Number1->NrOfDigits == 1 && Number1->Digits[0] == '0') || (Number2->NrOfDigits == 1 && Number2->Digits[0] == '0'))
    {
        return Init("0");
    }

    if(Number1->NrOfDigits < Karatsuba_BOUND || Number2 ->NrOfDigits < Karatsuba_BOUND)
      return StandardMultiply(Number1,Number2);
 
    unsigned int SplitSize=0;
    unsigned int MaxLen1 = Number1->NrOfDigits;
    unsigned int MaxLen2 = Number2->NrOfDigits;
    if(Number1->NrOfDigits>Number2->NrOfDigits)
        SplitSize=Number1->NrOfDigits/2;
    else
        SplitSize=Number2->NrOfDigits/2;

    BigNumber *Low1INT, *High1INT, *Low2INT, *High2INT;
    if (MaxLen1 > SplitSize) 
    {
        unsigned int High1Size = MaxLen1 - SplitSize;
        
        char* Low1 = malloc(SplitSize + 1);
        memcpy(Low1, Number1->Digits, SplitSize);
        Low1[SplitSize] = '\0';
        
        char* High1 = malloc(High1Size + 1);
        memcpy(High1, Number1->Digits + SplitSize, High1Size);
        High1[High1Size] = '\0';

        // Transfer ownership of the malloc'd pointers directly to the struct
        Low1INT = PrivateConstructor(Low1, SplitSize, false);
        High1INT = PrivateConstructor(High1, High1Size, false);
    } 
    else 
    {
        // Number is too small for a high half. 
        char* Low1 = malloc(MaxLen1 + 1);
        memcpy(Low1, Number1->Digits, MaxLen1 + 1); // Copies the \0 too
        
        char* High1 = malloc(2);
        memcpy(High1, "0", 2); 

        Low1INT = PrivateConstructor(Low1, MaxLen1, false);
        High1INT = PrivateConstructor(High1, 1, false);
    }

    // Splitting Number 2
    if (MaxLen2 > SplitSize) 
    {
        unsigned int High2Size = MaxLen2 - SplitSize;
        
        char* Low2 = malloc(SplitSize + 1);
        memcpy(Low2, Number2->Digits, SplitSize);
        Low2[SplitSize] = '\0';
        
        char* High2 = malloc(High2Size + 1);
        memcpy(High2, Number2->Digits + SplitSize, High2Size);
        High2[High2Size] = '\0';

        Low2INT = PrivateConstructor(Low2, SplitSize, false);
        High2INT = PrivateConstructor(High2, High2Size, false);
    } 
    else 
    {
        char* Low2 = malloc(MaxLen2 + 1);
        memcpy(Low2, Number2->Digits, MaxLen2 + 1);
        
        char* High2 = malloc(2);
        memcpy(High2, "0", 2);

        Low2INT = PrivateConstructor(Low2, MaxLen2, false);
        High2INT = PrivateConstructor(High2, 1, false);
    }

    BigNumber* Z0=Karatsuba(Low1INT,Low2INT);  
    BigNumber* Z2=Karatsuba(High1INT,High2INT);
    BigNumber* SumPair1=Sum(Low1INT,High1INT);
    BigNumber* SumPair2=Sum(Low2INT,High2INT); 
    BigNumber* Z3=Karatsuba(SumPair1,SumPair2);
    
    FreeMemory(Low1INT);FreeMemory(Low2INT);FreeMemory(High1INT);
    FreeMemory(High2INT);FreeMemory(SumPair1);FreeMemory(SumPair2);

    BigNumber *AuxiliaryDifference=Subtract(Z3,Z2);
    BigNumber *AuxiliaryDifference2=Subtract(AuxiliaryDifference,Z0);
    ShiftRightNPositions(AuxiliaryDifference2,SplitSize);
    FreeMemory(AuxiliaryDifference);

    ShiftRightNPositions(Z2,SplitSize*2);
    BigNumber *TempRezult=Sum(Z2,AuxiliaryDifference2);
    BigNumber *Rezult=Sum(TempRezult,Z0);

    FreeMemory(TempRezult);FreeMemory(AuxiliaryDifference2);
    FreeMemory(Z2);FreeMemory(Z0);FreeMemory(Z3);
    
    Rezult->IsNegative = (Number1->IsNegative != Number2->IsNegative);
    return Rezult;
}

static BigNumber* KaratsubaMultiThreaded(BigNumber* Number1, BigNumber* Number2, unsigned short int NumberOfIterations);
void* KaratsubaTheadFunc(void *argument)
{
    ThreadArgumentKaratsuba *args = (ThreadArgumentKaratsuba*)argument;
    args->Result = KaratsubaMultiThreaded(args->Number1, args->Number2, args->Iterations); 
    return NULL;
}

static BigNumber* KaratsubaMultiThreaded(BigNumber* Number1,BigNumber*Number2,unsigned short int NumberOfIterations) // For the 0,1,2,3... iterations of Karatsuba spawn for every recursive call a thread to compute Z0,Z1,Z3
{
   //We should only spawn at maximum 27 threads per number depending on its size and in the future if the system suport this level of multithreading
   //NumberOfIterations will assigned in the Multiply function
    if(Number1->NrOfDigits < Karatsuba_BOUND || Number2 ->NrOfDigits < Karatsuba_BOUND)
      return StandardMultiply(Number1,Number2);
 
    unsigned int SplitSize=0;
    unsigned int MaxLen1 = Number1->NrOfDigits;
    unsigned int MaxLen2 = Number2->NrOfDigits;
    if(Number1->NrOfDigits>Number2->NrOfDigits)
        SplitSize=Number1->NrOfDigits/2;
    else
        SplitSize=Number2->NrOfDigits/2;

    BigNumber *Low1INT, *High1INT, *Low2INT, *High2INT;
    if (MaxLen1 > SplitSize) 
    {
        unsigned int High1Size = MaxLen1 - SplitSize;
        
        char* Low1 = malloc(SplitSize + 1);
        memcpy(Low1, Number1->Digits, SplitSize);
        Low1[SplitSize] = '\0';
        
        char* High1 = malloc(High1Size + 1);
        memcpy(High1, Number1->Digits + SplitSize, High1Size);
        High1[High1Size] = '\0';

        // Transfer ownership of the malloc'd pointers directly to the struct
        Low1INT = PrivateConstructor(Low1, SplitSize, false);
        High1INT = PrivateConstructor(High1, High1Size, false);
    } 
    else 
    {
        // Number is too small for a high half. 
        char* Low1 = malloc(MaxLen1 + 1);
        memcpy(Low1, Number1->Digits, MaxLen1 + 1); // Copies the \0 too
        
        char* High1 = malloc(2);
        memcpy(High1, "0", 2); 

        Low1INT = PrivateConstructor(Low1, MaxLen1, false);
        High1INT = PrivateConstructor(High1, 1, false);
    }

    // Splitting Number 2
    if (MaxLen2 > SplitSize) 
    {
        unsigned int High2Size = MaxLen2 - SplitSize;
        
        char* Low2 = malloc(SplitSize + 1);
        memcpy(Low2, Number2->Digits, SplitSize);
        Low2[SplitSize] = '\0';
        
        char* High2 = malloc(High2Size + 1);
        memcpy(High2, Number2->Digits + SplitSize, High2Size);
        High2[High2Size] = '\0';

        Low2INT = PrivateConstructor(Low2, SplitSize, false);
        High2INT = PrivateConstructor(High2, High2Size, false);
    } 
    else 
    {
        char* Low2 = malloc(MaxLen2 + 1);
        memcpy(Low2, Number2->Digits, MaxLen2 + 1);
        
        char* High2 = malloc(2);
        memcpy(High2, "0", 2);

        Low2INT = PrivateConstructor(Low2, MaxLen2, false);
        High2INT = PrivateConstructor(High2, 1, false);
    }

    BigNumber* SumPair1=Sum(Low1INT,High1INT);
    BigNumber* SumPair2=Sum(Low2INT,High2INT);

    BigNumber *Z0 = NULL, *Z2 = NULL, *Z3 = NULL;
    if (NumberOfIterations > 0)
    {
        pthread_t ThreadZ0, ThreadZ2, ThreadZ3;
        
        unsigned short int NextIterations = NumberOfIterations - 1;
        ThreadArgumentKaratsuba argsZ0 = {Low1INT, Low2INT, NULL, NextIterations};
        ThreadArgumentKaratsuba argsZ2 = {High1INT, High2INT, NULL, NextIterations};
        ThreadArgumentKaratsuba argsZ3 = {SumPair1, SumPair2, NULL, NextIterations};

        pthread_create(&ThreadZ0, NULL, KaratsubaTheadFunc, &argsZ0);
        pthread_create(&ThreadZ2, NULL, KaratsubaTheadFunc, &argsZ2); 
        pthread_create(&ThreadZ3, NULL, KaratsubaTheadFunc, &argsZ3);

        pthread_join(ThreadZ0, NULL);
        pthread_join(ThreadZ2, NULL);
        pthread_join(ThreadZ3, NULL);

        Z0 = argsZ0.Result;
        Z2 = argsZ2.Result;
        Z3 = argsZ3.Result;
    }
    else
      {
        Z0=Karatsuba(Low1INT,Low2INT);  
        Z2=Karatsuba(High1INT,High2INT);
        Z3=Karatsuba(SumPair1,SumPair2);
      }
    
    FreeMemory(Low1INT);FreeMemory(Low2INT);FreeMemory(High1INT);
    FreeMemory(High2INT);FreeMemory(SumPair1);FreeMemory(SumPair2);

    BigNumber *AuxiliaryDifference=Subtract(Z3,Z2);
    BigNumber *AuxiliaryDifference2=Subtract(AuxiliaryDifference,Z0);
    ShiftRightNPositions(AuxiliaryDifference2,SplitSize);
    FreeMemory(AuxiliaryDifference);

    ShiftRightNPositions(Z2,SplitSize*2);
    BigNumber *TempRezult=Sum(Z2,AuxiliaryDifference2);
    BigNumber *Rezult=Sum(TempRezult,Z0);

    FreeMemory(TempRezult);FreeMemory(AuxiliaryDifference2);
    FreeMemory(Z2);FreeMemory(Z0);FreeMemory(Z3);
    
    Rezult->IsNegative = (Number1->IsNegative != Number2->IsNegative);
    return Rezult;
}

static BigNumber* KaratsubaSquared(BigNumber*Number)
{   
    if (Number->NrOfDigits == 1 && Number->Digits[0] == '0')
      return Init("0");
    
    if(Number->NrOfDigits <Karatsuba_BOUND)  //Base Case for Recursion
        return StandardSquare(Number);

    unsigned int MaxLen = Number->NrOfDigits;   //Split the number in half
    unsigned int SplitSize = MaxLen / 2;
    unsigned int HighSize = MaxLen - SplitSize;

    BigNumber *LowINT, *HighINT;
    
    char* Low = malloc(SplitSize + 1);
    memcpy(Low, Number->Digits, SplitSize);
    Low[SplitSize] = '\0';
    
    char* High = malloc(HighSize + 1);
    memcpy(High, Number->Digits + SplitSize, HighSize);
    High[HighSize] = '\0';

    LowINT = PrivateConstructor(Low, SplitSize, false);  //LowINT will take ownership of the string memory
    HighINT = PrivateConstructor(High, HighSize, false); //HighINT will take ownership of the string memory

    //Number= HighINT*(10^m)+LowINT | ()^2
    //Number^2 = HighINT^2*(10^2m) +2*HighINT*LowINT*10^m +LowINT^2
    //Notations  HighINT^2:=Z0 LowINT^2:=Z1 2*HighINT*LowINT:=Z2  (HighINT+LowINT)^2:=Z3
    //Observe that Z2=Z3-Z1-Z0
    //Observe that we can all of them are squares besides Z2 so we can make 3 recursive calls to speed up Z0,Z1,Z3
    //Final Result is Z1*10^2m +Z2*10^m +Z0

    BigNumber* Z0 = KaratsubaSquared(LowINT);  //recursive call
    BigNumber* Z1 = KaratsubaSquared(HighINT); //recursive call

    BigNumber* Z2 = Sum(LowINT, HighINT);
    FreeMemory(LowINT);
    FreeMemory(HighINT);
   
    BigNumber* Z3 = KaratsubaSquared(Z2); //recursive call

    BigNumber *AuxiliaryDifference = Subtract(Z3, Z1);
    BigNumber *AuxiliaryDifference2 = Subtract(AuxiliaryDifference, Z0); //Calculating Z2=Z3-Z1-X0
    SwapNumbersInMemory(&AuxiliaryDifference2,&Z2);
    ShiftRightNPositions(Z2, SplitSize);                                 //Calculating Z2*10^m

    FreeMemory(AuxiliaryDifference);
    FreeMemory(AuxiliaryDifference2);

    ShiftRightNPositions(Z1,SplitSize*2);                               //Calculating Z1*10^2m
    BigNumber *TempSum=Sum(Z1,Z2);
    BigNumber *Result=Sum(TempSum,Z0);                                  //Calculating Z1*10^2m + Z2*10^m +Z0  

    FreeMemory(TempSum);
    FreeMemory(Z0);
    FreeMemory(Z1);
    FreeMemory(Z2);
    FreeMemory(Z3);

    return Result;
}

static BigNumber* KaratsubaSquaredMultiThreaded(BigNumber* Number ,unsigned short int NumberOfIterations);
void* KaratsubaSquaredTheadFunc(void *argument)
{
    ThreadArgumentKaratsubaSquared* args=( ThreadArgumentKaratsubaSquared*)argument;
    args->Result = KaratsubaSquaredMultiThreaded(args->Number, args->Iterations); 
    return NULL;
}

static BigNumber*  KaratsubaSquaredMultiThreaded(BigNumber*Number,unsigned short int NumberOfIterations)
{
    if(Number->NrOfDigits <Karatsuba_BOUND)  //Base Case for Recursion
        return StandardSquare(Number);

    //We should only spawn at maximum 27 threads per number depending on its size and in the future if the system suport this level of multithreading
    //NumberOfIterations will assigned in the Multiply function

    unsigned int MaxLen = Number->NrOfDigits;   //Split the number in half
    unsigned int SplitSize = MaxLen / 2;
    unsigned int HighSize = MaxLen - SplitSize;

    BigNumber *LowINT, *HighINT;
    
    char* Low = malloc(SplitSize + 1);
    memcpy(Low, Number->Digits, SplitSize);
    Low[SplitSize] = '\0';
    
    char* High = malloc(HighSize + 1);
    memcpy(High, Number->Digits + SplitSize, HighSize);
    High[HighSize] = '\0';

    LowINT = PrivateConstructor(Low, SplitSize, false);  //LowINT will take ownership of the string memory
    HighINT = PrivateConstructor(High, HighSize, false); //HighINT will take ownership of the string memory

    //Number= HighINT*(10^m)+LowINT | ()^2
    //Number^2 = HighINT^2*(10^2m) +2*HighINT*LowINT*10^m +LowINT^2
    //Notations  HighINT^2:=Z0 LowINT^2:=Z1 2*HighINT*LowINT:=Z2  (HighINT+LowINT)^2:=Z3
    //Observe that Z2=Z3-Z1-Z0
    //Observe that we can all of them are squares besides Z2 so we can make 3 recursive calls to speed up Z0,Z1,Z3
    //Final Result is Z1*10^2m +Z2*10^m +Z0

    BigNumber* Z2 = Sum(LowINT, HighINT);

    BigNumber* Z0; BigNumber* Z1 ;BigNumber* Z3;
    if(NumberOfIterations>0)
      {
        //For every recursive call a thread will be created
        //To maintain a number of threads close to the hardware limit the NumberOfIterations should be between 1 and 3 
        //Number of threads in total is 3^(NumberOfIterations)

        pthread_t ThreadZ0, ThreadZ1, ThreadZ3;
        
        unsigned short int NextIterations = NumberOfIterations - 1;
        ThreadArgumentKaratsubaSquared argsZ0 = {LowINT, NULL, NextIterations};
        ThreadArgumentKaratsubaSquared argsZ1 = {HighINT, NULL, NextIterations};
        ThreadArgumentKaratsubaSquared argsZ3 = {Z2, NULL, NextIterations};

        pthread_create(&ThreadZ0, NULL, KaratsubaSquaredTheadFunc, &argsZ0);
        pthread_create(&ThreadZ1, NULL, KaratsubaSquaredTheadFunc, &argsZ1); 
        pthread_create(&ThreadZ3, NULL, KaratsubaSquaredTheadFunc, &argsZ3);

        pthread_join(ThreadZ0, NULL);
        pthread_join(ThreadZ1, NULL);
        pthread_join(ThreadZ3, NULL);

        Z0 = argsZ0.Result;
        Z1 = argsZ1.Result;
        Z3 = argsZ3.Result;
      }
    else
      {
        Z0 = KaratsubaSquared(LowINT);  //recursive call
        Z1 = KaratsubaSquared(HighINT); //recursive call
        Z3 = KaratsubaSquared(Z2);      //recursive call
      }

    FreeMemory(LowINT);
    FreeMemory(HighINT);

    BigNumber *AuxiliaryDifference = Subtract(Z3, Z1);
    BigNumber *AuxiliaryDifference2 = Subtract(AuxiliaryDifference, Z0); //Calculating Z2=Z3-Z1-X0
    SwapNumbersInMemory(&AuxiliaryDifference2,&Z2);
    ShiftRightNPositions(Z2, SplitSize);                                 //Calculating Z2*10^m

    FreeMemory(AuxiliaryDifference);
    FreeMemory(AuxiliaryDifference2);

    ShiftRightNPositions(Z1,SplitSize*2);                               //Calculating Z1*10^2m
    BigNumber *TempSum=Sum(Z1,Z2);
    BigNumber *Result=Sum(TempSum,Z0);                                  //Calculating Z1*10^2m + Z2*10^m +Z0  

    FreeMemory(TempSum);
    FreeMemory(Z0);
    FreeMemory(Z1);
    FreeMemory(Z2);
    FreeMemory(Z3);

    return Result;
}

//Implementation of Toom-Cook 3 way for Multiplication ~O(n^1.46)

//====================STEP 1 SPLIT
//Consider X,Y two integers
//First We need to Split the Numbers into 3 parts and construct 2 second degree polynomials
//P(t)=x_2t^2+x_1t+x_0 and Q(t)=y_2t^2+y_1t+y_0 where t=10^k
//Multiply the two to obtain R(t)=r_4t^4+r_3t^3+r_2t^2+r_1t^1+r_0

//====================STEP 2 EVALUATE AT 5 POINTS
//To find each r_i we need 5 t_i={0,1,-1,-2,inf} => 
//P(0)=x_0 , P(1)=x_2+x_1+x_0 ,P(-1)=x_2-x_1+x_0 ,P(-2)=4x_2-2x_1+x_0 P(inf)=x^2
//Q(0)=y_0 , Q(1)=y_2+y_1+y_0 ,Q(-1)=y_2-y_1+y_0 ,Q(-2)=4y_2-2y_1+y_0 Q(inf)=x^2

//====================STEP 3 RECURSION
//We get W(i)=P(i)*Q(i) for i={0,1,2,3,inf} ,each of these will be a recursive call
//At some defined threshold we can use either Karatsuba or StandardMultiplication

//====================STEP 4 SOLVE FOR COEFFICIENTS
//Finally we can fiind the coeff of R(i) in the following way:
//r_0=W(0) ,r_4=W(inf) ,r_2=(W(1)+W(-1))/2-R(0)-R(4) ,r_3=(16R(4)+4R(2)+W(-1)-W(1)+W(0)-W(-2))/6 ,r_1=(W(1)-W(-1))/2-R(3)

//====================STEP 5 RECOMPOSE
//Final result will be R(t)=r_410^4k+r_310^3k+r_210^2k+r_110^k+r_0

void MultiplyBy2(BigNumber* Number) //Modifies the NUMBER in MEMORY, DOENST RETURN A NEW ONE
{
    if (Number == NULL || Number->NrOfDigits == 0) return;

    //An O(NrOfDigits) algoritm to quickly find Number*2 in memory without any Auxiliary Memory and No Garbage Collection
    //Very Usefull in Newton-Raphson iterations for Inverse

    unsigned int carry = 0;
    for (unsigned int i = 0; i < Number->NrOfDigits; i++)
    {
        unsigned int current_digit = Number->Digits[i] - '0';
        unsigned int doubled = (current_digit << 1) + carry; // digit * 2 + carry
        
        Number->Digits[i] = (doubled % 10) + '0';
        carry = doubled / 10;
    }

    if (carry > 0)
    {
        Number->NrOfDigits++;
        Number->Digits = realloc(Number->Digits, Number->NrOfDigits + 1);
        if (Number->Digits == NULL)
        {
            perror("Memory reallocation failed in MultiplyBy2");
            exit(-1);
        }
        Number->Digits[Number->NrOfDigits - 1] = carry + '0';
        Number->Digits[Number->NrOfDigits] = '\0';
    }
}

void DivideBy2(BigNumber* X)
{
    if (X == NULL) return;

    int carry = 0;
    for (long int i =X->NrOfDigits - 1; i >= 0; i--)
    {
        int current_digit = X->Digits[i] - '0';
        int next_carry = (current_digit % 2 != 0) ? 5 : 0; 
        X->Digits[i] = (current_digit / 2) + carry + '0'; 
        carry = next_carry;
    }
    CleanTrailingZeros(X); 
}
void DivideBy6(BigNumber* X)
{
    if (X == NULL) return;
  
    int remainder = 0;

    for (long int i = X->NrOfDigits - 1; i >= 0; i--)
    {
        int current_val = (remainder * 10) + (X->Digits[i] - '0');
        X->Digits[i] = (current_val / 6) + '0';
        remainder = current_val % 6;
    }
    CleanTrailingZeros(X);
}

void DividByPowerOf10(BigNumber *Number, unsigned int Power) //Dividing an integer by a power of 10 (deleting last (power)digits from the number)
{
  if(Number==NULL || Power<=0) return ;

  if(Power>=Number->NrOfDigits) 
     {
       Number->Digits[0]='0';
       Number->Digits[1]='\0';
       Number->NrOfDigits=1;
       Number->IsNegative=false;
       return ;
     }

  unsigned int NewLenght = Number->NrOfDigits-Power;
    for (unsigned int i = 0; i < NewLenght; i++) 
    {
        Number->Digits[i] = Number->Digits[i + Power];
    }
    
    Number->Digits[NewLenght] = '\0';
    Number->NrOfDigits = NewLenght; 

   CleanTrailingZeros(Number);
}

static BigNumber* ExtractToomChunk(BigNumber* X, unsigned int start_index, unsigned int max_length) //O(N)
{
    // If the chunk starts outside the bounds of the number, the chunk is just "0"
    if (start_index >= X->NrOfDigits) 
    {
        return Init("0");
    }

    unsigned int actual_length = max_length;
    
    // If the chunk reaches past the end of the number, cap the length
    if (start_index + actual_length > X->NrOfDigits) 
    {
        actual_length = X->NrOfDigits - start_index;
    }

    while (actual_length > 1 && X->Digits[start_index + actual_length - 1] == '0') 
    {
        actual_length--;
    }

    BigNumber* Chunk = malloc(sizeof(BigNumber));
    if(Chunk==NULL)
      {
         perror("Allocating memory for Split Inside ExtractToomCook Failed");
         exit(-1);
      }
    Chunk->IsNegative = false; // Polynomial chunks are always evaluated as positive
    Chunk->NrOfDigits = actual_length;
    Chunk->Digits = malloc((actual_length + 1) * sizeof(char));

    memcpy(Chunk->Digits, X->Digits + start_index, actual_length);
    Chunk->Digits[actual_length] = '\0';

    return Chunk;
}

static void SplitToom3(BigNumber* Number, unsigned int k, BigNumber** x0, BigNumber** x1, BigNumber** x2) 
{
    if (Number == NULL) return;

    // x0 represents the lowest digits (10^0 to 10^k)
    *x0 = ExtractToomChunk(Number, 0, k);

    // x1 represents the middle digits (10^k to 10^2k)
    *x1 = ExtractToomChunk(Number, k, k);

    // x2 represents the highest digits (10^2k and beyond)
    *x2 = ExtractToomChunk(Number, 2 * k, k); 
}

static BigNumber *ToomCook3Way(BigNumber* X,BigNumber* Y)
{
  if ((X->NrOfDigits == 1 && X->Digits[0] == '0') || (Y->NrOfDigits == 1 && Y->Digits[0] == '0'))
    {
        return Init("0");
    }

  if(X->NrOfDigits<ToomCook3Way_BOUND || Y->NrOfDigits<ToomCook3Way_BOUND)
    {
      if (X->NrOfDigits < Karatsuba_BOUND || Y->NrOfDigits < Karatsuba_BOUND) 
        {
            return StandardMultiply(X, Y); 
        }
      return Karatsuba(X,Y);
    } 
  
  // Find the split (splits in 3 hence the name) size based on the largest number
    unsigned int MaxDigits = (X->NrOfDigits > Y->NrOfDigits) ? X->NrOfDigits : Y->NrOfDigits;
    unsigned int k = (MaxDigits + 2) / 3; // (Ceiling of MaxDigits)/3

    //Declare our 6 polynomial chunks
    BigNumber *X0, *X1, *X2;
    BigNumber *Y0, *Y1, *Y2;

    //STEP 1 SPLIT
    SplitToom3(X, k, &X0, &X1, &X2);
    SplitToom3(Y, k, &Y0, &Y1, &Y2);

    // STEPT 2 EVALUATION
    // We evaluate P(t) and Q(t) at points: 0, 1, -1, -2, and infinity

    // P(1) = X0 + X1 + X2   |   P(-1) = X0 - X1 + X2
    BigNumber *SumX0X2 = Sum(X0, X2);
    BigNumber *PX_1 = Sum(SumX0X2, X1);
    BigNumber *PX_minus1 = Subtract(SumX0X2, X1); 

    BigNumber *SumY0Y2 = Sum(Y0, Y2);
    BigNumber *PY_1 = Sum(SumY0Y2, Y1);
    BigNumber *PY_minus1 = Subtract(SumY0Y2, Y1);

    // P(-2) = X0 - 2*X1 + 4*X2
    BigNumber *TwoX1 = CloneBigNumber(X1);
    MultiplyBy2(TwoX1); 
    
    BigNumber *FourX2 = CloneBigNumber(X2);
    MultiplyBy2(FourX2); MultiplyBy2(FourX2);
    
    BigNumber *SumX0_FourX2 = Sum(X0, FourX2);
    BigNumber *PX_minus2 = Subtract(SumX0_FourX2, TwoX1);

    BigNumber *TwoY1 = CloneBigNumber(Y1);
    MultiplyBy2(TwoY1); 
    
    BigNumber *FourY2 = CloneBigNumber(Y2);
    MultiplyBy2(FourY2); MultiplyBy2(FourY2);
    
    BigNumber *SumY0_FourY2 = Sum(Y0, FourY2);
    BigNumber *PY_minus2 = Subtract(SumY0_FourY2, TwoY1);

    // STEP 3 RECURSION
    
    BigNumber *W_0 = ToomCook3Way(X0, Y0);
    BigNumber *W_inf = ToomCook3Way(X2, Y2); 
    BigNumber *W_1 = ToomCook3Way(PX_1, PY_1);
    BigNumber *W_minus1 = ToomCook3Way(PX_minus1, PY_minus1);
    BigNumber *W_minus2 = ToomCook3Way(PX_minus2, PY_minus2);


    FreeMemory(SumX0X2); FreeMemory(PX_1); FreeMemory(PX_minus1);
    FreeMemory(TwoX1); FreeMemory(FourX2); FreeMemory(SumX0_FourX2); FreeMemory(PX_minus2);
    
    FreeMemory(SumY0Y2); FreeMemory(PY_1); FreeMemory(PY_minus1);
    FreeMemory(TwoY1); FreeMemory(FourY2); FreeMemory(SumY0_FourY2); FreeMemory(PY_minus2);
    
    FreeMemory(X0); FreeMemory(X1); FreeMemory(X2);
    FreeMemory(Y0); FreeMemory(Y1); FreeMemory(Y2);

  
   //STEP 4 INTERPOLATION 

   // R0 = W_0 R4 = W_inf
    BigNumber *R0 = W_0;
    BigNumber *R4 = W_inf;

    // R2 = (W_1 + W_minus1)/2 - R0 - R4
    BigNumber *SumW1_Wminus1 = Sum(W_1, W_minus1);
    
    // IN-PLACE DIVISION: SumW1_Wminus1 becomes HalfSum!
    DivideBy2(SumW1_Wminus1); 
    
    BigNumber *HalfSum_Minus_R0 = Subtract(SumW1_Wminus1, R0);
    BigNumber *R2 = Subtract(HalfSum_Minus_R0, R4);

    // R3 = (16*R4 + 4*R2 + W_minus1 - W_1 + W_0 - W_minus2) / 6
    BigNumber *SixteenR4 = CloneBigNumber(R4);
    MultiplyBy2(SixteenR4); MultiplyBy2(SixteenR4); 
    MultiplyBy2(SixteenR4); MultiplyBy2(SixteenR4); 
    
    BigNumber *FourR2 = CloneBigNumber(R2);
    MultiplyBy2(FourR2); MultiplyBy2(FourR2);
    
    BigNumber *Term1 = Sum(SixteenR4, FourR2);
    BigNumber *Term2 = Sum(Term1, W_minus1);
    BigNumber *Term3 = Subtract(Term2, W_1);
    BigNumber *Term4 = Sum(Term3, W_0);
    
    BigNumber *NumeratorR3 = Subtract(Term4, W_minus2);
    
    // IN-PLACE DIVISION: NumeratorR3 BECOMES R3!
    DivideBy6(NumeratorR3); 
    BigNumber *R3 = NumeratorR3; // Map the pointer directly

    // 5. R1 = (W_1 - W_minus1)/2 - R3
    BigNumber *SubW1_Wminus1 = Subtract(W_1, W_minus1);
    DivideBy2(SubW1_Wminus1);
    
    BigNumber *R1 = Subtract(SubW1_Wminus1, R3);

    FreeMemory(SumW1_Wminus1); 
    FreeMemory(HalfSum_Minus_R0);
    FreeMemory(SixteenR4); 
    FreeMemory(FourR2);
    FreeMemory(Term1); 
    FreeMemory(Term2); 
    FreeMemory(Term3); 
    FreeMemory(Term4); 
    FreeMemory(SubW1_Wminus1); 
    
    FreeMemory(W_1); 
    FreeMemory(W_minus1); 
    FreeMemory(W_minus2);

    // PHASE 5: RECOMPOSITION (Shift and Add)
    // Result = R4*10^(4k) + R3*10^(3k) + R2*10^(2k) + R1*10^k + R0

    
    ShiftRightNPositions(R4, 4 * k);
    ShiftRightNPositions(R3, 3 * k);
    ShiftRightNPositions(R2, 2 * k);
    ShiftRightNPositions(R1, k);

    BigNumber *Res1 = Sum(R0, R1);
    BigNumber *Res2 = Sum(Res1, R2);
    BigNumber *Res3 = Sum(Res2, R3);
    BigNumber *FinalResult = Sum(Res3, R4);

    // Remove any leading zeros that might have formed
    CleanTrailingZeros(FinalResult);

    FreeMemory(R0); FreeMemory(R1); FreeMemory(R2); FreeMemory(R3); FreeMemory(R4);
    FreeMemory(Res1); FreeMemory(Res2); FreeMemory(Res3);

    FinalResult->IsNegative = (X->IsNegative != Y->IsNegative);
    return FinalResult;
}

static BigNumber* ToomCook3WayMultiThreaded(BigNumber* X, BigNumber* Y, unsigned short int NumberOfIterations);
void* ToomCook3WayTheadFunc(void *argument)
{
    ThreadArgumentToomCook *args = (ThreadArgumentToomCook*)argument;
    args->Result = ToomCook3WayMultiThreaded(args->Number1, args->Number2, args->Iterations); 
    return NULL;
}
static BigNumber *ToomCook3WayMultiThreaded(BigNumber* X, BigNumber *Y,unsigned short int NumberOfInterations)
{
   if ((X->NrOfDigits == 1 && X->Digits[0] == '0') || (Y->NrOfDigits == 1 && Y->Digits[0] == '0'))
    {
        return Init("0");
    }

   if(X->NrOfDigits<ToomCook3Way_BOUND || Y->NrOfDigits<ToomCook3Way_BOUND)
     {
      if (X->NrOfDigits < Karatsuba_BOUND || Y->NrOfDigits < Karatsuba_BOUND) 
        {
            return StandardMultiply(X, Y); 
        }
      return Karatsuba(X,Y);
     }
  
  // Find the split size based on the largest number
    unsigned int MaxDigits = (X->NrOfDigits > Y->NrOfDigits) ? X->NrOfDigits : Y->NrOfDigits;
    unsigned int k = (MaxDigits + 2) / 3; // (Ceiling of MaxDigits)/3

    //Declare our 6 polynomial chunks
    BigNumber *X0, *X1, *X2;
    BigNumber *Y0, *Y1, *Y2;

    //STEP 1 SPLIT
    SplitToom3(X, k, &X0, &X1, &X2);
    SplitToom3(Y, k, &Y0, &Y1, &Y2);

    // STEPT 2 EVALUATION
    // We evaluate P(t) and Q(t) at points: 0, 1, -1, -2, and infinity

    // P(1) = X0 + X1 + X2   |   P(-1) = X0 - X1 + X2
    BigNumber *SumX0X2 = Sum(X0, X2);
    BigNumber *PX_1 = Sum(SumX0X2, X1);
    BigNumber *PX_minus1 = Subtract(SumX0X2, X1); 

    BigNumber *SumY0Y2 = Sum(Y0, Y2);
    BigNumber *PY_1 = Sum(SumY0Y2, Y1);
    BigNumber *PY_minus1 = Subtract(SumY0Y2, Y1);

    // P(-2) = X0 - 2*X1 + 4*X2
    BigNumber *TwoX1 = CloneBigNumber(X1);
    MultiplyBy2(TwoX1); 
    
    BigNumber *FourX2 = CloneBigNumber(X2);
    MultiplyBy2(FourX2); MultiplyBy2(FourX2);
    
    BigNumber *SumX0_FourX2 = Sum(X0, FourX2);
    BigNumber *PX_minus2 = Subtract(SumX0_FourX2, TwoX1);

    BigNumber *TwoY1 = CloneBigNumber(Y1);
    MultiplyBy2(TwoY1); 
    
    BigNumber *FourY2 = CloneBigNumber(Y2);
    MultiplyBy2(FourY2); MultiplyBy2(FourY2);
    
    BigNumber *SumY0_FourY2 = Sum(Y0, FourY2);
    BigNumber *PY_minus2 = Subtract(SumY0_FourY2, TwoY1);

    // STEP 3 RECURSION --Maximum Number of Iterations should be 2 as NumberOfThreads being created is equal to 5^NrOfIterations
    BigNumber *W_0 = NULL;
    BigNumber *W_inf =  NULL; 
    BigNumber *W_1 = NULL;
    BigNumber *W_minus1 = NULL;
    BigNumber *W_minus2 = NULL;
    if(NumberOfInterations>0)
      {
        pthread_t ThreadW_0, ThreadW_inf, ThreadW_1, ThreadW_minus1, ThreadW_minus2;
        
        unsigned short int NextIterations = NumberOfInterations- 1;

        ThreadArgumentToomCook ThreadArgumentW_0 = {X0, Y0, NULL, NextIterations};
        ThreadArgumentToomCook ThreadArgumentW_inf = {X2, Y2, NULL, NextIterations};
        ThreadArgumentToomCook ThreadArgumentW_1 = {PX_1, PY_1, NULL, NextIterations};
        ThreadArgumentToomCook ThreadArgumentW_minus1 = {PX_minus1, PY_minus1, NULL, NextIterations};
        ThreadArgumentToomCook ThreadArgumentW_minus2 = {PX_minus2, PY_minus2, NULL, NextIterations};

        pthread_create(&ThreadW_0, NULL, ToomCook3WayTheadFunc, &ThreadArgumentW_0);
        pthread_create(&ThreadW_inf, NULL, ToomCook3WayTheadFunc, &ThreadArgumentW_inf); 
        pthread_create(&ThreadW_1, NULL, ToomCook3WayTheadFunc, &ThreadArgumentW_1);
        pthread_create(&ThreadW_minus1, NULL, ToomCook3WayTheadFunc, &ThreadArgumentW_minus1);
        pthread_create(&ThreadW_minus2, NULL, ToomCook3WayTheadFunc, &ThreadArgumentW_minus2);

        pthread_join(ThreadW_0, NULL);
        pthread_join(ThreadW_inf, NULL);
        pthread_join(ThreadW_1, NULL);
        pthread_join(ThreadW_minus1, NULL);
        pthread_join(ThreadW_minus2, NULL);

        W_0 = ThreadArgumentW_0.Result;
        W_inf = ThreadArgumentW_inf.Result;
        W_1 = ThreadArgumentW_1.Result;
        W_minus1 = ThreadArgumentW_minus1.Result;
        W_minus2 = ThreadArgumentW_minus2.Result;
      }
    else
      {
        W_0 = ToomCook3Way(X0, Y0);
        W_inf = ToomCook3Way(X2, Y2); 
        W_1 = ToomCook3Way(PX_1, PY_1);
        W_minus1 = ToomCook3Way(PX_minus1, PY_minus1);
        W_minus2 = ToomCook3Way(PX_minus2, PY_minus2);
      }

    FreeMemory(SumX0X2); FreeMemory(PX_1); FreeMemory(PX_minus1);
    FreeMemory(TwoX1); FreeMemory(FourX2); FreeMemory(SumX0_FourX2); FreeMemory(PX_minus2);
    
    FreeMemory(SumY0Y2); FreeMemory(PY_1); FreeMemory(PY_minus1);
    FreeMemory(TwoY1); FreeMemory(FourY2); FreeMemory(SumY0_FourY2); FreeMemory(PY_minus2);
    
    FreeMemory(X0); FreeMemory(X1); FreeMemory(X2);
    FreeMemory(Y0); FreeMemory(Y1); FreeMemory(Y2);

  
    //STEP 4 INTERPOLATION 
   
     BigNumber *R0 = W_0;
     BigNumber* R4 = W_inf;

    // R2 = (W_1 + W_minus1)/2 - R0 - R4
    BigNumber *SumW1_Wminus1 = Sum(W_1, W_minus1);
    DivideBy2(SumW1_Wminus1); 
    BigNumber *HalfSum_Minus_R0 = Subtract(SumW1_Wminus1, R0);
    BigNumber *R2 = Subtract(HalfSum_Minus_R0, R4);

    // R3 = (16*R4 + 4*R2 + W_minus1 - W_1 + W_0 - W_minus2) / 6
    BigNumber *SixteenR4 = CloneBigNumber(R4);
    MultiplyBy2(SixteenR4); MultiplyBy2(SixteenR4); 
    MultiplyBy2(SixteenR4); MultiplyBy2(SixteenR4); 
    
    BigNumber *FourR2 = CloneBigNumber(R2);
    MultiplyBy2(FourR2); MultiplyBy2(FourR2);
    
    BigNumber *Term1 = Sum(SixteenR4, FourR2);
    BigNumber *Term2 = Sum(Term1, W_minus1);
    BigNumber *Term3 = Subtract(Term2, W_1);
    BigNumber *Term4 = Sum(Term3, W_0);
    BigNumber *NumeratorR3 = Subtract(Term4, W_minus2);
    DivideBy6(NumeratorR3); 
    BigNumber *R3 = NumeratorR3; 

    //R1 = (W_1 - W_minus1)/2 - R3
    BigNumber *SubW1_Wminus1 = Subtract(W_1, W_minus1);
    DivideBy2(SubW1_Wminus1);
    BigNumber *R1 = Subtract(SubW1_Wminus1, R3);

    FreeMemory(SumW1_Wminus1); 
    FreeMemory(HalfSum_Minus_R0);
    FreeMemory(SixteenR4); 
    FreeMemory(FourR2);
    FreeMemory(Term1); 
    FreeMemory(Term2); 
    FreeMemory(Term3); 
    FreeMemory(Term4); 
    FreeMemory(SubW1_Wminus1); 
    
    FreeMemory(W_1); 
    FreeMemory(W_minus1); 
    FreeMemory(W_minus2); 

    // STEP 5: RECOMPOSITION
    // Result = R4*10^(4k) + R3*10^(3k) + R2*10^(2k) + R1*10^k + R0

    ShiftRightNPositions(R4, 4 * k);
    ShiftRightNPositions(R3, 3 * k);
    ShiftRightNPositions(R2, 2 * k);
    ShiftRightNPositions(R1, k);

    BigNumber *Res1 = Sum(R0, R1);
    BigNumber *Res2 = Sum(Res1, R2);
    BigNumber *Res3 = Sum(Res2, R3);
    BigNumber *FinalResult = Sum(Res3, R4);

    CleanTrailingZeros(FinalResult);

    FreeMemory(R0); FreeMemory(R1); FreeMemory(R2); FreeMemory(R3); FreeMemory(R4);
    FreeMemory(Res1); FreeMemory(Res2); FreeMemory(Res3);
    
    FinalResult->IsNegative = (X->IsNegative != Y->IsNegative);
    return FinalResult;
}

//Optimezed Implementation of ToomCook3Way for Squaring a BigNumber 
//Complexity is half the memory and time needed for the classical ToomCook3Way
//We only construct one second degree polynomial P(t)=Q(t) and the coeff will also be squares as W(i)=P(i)^2 then we make 5 reccursive ToomCook3WaySquared calls
//====================STEP 1 SPLIT
//Consider X:=Number
//First We need to Split the Number into 3 parts and construct 2 second degree polynomial
//P(t)=x_2t^2+x_1t+x_0
//Square the polynomial to obtain R(t)=r_4t^4+r_3t^3+r_2t^2+r_1t^1+r_0

//====================STEP 2 EVALUATE AT 5 POINTS
//To find each r_i we need 5 t_i={0,1,-1,-2,inf} => 
//P(0)=x_0 , P(1)=x_2+x_1+x_0 ,P(-1)=x_2-x_1+x_0 ,P(-2)=4x_2-2x_1+x_0 P(inf)=x^2

//====================STEP 3 RECURSION
//We get W(i)=P(i)*P(i) for i={0,1,2,3,inf} ,each of these will be a recursive call
//At some defined threshold we can use either KaratsubaSquared or StandardMultiplicationSquared

//====================STEP 4 SOLVE FOR COEFFICIENTS
//Finally we can fiind the coeff of R(i) in the following way:
//r_0=W(0) ,r_4=W(inf) ,r_2=(W(1)+W(-1))/2-R(0)-R(4) ,r_3=(16R(4)+4R(2)+W(-1)-W(1)+W(0)-W(-2))/6 ,r_1=(W(1)-W(-1))/2-R(3)

//====================STEP 5 RECOMPOSE
//Final result will be R(t)=r_410^4k+r_310^3k+r_210^2k+r_110^k+r_0

static BigNumber *ToomCook3WaySquared(BigNumber *X)
{
    if(X->NrOfDigits==1 && X->Digits[0]=='0')
    {
      return Init("0");
    }

    if(X->NrOfDigits <ToomCook3Way_BOUND)
       {
        if (X->NrOfDigits < Karatsuba_BOUND ) 
        {
            return StandardSquare(X); 
        }

        return KaratsubaSquared(X);
      }

    unsigned int k = (X->NrOfDigits + 2) / 3; // (Ceiling of X->NrOfDigits)/3

    //Declare our 6 polynomial chunks
    BigNumber *X0, *X1, *X2;

    //STEP 1 SPLIT
    SplitToom3(X, k, &X0, &X1, &X2);

    // STEPT 2 EVALUATION
    // We evaluate P(t) at points: 0, 1, -1, -2, and infinity

    // P(1) = X0 + X1 + X2   |   P(-1) = X0 - X1 + X2
    BigNumber *SumX0X2 = Sum(X0, X2);
    BigNumber *PX_1 = Sum(SumX0X2, X1);
    BigNumber *PX_minus1 = Subtract(SumX0X2, X1); 

    // P(-2) = X0 - 2*X1 + 4*X2
    BigNumber *TwoX1 = CloneBigNumber(X1);
    MultiplyBy2(TwoX1);  
    BigNumber *FourX2 = CloneBigNumber(X2);
    MultiplyBy2(FourX2); MultiplyBy2(FourX2); 
    BigNumber *SumX0_FourX2 = Sum(X0, FourX2);
    BigNumber *PX_minus2 = Subtract(SumX0_FourX2, TwoX1);

    // STEP 3 RECURSION
    
    BigNumber *W_0 = ToomCook3WaySquared(X0);
    BigNumber *W_inf = ToomCook3WaySquared(X2); 
    BigNumber *W_1 = ToomCook3WaySquared(PX_1);
    BigNumber *W_minus1 = ToomCook3WaySquared(PX_minus1);
    BigNumber *W_minus2 = ToomCook3WaySquared(PX_minus2);

    FreeMemory(SumX0X2); FreeMemory(PX_1); FreeMemory(PX_minus1);
    FreeMemory(TwoX1); FreeMemory(FourX2); FreeMemory(SumX0_FourX2); FreeMemory(PX_minus2);
    FreeMemory(X0); FreeMemory(X1); FreeMemory(X2);

    //STEPT 4 INTERPOLATION 
    // R0 = W_0   R4 = W_inf
    BigNumber *R0 =W_0;
    BigNumber *R4 =W_inf;

    // R2 = (W_1 + W_minus1)/2 - R0 - R4
    BigNumber *SumW1_Wminus1 = Sum(W_1, W_minus1);
    DivideBy2(SumW1_Wminus1); 
    BigNumber *HalfSum_Minus_R0 = Subtract(SumW1_Wminus1, R0);
    BigNumber *R2 = Subtract(HalfSum_Minus_R0, R4);

    // R3 = (16*R4 + 4*R2 + W_minus1 - W_1 + W_0 - W_minus2) / 6
    BigNumber *SixteenR4 = CloneBigNumber(R4);
    MultiplyBy2(SixteenR4); MultiplyBy2(SixteenR4); 
    MultiplyBy2(SixteenR4); MultiplyBy2(SixteenR4);    
    BigNumber *FourR2 = CloneBigNumber(R2);
    MultiplyBy2(FourR2); MultiplyBy2(FourR2);
    
    BigNumber *Term1 = Sum(SixteenR4, FourR2);
    BigNumber *Term2 = Sum(Term1, W_minus1);
    BigNumber *Term3 = Subtract(Term2, W_1);
    BigNumber *Term4 = Sum(Term3, W_0);
    
    BigNumber *NumeratorR3 = Subtract(Term4, W_minus2);
    DivideBy6(NumeratorR3); 
    BigNumber *R3 = NumeratorR3;

    // 5. R1 = (W_1 - W_minus1)/2 - R3
    BigNumber *SubW1_Wminus1 = Subtract(W_1, W_minus1);
    DivideBy2(SubW1_Wminus1);
    
    BigNumber *R1 = Subtract(SubW1_Wminus1, R3);

    FreeMemory(SumW1_Wminus1); 
    FreeMemory(HalfSum_Minus_R0);
    FreeMemory(SixteenR4); 
    FreeMemory(FourR2);
    FreeMemory(Term1); 
    FreeMemory(Term2); 
    FreeMemory(Term3); 
    FreeMemory(Term4); 
    FreeMemory(SubW1_Wminus1); 
    
    FreeMemory(W_1); 
    FreeMemory(W_minus1); 
    FreeMemory(W_minus2);

    // PHASE 5: RECOMPOSITION (Shift and Add)
    // Result = R4*10^(4k) + R3*10^(3k) + R2*10^(2k) + R1*10^k + R0
   
    ShiftRightNPositions(R4, 4 * k);
    ShiftRightNPositions(R3, 3 * k);
    ShiftRightNPositions(R2, 2 * k);
    ShiftRightNPositions(R1, k);

    BigNumber *Res1 = Sum(R0, R1);
    BigNumber *Res2 = Sum(Res1, R2);
    BigNumber *Res3 = Sum(Res2, R3);
    BigNumber *FinalResult = Sum(Res3, R4);

    CleanTrailingZeros(FinalResult);

    FreeMemory(R0); FreeMemory(R1); FreeMemory(R2); FreeMemory(R3); FreeMemory(R4);
    FreeMemory(Res1); FreeMemory(Res2); FreeMemory(Res3);

    return FinalResult;
}

//A MultiThreaded Version to ToomCook3WaySquared for really big numbers
//Again the number of threads are growing exponentially (5^NumberOfIterations) 
//With that being said on most Systems, NumberOfIterations should be 1 (or 2 on newer Intel CPU`s or 3 on ThreadRipper Systems)
static BigNumber *ToomCook3WaySquareMultiThreaded(BigNumber* X, unsigned short int NumberOfIterations);
void* ToomCook3WaySquareThreadFunc(void *argument)
{
    ThreadArgumentToomCookSquared* args=(ThreadArgumentToomCookSquared*)argument;
    args->Result = ToomCook3WaySquareMultiThreaded(args->Number, args->Iterations); 
    return NULL;
}
static BigNumber *ToomCook3WaySquareMultiThreaded(BigNumber* X, unsigned short int NumberOfIterations)
{
    if(X->NrOfDigits==1 && X->Digits[0]=='0')
    {
      return Init("0");
    }

    if(X->NrOfDigits < ToomCook3Way_BOUND)
        { 
          if(X->NrOfDigits < Karatsuba_BOUND)
            {
              return StandardSquare(X);
            }
          return KaratsubaSquared(X);
        }
  
    unsigned int k = (X->NrOfDigits + 2) / 3;

    BigNumber *X0, *X1, *X2;
    SplitToom3(X, k, &X0, &X1, &X2);

    // ==========================================================
    // STEP 2: EVALUATION 
    // ==========================================================
    
    // P(1) = X0 + X1 + X2   |   P(-1) = X0 - X1 + X2
    BigNumber *SumX0X2 = Sum(X0, X2);
    BigNumber *PX_1 = Sum(SumX0X2, X1);
    BigNumber *PX_minus1 = Subtract(SumX0X2, X1); 

    // P(-2) = X0 - 2*X1 + 4*X2
    MultiplyBy2(X1); //
    BigNumber *FourX2 = CloneBigNumber(X2);
    MultiplyBy2(FourX2); MultiplyBy2(FourX2);
    BigNumber *SumX0_FourX2 = Sum(X0, FourX2);
    BigNumber *PX_minus2 = Subtract(SumX0_FourX2, X1);

    // ==========================================================
    // STEP 3: RECURSIVE SQUARING
    // ==========================================================
    BigNumber *W_0 = NULL, *W_inf = NULL, *W_1 = NULL, *W_minus1 = NULL, *W_minus2 = NULL;
    
    if(NumberOfIterations > 0)
    {
        pthread_t ThreadW_0, ThreadW_inf, ThreadW_1, ThreadW_minus1, ThreadW_minus2;
        unsigned short int NextIterations = NumberOfIterations - 1;

        ThreadArgumentToomCookSquared ArgW_0 = {X0, NULL, NextIterations};
        ThreadArgumentToomCookSquared ArgW_inf = {X2, NULL, NextIterations};
        ThreadArgumentToomCookSquared ArgW_1 = {PX_1, NULL, NextIterations};
        ThreadArgumentToomCookSquared ArgW_minus1 = {PX_minus1, NULL, NextIterations};
        ThreadArgumentToomCookSquared ArgW_minus2 = {PX_minus2, NULL, NextIterations};

        if(pthread_create(&ThreadW_0, NULL, ToomCook3WaySquareThreadFunc, &ArgW_0)!=0) 
        {
          perror("Creating Thread FAILED");
          exit(-2);
        }
        if(pthread_create(&ThreadW_inf, NULL, ToomCook3WaySquareThreadFunc, &ArgW_inf)!=0) 
        {
          perror("Creating Thread FAILED");
          exit(-2);
        }
        if(pthread_create(&ThreadW_1, NULL, ToomCook3WaySquareThreadFunc, &ArgW_1)!=0) 
        {
          perror("Creating Thread FAILED");
          exit(-2);
        }
        if(pthread_create(&ThreadW_minus1, NULL, ToomCook3WaySquareThreadFunc, &ArgW_minus1)!=0) {
          perror("Creating Thread FAILED");
          exit(-2);
        }
        if(pthread_create(&ThreadW_minus2, NULL, ToomCook3WaySquareThreadFunc, &ArgW_minus2)!=0) 
        {
          perror("Creating Thread FAILED");
          exit(-2);
        }

        pthread_join(ThreadW_0, NULL);
        pthread_join(ThreadW_inf, NULL);
        pthread_join(ThreadW_1, NULL);
        pthread_join(ThreadW_minus1, NULL);
        pthread_join(ThreadW_minus2, NULL);

        W_0 = ArgW_0.Result;
        W_inf = ArgW_inf.Result;
        W_1 = ArgW_1.Result;
        W_minus1 = ArgW_minus1.Result;
        W_minus2 = ArgW_minus2.Result;
    }
    else
    {
        W_0 = ToomCook3WaySquared(X0);
        W_inf = ToomCook3WaySquared(X2); 
        W_1 = ToomCook3WaySquared(PX_1);
        W_minus1 = ToomCook3WaySquared(PX_minus1);
        W_minus2 = ToomCook3WaySquared(PX_minus2);
    }

    FreeMemory(SumX0X2); FreeMemory(PX_1); FreeMemory(PX_minus1);
    FreeMemory(FourX2); FreeMemory(SumX0_FourX2); FreeMemory(PX_minus2);
    FreeMemory(X0); FreeMemory(X1); FreeMemory(X2);

    //STEPT 4 INTERPOLATION 
    // R0 = W_0   R4 = W_inf
    BigNumber *R0 = W_0;
    BigNumber *R4 = W_inf;

    // R2 = (W_1 + W_minus1)/2 - R0 - R4
    BigNumber *SumW1_Wminus1 = Sum(W_1, W_minus1);
    DivideBy2(SumW1_Wminus1); 
    BigNumber *HalfSum_Minus_R0 = Subtract(SumW1_Wminus1, R0);
    BigNumber *R2 = Subtract(HalfSum_Minus_R0, R4);

    // R3 = (16*R4 + 4*R2 + W_minus1 - W_1 + W_0 - W_minus2) / 6
    BigNumber *SixteenR4 = CloneBigNumber(R4);
    MultiplyBy2(SixteenR4); MultiplyBy2(SixteenR4); 
    MultiplyBy2(SixteenR4); MultiplyBy2(SixteenR4);    
    BigNumber *FourR2 = CloneBigNumber(R2);
    MultiplyBy2(FourR2); MultiplyBy2(FourR2);
    
    BigNumber *Term1 = Sum(SixteenR4, FourR2);
    BigNumber *Term2 = Sum(Term1, W_minus1);
    BigNumber *Term3 = Subtract(Term2, W_1);
    BigNumber *Term4 = Sum(Term3, W_0);
    
    BigNumber *NumeratorR3 = Subtract(Term4, W_minus2);
    DivideBy6(NumeratorR3); 
    BigNumber *R3 = NumeratorR3;

    // 5. R1 = (W_1 - W_minus1)/2 - R3
    BigNumber *SubW1_Wminus1 = Subtract(W_1, W_minus1);
    DivideBy2(SubW1_Wminus1);
    
    BigNumber *R1 = Subtract(SubW1_Wminus1, R3);

    FreeMemory(SumW1_Wminus1); 
    FreeMemory(HalfSum_Minus_R0);
    FreeMemory(SixteenR4); 
    FreeMemory(FourR2);
    FreeMemory(Term1); 
    FreeMemory(Term2); 
    FreeMemory(Term3); 
    FreeMemory(Term4); 
    FreeMemory(SubW1_Wminus1); 
    
    FreeMemory(W_1); 
    FreeMemory(W_minus1); 
    FreeMemory(W_minus2);

    // PHASE 5: RECOMPOSITION (Shift and Add)
    // Result = R4*10^(4k) + R3*10^(3k) + R2*10^(2k) + R1*10^k + R0
   
    ShiftRightNPositions(R4, 4 * k);
    ShiftRightNPositions(R3, 3 * k);
    ShiftRightNPositions(R2, 2 * k);
    ShiftRightNPositions(R1, k);

    BigNumber *Res1 = Sum(R0, R1);
    BigNumber *Res2 = Sum(Res1, R2);
    BigNumber *Res3 = Sum(Res2, R3);
    BigNumber *FinalResult = Sum(Res3, R4);

    CleanTrailingZeros(FinalResult);

    FreeMemory(R0); FreeMemory(R1); FreeMemory(R2); FreeMemory(R3); FreeMemory(R4);
    FreeMemory(Res1); FreeMemory(Res2); FreeMemory(Res3);

    return FinalResult;
}

BigNumber* Multiply(BigNumber* Number1,BigNumber*Number2)
{
    if (Number1 == NULL || Number2 == NULL) return NULL;  //Consider Edge Cases such as NULL,Multiply by +-1, 0

    BigNumber *Zero=Init("0");
    if(IsEqual(Number1,Zero)==true || IsEqual(Number2,Zero))
      {
         return Zero;
      }
    FreeMemory(Zero);

    BigNumber *One=Init("1");
    if(IsEqual(Number1,One)==true)
      {
        BigNumber *CloneNumber2=CloneBigNumber(Number2);
        FreeMemory(One);
        return CloneNumber2;
      }
    if(IsEqual(Number2,One)==true)
      {
         BigNumber *CloneNumber1=CloneBigNumber(Number1);
         FreeMemory(One);
         return CloneNumber1;
      }
    FreeMemory(One);

    BigNumber *NegativeOne=Init("-1");
    if(IsEqual(Number2,NegativeOne)==true)
      {
        BigNumber *CloneNumber1=CloneBigNumber(Number1);
        MultiplyByNegativeOne(CloneNumber1);
        FreeMemory(NegativeOne);
        return CloneNumber1;
      }
    if(IsEqual(Number1,NegativeOne)==true)
      {
         BigNumber *CloneNumber2=CloneBigNumber(Number2);
         MultiplyByNegativeOne(CloneNumber2);
         FreeMemory(NegativeOne);
         return CloneNumber2;
      }
    FreeMemory(NegativeOne);

    unsigned int MaxDigits=(Number1->NrOfDigits > Number2->NrOfDigits) ? Number1->NrOfDigits : Number2->NrOfDigits; 
    unsigned short int NumberOfIterations=0;
    
    if (MaxDigits>500000) 
    {
      NumberOfIterations = 2; // 25 New Threads for astronomical numbers
    } else if (MaxDigits > 50000) 
    {
      NumberOfIterations = 1; // 5 New Threads for heavy numbers
    } else 
    {
      NumberOfIterations = 0; // 0 New Threads (Sequential) for standard massive numbers
    }

    if(IsEqual(Number1,Number2)==true)
      {
        BigNumber *Result=ToomCook3WaySquareMultiThreaded(Number1,NumberOfIterations);
        return Result;
      }

    BigNumber *Result=ToomCook3WayMultiThreaded(Number1,Number2,NumberOfIterations);
    
    // Toom-Cook and Karatsuba evaluate only the absolute values of the magnitudes
    // Positive * Negative = Negative and  Negative * Negative = Positive
    if(Result != NULL && !(Result->NrOfDigits == 1 && Result->Digits[0] == '0')) //If one of the numbers is negative ,change sign
    {
      Result->IsNegative = Number1->IsNegative ^ Number2->IsNegative;
    }

    return Result;
}

BigFloatNumber* MultiplyFloat(BigFloatNumber *Number1,BigFloatNumber *Number2) //Mantissa can be treated as an BigINT than the result its just Multiply(Mantissa1,Mantissa2)*10^(Exponent1+Exponent2)
{
    if (Number1 == NULL || Number2 == NULL) return NULL;

    BigNumber *Mantissa=Multiply(Number1->Mantissa,Number2->Mantissa);
    long int Exponent=Number1->Exponent+Number2->Exponent;
    BigFloatNumber *Number=PrivateConstructorFloat(Mantissa,Exponent);
    CompressFloatInPlace(Number);
    return Number;
}

uint32_t BigNumberToUINT32(BigNumber* Number)
{
    if (Number == NULL || Number->Digits == NULL) return 0;
    
    uint32_t Result = 0;
    for (long int i = Number->NrOfDigits - 1; i >= 0; i--) 
    {
        Result = Result * 10 + (Number->Digits[i] - '0');
    }
    
    return Result;
}

int32_t BigNumberToINT32(BigNumber* Number)
{
    if (Number == NULL || Number->Digits == NULL) return 0;
    
    int32_t Result = 0;
    for (long int i = Number->NrOfDigits - 1; i >= 0; i--) 
    {
        Result = Result * 10 + (Number->Digits[i] - '0');
    }
    
    if(Number->IsNegative)
      Result*=-1;

    return Result;
}

uint64_t BigNumberToUINT64(BigNumber* Number)
{
    if (Number == NULL || Number->Digits == NULL) return 0;
    
    uint64_t Result = 0;
    for (long int i = Number->NrOfDigits - 1; i >= 0; i--) 
    {
        Result = Result * 10 + (Number->Digits[i] - '0');
    }
    
    return Result;
}

int64_t BigNumberToINT64(BigNumber* Number)
{
    if (Number == NULL || Number->Digits == NULL) return 0;
    
    int64_t Result = 0;
    for (long int i = Number->NrOfDigits - 1; i >= 0; i--) 
    {
        Result = Result * 10 + (Number->Digits[i] - '0');
    }
    
    if(Number->IsNegative)
      Result*=-1;

    return Result;
}

#if (SUPPORT_UINT128)
 __uint128_t BigNumberToUINT128(BigNumber* Number)
  {
    if (Number == NULL || Number->Digits == NULL) return 0;
    
    __uint128_t Result = 0;
    for (long int i = Number->NrOfDigits - 1; i >= 0; i--) 
    {
        Result = Result * 10 + (Number->Digits[i] - '0');
    }
    
    return Result;
 }

 __int128_t BigNumberToINT128(BigNumber* Number)
 {
    if (Number == NULL || Number->Digits == NULL) return 0;
    
    __int128_t Result = 0;
    for (long int i = Number->NrOfDigits - 1; i >= 0; i--) 
    {
        Result = Result * 10 + (Number->Digits[i] - '0');
    }
    
    if(Number->IsNegative)
      Result*=(__int128_t)-1;

    return Result;
 }
#endif

BigNumber* FromUnsignedIntegerToBigNum(unsigned int Number)
{
    if (Number== 0) 
    {
        char* Digits = malloc(sizeof(char)*2);
        Digits[0] = '0'; Digits[1] = '\0';
        return PrivateConstructor(Digits, 1, false);
    }

    //MAX VALUE OF UNSIGNED INT 4,294,967,295 - 10 digits + '\0' = 11 bytes
    char* Digits = malloc(sizeof(char)*11);
    short int NrOfDigits = 0;
    if(Digits==NULL)
      {
        perror("Allocating memory for Digits in FromUnsignedIntegerToBigNum failed");
        exit(-1);
      }

    while(Number> 0) 
    {
        Digits[NrOfDigits] = (Number% 10) + '0';
        Number /= 10;
        NrOfDigits++;
    }
    
    Digits[NrOfDigits] = '\0';
    
    // Unsigned is ALWAYS positive (false)
    return PrivateConstructor(Digits, NrOfDigits, false); 
}

BigNumber* FromUnsignedLongLongToBigNum(unsigned long long int Number)
{
    if (Number==0) 
    {
        char* Digits = malloc(sizeof(char)*2);
        Digits[0] = '0'; Digits[1] = '\0';
        return PrivateConstructor(Digits, 1, false);
    }

    //MAX VALUE OF unsigned long long int 18,446,744,073,709,551,615 - 20 digits + '\0' = 21 bytes
    char* Digits = malloc(sizeof(char)*21);
    if(Digits==NULL)
      {
        perror("Allocating memory for Digits in FromUnsignedLongLongToBigNum failed");
        exit(-1);
      }
    short int NrOfDigits = 0;
    
    while(Number>0) 
    {
        Digits[NrOfDigits] = (Number% 10) + '0';
        Number/= 10;
        NrOfDigits++;
    }
    
    Digits[NrOfDigits] = '\0';
    return PrivateConstructor(Digits, NrOfDigits, false); 
}

BigNumber* FromUnsignedInt128ToBigNum(__uint128_t Number)
{
    if (Number==0) 
    {
        char* Digits = malloc(sizeof(char)*2);
        Digits[0] = '0'; Digits[1] = '\0';
        return PrivateConstructor(Digits, 1, false);
    }

    // MAX VALUE OF UINT128 340,282,366,920,938,463,463,374,607,431,768,211,455 - 39 digits + '\0' = 40 bytes
    char* Digits = malloc(sizeof(char)*40);
    short int NrOfDigits = 0;
    if(Digits==NULL)
      {
        perror("Allocating memory for Digits in FromUnsignedInt128ToBigNum failed");
        exit(-1);
      }
    
    while(Number>0) 
    {
        Digits[NrOfDigits] = (char)(Number% 10) + '0'; 
        Number/= 10;
        NrOfDigits++;
    }
    
    Digits[NrOfDigits] = '\0';
    return PrivateConstructor(Digits, NrOfDigits, false);  
}

BigNumber* FromSignedIntegerToBigNum(int Number)
{
    if (Number==0) 
    {
        char* Digits = malloc(sizeof(char)*2);
        Digits[0] = '0'; Digits[1] = '\0';
        return PrivateConstructor(Digits, 1, false);
    }

    bool IsNegative = (Number<0);
    
    // Cast to unsigned FIRST to prevent INT_MIN overflow crash!
    unsigned int AbsoluteValueNumber= IsNegative ? (unsigned int)(-(Number)) : (unsigned int)Number;

    // MAX VALUE OF INT -2,147,483,648 - 10 digits + '\0' = 11 bytes
    char* Digits = malloc(sizeof(char)*11);
    short int NrOfDigits = 0;
    if(Digits==NULL)
      {
        perror("Allocating memory for Digits in FromSignedIntegerToBigNum failed");
        exit(-1);
      }
    
    while(AbsoluteValueNumber> 0) 
    {
        Digits[NrOfDigits] = (AbsoluteValueNumber% 10) + '0';
        AbsoluteValueNumber/= 10;
        NrOfDigits++;
    }
    
    Digits[NrOfDigits] = '\0';
    return PrivateConstructor(Digits, NrOfDigits, IsNegative);
}

static BigNumber* LongDivision(BigNumber* Dividend, BigNumber* Divisor,BigNumber **Remainder)  //Time Complexity O(Divident.size * Divizor.size) 
{
    //LongDivision will always get positive numbers with restriction Divisior != 0 and Divident > Divizor
  
    char* QuotientString = calloc(Dividend->NrOfDigits + 1, sizeof(char));
    BigNumber* CurrentRemainder = Init("0");

    for (int i = Dividend->NrOfDigits - 1; i >= 0; i--)
    {
        ShiftRightNPositions(CurrentRemainder,1);
        CurrentRemainder->Digits[0]=Dividend->Digits[i];
        CleanTrailingZeros(CurrentRemainder);
        int quotient_digit = 0;
        while (BigNumberCompareAbsoluteValue(CurrentRemainder, Divisor) >= 0)
        {
            BigNumber* NextRemainder = Subtract(CurrentRemainder, Divisor);
            FreeMemory(CurrentRemainder);
            CurrentRemainder = NextRemainder;
            quotient_digit++;
        }

        QuotientString[i] = quotient_digit + '0';
    }

    //Clean up trailing zeros in the Quotient string
    unsigned int ActualDigits = Dividend->NrOfDigits;
    while (ActualDigits > 1 && QuotientString[ActualDigits - 1] == '0')
    {
        ActualDigits--;
    }
    QuotientString[ActualDigits] = '\0';


    BigNumber* FinalQuotient = PrivateConstructor(QuotientString, ActualDigits,false);
    
    if(Remainder==NULL)
      {
        FreeMemory(CurrentRemainder);
      }
    else
      {
        //If user wants to retain the Remaider we take ownership from CurentRemainder
        *Remainder=CurrentRemainder;
      }
    
    return FinalQuotient;
}

void MultiplyBySingleDigit(BigNumber *Number, short int Digit)
{
   if(Number==NULL || Digit>10) return;

   if(Digit==0)
    {
        Number->Digits[0] = '0';
        Number->Digits[1] = '\0';
        Number->NrOfDigits = 1;
        Number->IsNegative = false;
        return;
    }
  
  unsigned int Carry = 0;
  for(unsigned int i=0;i<Number->NrOfDigits;i++)
    {
       unsigned int Prod=(Number->Digits[i]-'0')*Digit +Carry;
       Number->Digits[i] =(Prod % 10)+'0';
       Carry = Prod / 10;
    }
  
  if(Carry>0)
    {
        char *ExpandedDigits=realloc(Number->Digits,sizeof(char)*(Number->NrOfDigits+2));
        if(ExpandedDigits==NULL)
          {
            perror("Allocating Memory inside Multiply by Single Digit in case of overflow failed\n");
            exit(-1);
          }
        Number->Digits=ExpandedDigits;
        Number->Digits[Number->NrOfDigits] =Carry+'0';
        Number->NrOfDigits++;
        Number->Digits[Number->NrOfDigits] ='\0';
    }
}

void DivideBySingleDigit(BigNumber* Number, unsigned int Divisor) 
{
    if (Number == NULL || Divisor == 0) return ;
    if (Divisor == 1) return;

    unsigned long long Remainder = 0;
    for (long int i = Number->NrOfDigits - 1; i >= 0; i--) 
    {
        Remainder = (Remainder * 10) + (Number->Digits[i] - '0');
        Number->Digits[i] = (Remainder / Divisor) + '0';
        Remainder = Remainder % Divisor;
    }

    CleanTrailingZeros(Number);
}

static void NormalizeBurnikelZiegler(BigNumber* CloneDividend, BigNumber* CloneDivisor,int *OutScalar, int* OutPad)
{
    //In order to use Burnikel-Ziegler we need MSD(Divisor) >=5
    //We will multiply both numbers by a scallar factor c=floor(10/(MSD+1))

    unsigned int MSD = CloneDivisor->Digits[CloneDivisor->NrOfDigits-1] - '0';
    *OutScalar=1;
    if(MSD<5)
     {
        *OutScalar=10/(MSD+1);
     }
    if(*OutScalar>1)
     {
        MultiplyBySingleDigit(CloneDividend,*OutScalar);
        MultiplyBySingleDigit(CloneDivisor,*OutScalar);
     }
    
    unsigned int TargetLen = BurnikelZiegler_BOUND;
    while (TargetLen < CloneDivisor->NrOfDigits) 
    {
        TargetLen *= 2;
    }
    
    *OutPad = TargetLen - CloneDivisor->NrOfDigits;
    if (*OutPad > 0) 
    {
        ShiftRightNPositions(CloneDividend, *OutPad);
        ShiftRightNPositions(CloneDivisor,  *OutPad);
    }

}

static BigNumber* ExtractBigNumberBlock(BigNumber* Number, unsigned int StartIndex, unsigned int Lenght) 
{
    // If startingIndex is larger than number of digits return "0" 
    if (StartIndex >= Number->NrOfDigits) 
    {
        BigNumber *Zero=Init("0");
        return Zero;
    }

    unsigned int ActualLenght = Lenght;
    if (StartIndex+ Lenght > Number->NrOfDigits) 
    {
        ActualLenght=Number->NrOfDigits-StartIndex;
    }

    char* ExtractedString = malloc(ActualLenght+1);
    if(ExtractedString==NULL)
     {
       perror("Allocating Memory inside BurnikelZiegler failed");
       exit(-1);
     }
    memcpy(ExtractedString,Number->Digits+StartIndex,ActualLenght);
    ExtractedString[ActualLenght] = '\0';

    BigNumber* Result = PrivateConstructor(ExtractedString, ActualLenght, Number->IsNegative);
    CleanTrailingZeros(Result);
    
    return Result;
}

//Optimized variant for (Top * 10^M) + Bottom  
//Fills with zeros if Bottom is shorter than M.
static BigNumber* ConcatPad(BigNumber* Bottom, BigNumber* Top, unsigned int M) 
{
    // If Top is Zero the result is the Bottom number
    if (Top->NrOfDigits == 1 && Top->Digits[0] == '0') 
    {
        return CloneBigNumber(Bottom);
    }

    unsigned int TotalLength = M + Top->NrOfDigits;
    char* CombinedDigits = malloc(TotalLength + 1);
    
    if (CombinedDigits == NULL) 
    { 
        perror("Memory Allocation failed in ConcatPad"); 
        exit(-1); 
    }
    
    // Copy the bottom part into the lower indices
    unsigned int Index;
    for (Index = 0; Index < Bottom->NrOfDigits && Index < M; Index++) 
    {
        CombinedDigits[Index] = Bottom->Digits[Index];
    }
    
    // Pad with zeros up to M if the bottom part was smaller
    for (; Index < M; Index++) 
    {
        CombinedDigits[Index] = '0';
    }
    
    //Append the top part into the higher indices
    for (unsigned int J = 0; J < Top->NrOfDigits; J++) 
    {
        CombinedDigits[M + J] = Top->Digits[J];
    }
    CombinedDigits[TotalLength] = '\0';
    
    BigNumber* Result = PrivateConstructor(CombinedDigits, TotalLength, false);
    CleanTrailingZeros(Result);
    
    return Result;
}

static BigNumber* Divide3nBy2n(BigNumber* Dividend3n, BigNumber* Divisor2n, BigNumber** Remainder);
static BigNumber* BurnikelZieglerDivide(BigNumber* Dividend, BigNumber* Divisor, BigNumber** Remainder) //ONLY WORKS IF Dividend->NrOfDigits<=2*Divisor->NrOfDigits
{
  if(Divisor->NrOfDigits <BurnikelZiegler_BOUND)
      {
        return LongDivision(Dividend,Divisor,Remainder);
      }
  
  unsigned int M = Divisor->NrOfDigits / 2;
  BigNumber* TopThreeBlocks = ExtractBigNumberBlock(Dividend, M, 3 * M);  //Extract blocks A3, A2, A1 as one contiguous array
  
  //Calculate the Upper Quotient recursively
    BigNumber* UpperRemainder = NULL;
    BigNumber* UpperQuotient = Divide3nBy2n(TopThreeBlocks, Divisor, &UpperRemainder);
    FreeMemory(TopThreeBlocks);
   
  //Assemble Temporary Dividend = UpperRemainder * 10^M + A0
    BigNumber* BottomBlockA0 = ExtractBigNumberBlock(Dividend, 0, M);
    BigNumber* CombinedForLowerQuotient = ConcatPad(BottomBlockA0, UpperRemainder, M);
    FreeMemory(BottomBlockA0); 
    FreeMemory(UpperRemainder);

  //Calculate the Lower Quotient recursively
    BigNumber* LowerRemainder = NULL;
    BigNumber* LowerQuotient = Divide3nBy2n(CombinedForLowerQuotient, Divisor, &LowerRemainder);
    FreeMemory(CombinedForLowerQuotient);

  //Assign the Optional Remainder if needed
    if (Remainder != NULL) 
      {
        *Remainder = LowerRemainder; 
      }
    else 
      {
        FreeMemory(LowerRemainder);
      }
    
    // Assemble Final Quotient = UpperQuotient * 10^M + LowerQuotient
    BigNumber* FinalQuotient = ConcatPad(LowerQuotient, UpperQuotient, M);

    FreeMemory(UpperQuotient); 
    FreeMemory(LowerQuotient);

    return FinalQuotient;
}

static BigNumber* Divide3nBy2n(BigNumber* Dividend3n, BigNumber* Divisor2n, BigNumber** Remainder)
{
  unsigned int M = Divisor2n->NrOfDigits / 2;

  // Extract A21 (Top 2 blocks of Dividend) and B1 (Top block of Divisor)
  BigNumber* TopTwoBlocks = ExtractBigNumberBlock(Dividend3n, M, 2 * M);
  BigNumber* UpperDivisorB1 = ExtractBigNumberBlock(Divisor2n, M, M);

  //Estimate the Quotient by returning to the 2n/n algorithm
    BigNumber* PartialRemainder = NULL;
    BigNumber* EstimatedQuotient =BurnikelZieglerDivide(TopTwoBlocks, UpperDivisorB1, &PartialRemainder);
    FreeMemory(TopTwoBlocks); 
    FreeMemory(UpperDivisorB1);

  //R_temp = PartialRemainder * 10^M + A0
    BigNumber* BottomBlockA0 = ExtractBigNumberBlock(Dividend3n, 0, M);
    BigNumber* TemporaryRemainder = ConcatPad(BottomBlockA0, PartialRemainder, M);
    FreeMemory(BottomBlockA0); 
    FreeMemory(PartialRemainder);
  
    BigNumber* LowerDivisorB0 = ExtractBigNumberBlock(Divisor2n, 0, M);
    BigNumber* CorrectionProduct = Multiply(EstimatedQuotient, LowerDivisorB0);
    FreeMemory(LowerDivisorB0);
    
    // Correction Step 
    // If TemporaryRemainder < CorrectionProduct, the quotient was overestimated.
    while (BigNumberCompareAbsoluteValue(TemporaryRemainder, CorrectionProduct) == -1) 
    {
        // Decrement EstimatedQuotient by 1
        BigNumber* One = Init("1");
        BigNumber* DecrementedQuotient = Subtract(EstimatedQuotient, One); 
        FreeMemory(EstimatedQuotient); 
        EstimatedQuotient = DecrementedQuotient; 
        FreeMemory(One);
        
        // Add Divisor back to the TemporaryRemainder
        BigNumber* AdjustedRemainder = Sum(TemporaryRemainder, Divisor2n); 
        FreeMemory(TemporaryRemainder); 
        TemporaryRemainder = AdjustedRemainder;
    }

  //Calculate actual Remainder if requested
    if (Remainder != NULL) 
    {
        *Remainder = Subtract(TemporaryRemainder, CorrectionProduct);
    }

    FreeMemory(TemporaryRemainder); 
    FreeMemory(CorrectionProduct);

    return EstimatedQuotient;
}

static BigNumber* ArbitraryBurnikelZiegler(BigNumber* Dividend, BigNumber* Divisor, BigNumber** Remainder)
{ 
    if (Dividend->NrOfDigits <= 2 * Divisor->NrOfDigits) 
    {
        return BurnikelZieglerDivide(Dividend, Divisor, Remainder);
    }

    // Divide the divident into blocks of size Divisor->NrOfDigits
    // Similar to LongDivision, each block will get divided
    
    BigNumber* CurrentRemainder = Init("0");
    BigNumber* FinalQuotient    = Init("0"); 
    BigNumber* Zero             = Init("0");
    // From left to right traverse the Divident into Divisor->NrOfDigits blocks
    long int currentIndex = Dividend->NrOfDigits;
    
    while (currentIndex > 0)
    {
        unsigned int ChunkSize = Divisor->NrOfDigits;
        if (currentIndex < Divisor->NrOfDigits) 
            ChunkSize = currentIndex; 
        
        currentIndex -= ChunkSize;
  
        BigNumber* NextChunk = ExtractBigNumberBlock(Dividend, currentIndex, ChunkSize);
        
        // TempDividend: R * 10^ChunkSize + NextChunk
        BigNumber* TempDividend;
        bool isRemainderZero = IsEqual(CurrentRemainder, Zero);
        if (isRemainderZero)
        {
            TempDividend = CloneBigNumber(NextChunk);
        }
        else
        {
            TempDividend = ConcatPad(NextChunk, CurrentRemainder, ChunkSize);
        }
        FreeMemory(NextChunk);
        
        BigNumber* BlockRemainder = NULL;
        BigNumber* BlockQuotient;
        
        if (BigNumberCompareAbsoluteValue(TempDividend, Divisor) == -1)
        {
            BlockQuotient = Init("0");
            BlockRemainder = CloneBigNumber(TempDividend);
        }
        else
        {
            BlockQuotient = BurnikelZieglerDivide(TempDividend, Divisor, &BlockRemainder);
        }
        
        // Adăugăm Câtul blocului la Câtul Final (Q_final = Q_final * 10^ChunkSize + BlockQuotient)
        BigNumber* TempFinalQ = ConcatPad(BlockQuotient, FinalQuotient, ChunkSize);
        FreeMemory(FinalQuotient);
        FinalQuotient = TempFinalQ;
        
        FreeMemory(CurrentRemainder);CurrentRemainder = BlockRemainder;
        FreeMemory(TempDividend);
        FreeMemory(BlockQuotient);
    }
    
    FreeMemory(Zero);

    if (Remainder != NULL)
    {
        *Remainder = CurrentRemainder;
    }
    else
    {
        FreeMemory(CurrentRemainder);
    }
    
    CleanTrailingZeros(FinalQuotient);
    return FinalQuotient;
}

BigNumber *Division(BigNumber *Dividend,BigNumber *Divisor,BigNumber **Remainder)
{
    BigNumber *Zero=Init("0");
    if(Dividend==NULL || Divisor==NULL || IsEqual(Divisor,Zero)==true)
      {
         if(Remainder!=NULL)
          *Remainder=NULL;
        FreeMemory(Zero);
        return NULL;
      }
    
    if(IsEqual(Dividend,Zero)==true)
      {
         if(Remainder!=NULL)
          {
            *Remainder=Init("0");
          }
         return Zero;
      }

    int CompareDivisorAndDivident=BigNumberCompareAbsoluteValue(Dividend, Divisor);
    if (CompareDivisorAndDivident== -1) 
      {
        if (Remainder != NULL) *Remainder = CloneBigNumber(Dividend);
        return Zero;
      }
    
    BigNumber *One=Init("1");
    if(CompareDivisorAndDivident == 0)
    {
        if (Remainder != NULL) *Remainder = Zero;
        else FreeMemory(Zero);
        return One;
    }

    if(IsEqual(Divisor,One)==true)
      {
        if(Remainder!=NULL)  *Remainder=Zero;
        else FreeMemory(Zero);
        
        BigNumber *Quotient=CloneBigNumber(Dividend);
        FreeMemory(One);
        return Quotient;
      }
    
    BigNumber *NegativeOne=Init("-1");
    if(IsEqual(Divisor,NegativeOne)==true)
      {
        if(Remainder!=NULL)
          *Remainder=Zero;
        
        BigNumber *Quotient=CloneBigNumber(Dividend);
        MultiplyByNegativeOne(Quotient);
        FreeMemory(NegativeOne);
        return Quotient;
      }
    
    bool ResultSign= Dividend->IsNegative!=Divisor->IsNegative; //Sign of the result
    
    //By working with clones(instead of multiplyin with -1 then reversing it at the end) we make it thread safe
    BigNumber* CloneDividend=CloneBigNumber(Dividend);
    BigNumber* CloneDivisor= CloneBigNumber(Divisor);

    //Forcing the numbers to be positive
    if(IsNegative(CloneDividend)==true) MultiplyByNegativeOne(CloneDividend);
    if(IsNegative(CloneDivisor)==true)  MultiplyByNegativeOne(CloneDivisor);
    
    BigNumber* Quotient;
    if(Divisor->NrOfDigits < BurnikelZiegler_BOUND ) 
      {
        Quotient=LongDivision(CloneDividend,CloneDivisor,Remainder);
      }
    else
      { 
        int Scalar=1;
        int Padding=0;
        NormalizeBurnikelZiegler(CloneDividend,CloneDivisor,&Scalar,&Padding);
        Quotient=ArbitraryBurnikelZiegler(CloneDividend,CloneDivisor,Remainder);
        if (Remainder != NULL && *Remainder != NULL) 
        {       
            if (Padding > 0) DividByPowerOf10(*Remainder, Padding);
            if (Scalar > 1)  DivideBySingleDigit(*Remainder, Scalar); 
        }
      }
    
    FreeMemory(CloneDividend);
    FreeMemory(CloneDivisor);

    Quotient->IsNegative = ResultSign;
    return Quotient;
}

void DivizionBy2(BigNumber *Number) //Modifies the NUMBER in MEMORY, DOENST RETURN A NEW ONE
{
    //An O(NrOfDigits) algoritm to quickly find Number/2 in memory without any Auxiliary Memory and No Garbage Collection
    //Very Usefull in Newton-Raphson iterations for Square Root and for Exponentiation by Squaring

    if(Number==NULL) return ;

    // (current_digit & 1 return the parity)  current_digit >> 1 returns curent_digit/2
    //If the digit is an even number the next carry is 5 else the carry is 0 

    unsigned int carry = 0;
    for (int i = (int)Number->NrOfDigits - 1; i >= 0; i--)  //number is stored in reverse in memory
    {
        unsigned int current_digit = Number->Digits[i] - '0';
        unsigned int new_digit = (current_digit >> 1) + (carry * 5);  //curent_digit/2 + (5 or 0) depending on parity
        carry = current_digit & 1; 
        Number->Digits[i] = new_digit + '0';
    }

    // Clean up any leading zero created by the division  (exemple 12/6 produces 06)
    if (Number->NrOfDigits > 1 && Number->Digits[Number->NrOfDigits - 1] == '0')
    {
        Number->NrOfDigits--;
        Number->Digits[Number->NrOfDigits] = '\0';
    }

}

void DivizionBy2Float(BigFloatNumber* Number)
{
    if (Number == NULL || Number->Mantissa == NULL) return;

    // If the mantissa is odd, integer division by 2 would lose the .5
    // We prevent this by shifting the mantissa * 10 and decrementing the exponent!
    if (IsOdd(Number->Mantissa))
    {
        ShiftRightNPositions(Number->Mantissa, 1); // Fast O(N) memory shift
        Number->Exponent -= 1; 
    }

    // Now it is guaranteed to be an even integer, so we safely apply the O(N) bit-shift!
    DivizionBy2(Number->Mantissa);
    
    // Clean up any trailing zeros we might have created
    CompressFloatInPlace(Number);
}


static BigFloatNumber* InverseInitialGuess(BigFloatNumber* Divisor)
{
    unsigned int len = Divisor->Mantissa->NrOfDigits;
    BigNumber* GuessMantissa = NULL;
    long int GuessExponent = 0;
    long int OriginalMagnitude = (long int)len - 1 + Divisor->Exponent;

// ==============================================================================
// 128-BIT ARCHITECTURE SUPPORT
// ==============================================================================
#if defined(__SIZEOF_INT128__)
    
    unsigned int DigitsToExtract = (len > 18) ? 18 : len; 
    __uint128_t TopDigits = 0;
    for (unsigned int i = 0; i < DigitsToExtract; i++) {
        TopDigits = TopDigits * 10 + (Divisor->Mantissa->Digits[len - 1 - i] - '0');
    }

    __uint128_t MassiveNumerator = ((__uint128_t)1000000000000000000ULL) * 1000000000000000000ULL; // 10^36
    __uint128_t InverseInt = MassiveNumerator / TopDigits;
    
    GuessMantissa = FromUnsignedInt128ToBigNum(InverseInt); 
    GuessExponent = -OriginalMagnitude + DigitsToExtract - 37; // Perfect 128-bit scaling

// ==============================================================================
// 64-BIT ARCHITECTURE
// ==============================================================================
#elif UINTPTR_MAX == 0xffffffffffffffff || defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
    
    unsigned int DigitsToExtract = (len > 9) ? 9 : len; 
    unsigned long long TopDigits = 0;
    for (unsigned int i = 0; i < DigitsToExtract; i++) {
        TopDigits = TopDigits * 10 + (Divisor->Mantissa->Digits[len - 1 - i] - '0');
    }

    unsigned long long MassiveNumerator = 1000000000000000000ULL; // 10^18
    unsigned long long InverseInt = MassiveNumerator / TopDigits;
    
    GuessMantissa = FromUnsignedLongLongToBigNum(InverseInt); 
    GuessExponent = -OriginalMagnitude + DigitsToExtract - 19; // Perfect 64-bit scaling

// ==============================================================================
// 32-BIT ARCHITECTURE
// ==============================================================================
#else

    unsigned int DigitsToExtract = (len > 4) ? 4 : len; 
    unsigned int TopDigits = 0;
    for (unsigned int i = 0; i < DigitsToExtract; i++) {
        TopDigits = TopDigits * 10 + (Divisor->Mantissa->Digits[len - 1 - i] - '0');
    }

    unsigned int MassiveNumerator = 100000000; // 10^8
    unsigned int InverseInt = MassiveNumerator / TopDigits;
    
    GuessMantissa = FromUnsignedIntegerToBigNum(InverseInt);
    GuessExponent = -OriginalMagnitude + DigitsToExtract - 9; // Perfect 32-bit scaling

#endif
// ==============================================================================

    GuessMantissa->IsNegative = Divisor->Mantissa->IsNegative;
    return PrivateConstructorFloat(GuessMantissa, GuessExponent);
}

BigFloatNumber* Inverse(BigFloatNumber *Number,unsigned int precision)
{
    //Finding 1/Number with set presion(Number of Decimals) using Newton Raphson and our above InitialGuess
    if(Number == NULL) return NULL;

    BigFloatNumber* Zero=InitFloat("0");
    if(IsEqual(Number->Mantissa,Zero->Mantissa)==true)
      {
         printf("Divizion by 0 encountered ,returning NULL");
         FreeMemoryFloat(Zero);
         return NULL;
      }
    FreeMemoryFloat(Zero);
    
    //By Newton Raphson method we get the following quadratic convergence sequence for a good enough initial guess
    // X_0=ReciprocalInitialGuess(Number) 
    // X_(N+1)=X_N*2-Number*X_N^2 
    // lim X_N = 1/Number
    //DEPENDING ON THE ARHITECTURE OF THE SYSTEM THE INITIAL GUESS HAS ALREADY 4(32bit),9(64bit),18(uint128 suport) CORRECT DECIMALS
    //Number of Iterations needed also depend on arhitecture as with each itteration the number of correct digits double
    
    unsigned int InternalPrecision=precision+3;
    unsigned int CurrentPrecision=4; //ON 32BIT
    #if defined(__SIZEOF_INT128__)  
      CurrentPrecision =18;          //ON 128BIT SUPPORT
    #elif UINTPTR_MAX == 0xffffffffffffffff || defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)  
      CurrentPrecision =9;          //ON 64BIT 
    #endif

    BigFloatNumber* XN=InverseInitialGuess(Number); //if precision is low, hardware division migh be enough
    if(CurrentPrecision>InternalPrecision)
       {
          RoundFloat(XN,precision);
          return XN;
       }
      
    while(CurrentPrecision<InternalPrecision)
      {
        //XN*XN
        BigFloatNumber* XNSquared=MultiplyFloat(XN,XN);
        RoundFloat(XNSquared,InternalPrecision);

        //Number*XN^2
        BigFloatNumber* NumberTimesXNSquared=MultiplyFloat(XNSquared,Number);
        RoundFloat(NumberTimesXNSquared,InternalPrecision);

        MultiplyBy2(XN->Mantissa);

        //2XN-Number*XN^2
        BigFloatNumber* NEXTXN=SubtractFloat(XN,NumberTimesXNSquared);
        CurrentPrecision*=2;

        FreeMemoryFloat(XN);
        XN=NEXTXN; //Assign OLD XN to NEW XN

        FreeMemoryFloat(NumberTimesXNSquared);
        FreeMemoryFloat(XNSquared);
      }

    RoundFloat(XN,precision);
    return XN;
}

BigNumber *Floor(BigFloatNumber *NumberFloat)
{
    BigNumber* NumberInt = CloneBigNumber(NumberFloat->Mantissa);
    if (NumberFloat->Exponent > 0) 
    {
        // It's a compressed integer (e.g. 5 * 10^2) so we are addign the 0`s back
        ShiftRightNPositions(NumberInt, NumberFloat->Exponent);
    } 
    else if (NumberFloat->Exponent < 0) 
    {
        // It has decimals. We TRUNCATE the decimals to simulate Floor()
        long int digits_to_chop = -(NumberFloat->Exponent);
        
        if (digits_to_chop >= NumberInt->NrOfDigits) 
        {
            // The number is 0.something
            NumberInt->Digits[0] = '0';
            NumberInt->Digits[1] = '\0';
            NumberInt->NrOfDigits = 1;
        } 
        else 
        {
            // Shift the integer digits down to index 0, overwriting the decimals
            unsigned int new_len = NumberInt->NrOfDigits - digits_to_chop;
            for (unsigned int i = 0; i < new_len; i++) 
            {
                NumberInt->Digits[i] = NumberInt->Digits[i + digits_to_chop];
            }
            NumberInt->Digits[new_len] = '\0';
            NumberInt->NrOfDigits = new_len;
        }
    }
  
    return NumberInt;
}

BigNumber* Modulo(BigNumber *Dividend, BigNumber *Modulus) //Very fast Modulo operation using Burnikel-Ziegler
{
    if (Dividend == NULL || Modulus == NULL) return NULL;

    BigNumber* Remainder = NULL;
    BigNumber* Quotient = Division(Dividend, Modulus, &Remainder); //We only care about the remainder from Burnikel-Ziegler

    FreeMemory(Quotient);

    return Remainder;
}

//Deprecated
BigNumber* ModuloNewtonRaphson(BigNumber * Dividend, BigNumber *Modulus)  // Divident mod Modulus Complexity O(N^1.58) Avg and O(1) when Modulus =0,1,2 and Dividend<Modulus
{
  // We calcute the Modulo by Divident mod Modulus:= Dividend -Modulus*Floor(Dividend/Modulus)  and finding Dividend/Modulus using Newton`s method
    if(Dividend==NULL || Modulus==NULL) return NULL;
    
    BigNumber *Zero=Init("0");
    if(IsEqual(Modulus,Zero)==true) // Undefined for a mod 0
      {
        FreeMemory(Zero);
        return NULL;
      }

    if(IsEqual(Dividend,Zero)==true && IsEqual(Modulus,Zero)==false)  // For any b>0 0 (mod b) =0
      {
        return Zero;
      }
    
    BigNumber* One=Init("1");
    BigNumber* Two=Init("2");
    if(IsEqual(Modulus,Two)==true)    
      {
        FreeMemory(Two);
        if(IsOdd(Dividend)==true)
           {
              FreeMemory(Zero);
              return One;
           }
        else
           {
              FreeMemory(One);
              return Zero;
           }
      }
    FreeMemory(Two);
    FreeMemory(One);
    FreeMemory(Zero);
    
    // If Dividend < Modulus, Dividend mod Modulus = Dividend
    if (BigNumberCompareAbsoluteValue(Dividend, Modulus) == -1)
    {
        return CloneBigNumber(Dividend);
    }

    unsigned int RequiredPrecision = Dividend->NrOfDigits + 5; //Guard Digits to increase accuracy for Newton`s Method and reduce the need for Correction Steps

    BigFloatNumber *DividendFloat=FromBigNumber(Dividend);                    // Converting Divident into BigFloat
    BigFloatNumber *ModulusFloat=FromBigNumber(Modulus);                      // Converting Modulus into BigFloat
    BigFloatNumber *InverseModulus=Inverse(ModulusFloat,RequiredPrecision);   // 1/Modulus
    BigFloatNumber *Quotient=MultiplyFloat(DividendFloat,InverseModulus);     // Dividend/Modulus
    BigNumber      *FloorQuotient=Floor(Quotient);                            // Floor(Divident/Modulus)
    BigNumber      *TempMultiply=Multiply(FloorQuotient,Modulus);             // Modulus*Floor(Dividend/Modulus)
    BigNumber      *Result=Subtract(Dividend,TempMultiply);                   // Dividend -Modulus*Floor(Dividend/Modulus)
    
    while (Result->IsNegative == true)
    {
        BigNumber* Corrected = Sum(Result, Modulus);
        FreeMemory(Result);
        Result = Corrected;
    }

    // Corrects the off-by-one truncation errors inherent to floating-point math.
    // If Quotient was 1 too large (Remainder went into the negatives)
    // If Quotient was 1 too small (Remainder is >= Modulus)
    while (BigNumberCompareAbsoluteValue(Result, Modulus) >= 0)
    {
        BigNumber* Corrected = Subtract(Result, Modulus);
        FreeMemory(Result);
        Result = Corrected;
    }

    FreeMemoryFloat(DividendFloat);
    FreeMemoryFloat(ModulusFloat);
    FreeMemoryFloat(Quotient);
    FreeMemoryFloat(InverseModulus);
    FreeMemory(TempMultiply);
    FreeMemory(FloorQuotient);

    return Result;
}

BigFloatNumber *DivisionFloat(BigFloatNumber *Divident,BigFloatNumber *Divisor, unsigned int precision) //Precision means number of decimal digits that the user wants
{
    if(Divident==NULL || Divisor==NULL )  //Also consider divizion by 0, in the future suport for +-Infinity could be added
       return NULL;
    
    //We need to normalize the divident to have the exponent equal to the precision
    //Then Use the Divizion Algorithm for BigINT to calculate the mantissa
    //Remark : Reminder will be set to NULL
    
    long int ShiftNeededInDividentExponent=Divident->Exponent - Divisor->Exponent + precision; //calculate shift for normalization
    BigNumber *CopyDivident=CloneBigNumber(Divident->Mantissa);  //create a new BIGINT ,DOESNT ALTER DIVIDENT
    BigNumber *CopyDivisor =CloneBigNumber(Divisor->Mantissa);   //create a new BIGINT ,DOESNT ALTER DIVISOR
    long int FinalExponent;

    if(ShiftNeededInDividentExponent>0)
      {
        ShiftRightNPositions(CopyDivident,ShiftNeededInDividentExponent); //Normalize Divident
        FinalExponent = Divident->Exponent - Divisor->Exponent - ShiftNeededInDividentExponent;
      }
    else
      {
        if(ShiftNeededInDividentExponent<0)
          {
            long int AbsShift = -ShiftNeededInDividentExponent;
            ShiftRightNPositions(CopyDivisor, (unsigned int)AbsShift);  //Normalize Divisor
            FinalExponent = Divident->Exponent - (Divisor->Exponent - AbsShift);
          }
        else
          {
            FinalExponent = Divident->Exponent - Divisor->Exponent;
          }
      }

    BigFloatNumber* Quotient=malloc(sizeof(BigFloatNumber));
    if(Quotient==NULL)
      {
        perror("Allocating memory for QuotientFloat failed");
        exit(-1);
      }
    Quotient->Mantissa=Division(CopyDivident,CopyDivisor,NULL);  //Perform Divizion to calculate Quotient, REMAINDER SET TO NULL
    Quotient->Exponent = FinalExponent;

    FreeMemory(CopyDivident);
    FreeMemory(CopyDivisor);
    
    CompressFloatInPlace(Quotient);
    return Quotient;
}

BigNumber* Power(BigNumber*Number,BigNumber *Power) //Return a new BIGINT equal to Number^Power , if Power is negative return 0, if power is Positive appy Exponentiation by squaring
{
    if(Power==NULL || Number==NULL) //Return NULL if any of the arguments are Not Valid
       return NULL;
    
    BigNumber *Zero=Init("0");
    BigNumber *One=Init("1");

    if(IsEqual(Number,One)==true) //1^a=1 for any integer a
      {
         FreeMemory(Zero);
         return One;
      }

    if(IsEqual(Power,Zero)==true)  //Any number raised to 0 is 1, We will follow the Real Analysis Convention that 0^0=1
      {
        FreeMemory(Zero);
        return One;
      }

    if(IsNegative(Power)==true) //Number^(-Power)==1/(Number^Power) and considering Number is a BIGINT ,Result will be 0
      {
        BigNumber *Zero=Init("0");  
        FreeMemory(One);
        return Zero;
      }
    else  //we will implement an iterrative aproach 
      {
         BigNumber *Two=Init("2");
         BigNumber *CopyNumber=CloneBigNumber(Number); //we need to preserve the old values
         BigNumber *CopyPower =CloneBigNumber(Power);
         BigNumber *Y=Init("1");
         while(BigNumberCompare(CopyPower,One)==1)  //while Power>1
           {
              if(IsOdd(CopyPower)==true)
                {  
                  BigNumber* NewY=Multiply(Y,CopyNumber);
                  SwapNumbersInMemory(&Y,&NewY);
                  FreeMemory(NewY);
                }
              
              BigNumber *SquaredNumber=KaratsubaSquaredMultiThreaded(CopyNumber,3);
              SwapNumbersInMemory(&SquaredNumber,&CopyNumber);  //x=x^2
              FreeMemory(SquaredNumber);

              DivizionBy2(CopyPower); //CopyPower=CopyPower/2
           }

         FreeMemory(Zero);
         FreeMemory(Two);
         FreeMemory(CopyPower);
         FreeMemory(One);

         BigNumber *Result=Multiply(Y,CopyNumber);
         FreeMemory(Y);
         return(Result);
      }
}

BigFloatNumber* Ln(BigFloatNumber* Number,unsigned int precision);
BigFloatNumber* Exp(BigFloatNumber* Number,unsigned int precision);
BigFloatNumber* PowerFloat(BigFloatNumber *Number,BigFloatNumber *Power,unsigned int precision) // Number^Power where both Number and Power are real numbers, precision dictates the number of digit if power is not a natural number
{
   if(Number==NULL ||Power==NULL || Number->Mantissa==NULL || Power->Mantissa==NULL)
    {
       return NULL;
    }

    BigFloatNumber *Zero=InitFloat("0");
    BigFloatNumber *One=InitFloat("1");

    if(IsEqual(Number->Mantissa,One->Mantissa)==true && Number->Exponent == 0) //1^a=1 for any real number a
      {
         FreeMemoryFloat(Zero);
         return One;
      }

    if(IsEqual(Power->Mantissa,Zero->Mantissa)==true && Power->Exponent == 0)  //Any real number raised to 0 is 1, We will follow the Real Analysis Convention that 0^0=1
      {
        FreeMemoryFloat(Zero);
        return One;
      }

    if(Power->Exponent>=0) //If Power is an integer use Exponentiation by squaring as above with the caveat that if Power is negative do x^-n= (1/x)^n
      {
          // Implementation of a GuardBuffer in order to manage the increasing decimal digits induced by the multiplication operation
          // GuardBuffer wil be used in Exponentiation By Squaring at each itteration of the while loop 
          unsigned int GuardBuffer= Power->Mantissa->NrOfDigits + 1;  //G= floor(log_10(Power))+ 2= Power->NrOfDigits+1
          unsigned int InternalPrecision = precision + GuardBuffer;
   
          BigFloatNumber* CopyNumber=CloneBigNumberFloat(Number);
          BigFloatNumber* CopyPower=CloneBigNumberFloat(Power);

          ShiftRightNPositions(CopyPower->Mantissa, CopyPower->Exponent);
          CopyPower->Exponent = 0;

          BigFloatNumber* Y=InitFloat("1");
          if(IsNegative(Power->Mantissa)==true) //x^-n= (1/x)^n
            {
              BigFloatNumber *DivideByOne=DivisionFloat(One,Number,InternalPrecision);// Calculate 1/x with InternalPrecision
              SwapNumbersInMemoryFloat(&DivideByOne,&CopyNumber);
              FreeMemoryFloat(DivideByOne);
              MultiplyByNegativeOne(CopyPower->Mantissa);  //Multiply the power by -1
            }
          while(BigNumberCompare(CopyPower->Mantissa,One->Mantissa)==1)  //while Power>1
           {
              if(IsOdd(CopyPower->Mantissa)==true)
                {  
                  BigFloatNumber* NewY=MultiplyFloat(Y,CopyNumber);
                  SwapNumbersInMemoryFloat(&Y,&NewY);
                  FreeMemoryFloat(NewY);
                  RoundFloat(Y,InternalPrecision);   
                }
              
              BigFloatNumber *SquaredNumber=MultiplyFloat(CopyNumber,CopyNumber);
              SwapNumbersInMemoryFloat(&SquaredNumber,&CopyNumber);  //x=x^2
              FreeMemoryFloat(SquaredNumber);
              RoundFloat(CopyNumber,InternalPrecision);

              DivizionBy2(CopyPower->Mantissa); // n=n/2
           }

         FreeMemoryFloat(Zero);
         FreeMemoryFloat(CopyPower);
         FreeMemoryFloat(One);

         BigFloatNumber *Result=MultiplyFloat(Y,CopyNumber);

         FreeMemoryFloat(Y);
         FreeMemoryFloat(CopyNumber);
         
         RoundFloat(Result,precision);
         return(Result);
      }
    else
      {
        // Number^Power = e^(Power*Ln(Number))
        BigFloatNumber* LnNumber=Ln(Number,precision);
        BigFloatNumber* ArgumentExp=MultiplyFloat(LnNumber,Power);
        BigFloatNumber* Result=Exp(ArgumentExp,precision);
        
        FreeMemoryFloat(Zero);
        FreeMemoryFloat(One);
        FreeMemoryFloat(LnNumber);
        FreeMemoryFloat(ArgumentExp);

        RoundFloat(Result,precision);
        return Result;
      }
} 

BigNumber* Factorial(unsigned int Number)
{
    BigNumber* FactorialRezult=Init("1");
    unsigned int index=2;
    for(index=2;index<=Number;index++)
      {
         BigNumber *Index=FromUnsignedIntegerToBigNum(index);
         BigNumber* TempFactorialRezult=Multiply(FactorialRezult,Index);
         SwapNumbersInMemory(&TempFactorialRezult,&FactorialRezult);
         FreeMemory(TempFactorialRezult);
         FreeMemory(Index);
      }
    
    return FactorialRezult;
}

static BigFloatNumber* SquareRootInitialGuess(BigFloatNumber* Number)
{
    if (Number == NULL || Number->Mantissa == NULL) return NULL;
    if (Number->Mantissa->IsNegative) 
    {
        perror("Mathematical Error: Cannot calculate square root of a negative number!");
        return NULL;
    }

    // Calculate the true base-10 magnitude
    long int E_total = (long int)Number->Mantissa->NrOfDigits - 1 + Number->Exponent;
    
    // Safely handle integer division for negative numbers to mimic mathematical floor()
    long int E_root;
    bool is_odd = (E_total % 2 != 0);

    if (E_total >= 0) 
    {
        E_root = E_total / 2;
    } 
    else 
    {
        if (is_odd) E_root = (E_total - 1) / 2;
        else        E_root = E_total / 2;
    }

    // Extract the top 1 or 2 digits based on parity
    unsigned int TopDigits = Number->Mantissa->Digits[Number->Mantissa->NrOfDigits - 1] - '0';
    
    if (is_odd) 
    {
        TopDigits *= 10;
        // Safety check: if the mantissa is something like 0.4 (1 digit total, but odd magnitude)
        if (Number->Mantissa->NrOfDigits > 1) 
        {
            TopDigits += Number->Mantissa->Digits[Number->Mantissa->NrOfDigits - 2] - '0';
        }
    }

    unsigned int RootDigit = 1;
    bool carry_exponent = false;

    if (TopDigits > 81) { RootDigit = 1; carry_exponent = true; } 
    else if (TopDigits > 64) RootDigit = 9;
    else if (TopDigits > 49) RootDigit = 8;
    else if (TopDigits > 36) RootDigit = 7;
    else if (TopDigits > 25) RootDigit = 6;
    else if (TopDigits > 16) RootDigit = 5;
    else if (TopDigits > 9)  RootDigit = 4;
    else if (TopDigits > 4)  RootDigit = 3;
    else if (TopDigits > 1)  RootDigit = 2;
    else RootDigit = 1; 

    if (carry_exponent) 
    {
        E_root += 1;
    }
    // Construct the 1-digit Mantissa
    char* GuessString = malloc(2 * sizeof(char));
    if (GuessString == NULL) return NULL;
    
    GuessString[0] = RootDigit + '0';
    GuessString[1] = '\0';
    
    BigNumber* MantissaGuess = PrivateConstructor(GuessString, 1, false);

    return PrivateConstructorFloat(MantissaGuess, E_root);
}

BigFloatNumber *SquareRoot(BigFloatNumber* Number, unsigned int precision) 
{
    if (Number == NULL || Number->Mantissa == NULL) return NULL;
    if (Number->Mantissa->IsNegative) 
    {
        perror("Mathematical Error: Cannot calculate square root of a negative number (yet)");
        return NULL;
    }

    unsigned int InternalPrecision = precision + 10;
    unsigned int current_precision = 1; // Our hardware guess gives us roughly 1 correct digit

    BigFloatNumber* InverseGuess = SquareRootInitialGuess(Number);
    BigFloatNumber* One= InitFloat("1");
    BigFloatNumber* Y=DivisionFloat(One,InverseGuess,InternalPrecision);
    BigFloatNumber* Three = InitFloat("3");

    while (current_precision <= InternalPrecision)
    {
        // Y^2
        BigFloatNumber* Y_Squared = MultiplyFloat(Y, Y);
        RoundFloat(Y_Squared, InternalPrecision);

        // S * Y^2
        BigFloatNumber* TempMultiply = MultiplyFloat(Number, Y_Squared);
        RoundFloat(TempMultiply, InternalPrecision);

        // 3 - (S * Y^2)
        BigFloatNumber* Difference = SubtractFloat(Three, TempMultiply);

        // (3 - (S * Y^2))/2
        DivizionBy2Float(Difference);

        //Next Y = Y/2*(3 - (S * Y^2))
        BigFloatNumber* NextY = MultiplyFloat(Y, Difference);
        RoundFloat(NextY, InternalPrecision);

        FreeMemoryFloat(Y_Squared);
        FreeMemoryFloat(TempMultiply);
        FreeMemoryFloat(Difference);
        FreeMemoryFloat(Y);

        Y = NextY; 
        current_precision *= 2; // Quadratic convergence
    }

   // S * (1 / sqrt(S)) = sqrt(S)
    BigFloatNumber* Result = MultiplyFloat(Number, Y);
    RoundFloat(Result, precision); // Chop off the guard digits


    FreeMemoryFloat(Three);
    FreeMemoryFloat(Y);
    FreeMemoryFloat(One);
    FreeMemoryFloat(InverseGuess);

    return Result;
}

BigFloatNumber *InverseSquareRoot(BigFloatNumber* Number, unsigned int precision) 
{
    if (Number == NULL || Number->Mantissa == NULL) return NULL;
    if (Number->Mantissa->IsNegative) 
    {
        perror("Mathematical Error: Cannot calculate square root of a negative number (yet)");
        return NULL;
    }

    unsigned int InternalPrecision = precision + 10;
    unsigned int current_precision = 1; // Our hardware guess gives us roughly 1 correct digit

    BigFloatNumber* InverseGuess = SquareRootInitialGuess(Number);
    BigFloatNumber* One= InitFloat("1");
    BigFloatNumber* Y=DivisionFloat(One,InverseGuess,InternalPrecision);
    BigFloatNumber* Three = InitFloat("3");

    while (current_precision <= InternalPrecision)
    {
        // Y^2
        BigFloatNumber* Y_Squared = MultiplyFloat(Y, Y);
        RoundFloat(Y_Squared, InternalPrecision);

        // S * Y^2
        BigFloatNumber* TempMultiply = MultiplyFloat(Number, Y_Squared);
        RoundFloat(TempMultiply, InternalPrecision);

        // 3 - (S * Y^2)
        BigFloatNumber* Difference = SubtractFloat(Three, TempMultiply);

        // (3 - (S * Y^2))/2
        DivizionBy2Float(Difference);

        //Next Y = Y/2*(3 - (S * Y^2))
        BigFloatNumber* NextY = MultiplyFloat(Y, Difference);
        RoundFloat(NextY, InternalPrecision);

        FreeMemoryFloat(Y_Squared);
        FreeMemoryFloat(TempMultiply);
        FreeMemoryFloat(Difference);
        FreeMemoryFloat(Y);

        Y = NextY; 
        current_precision *= 2; // Quadratic convergence
    }

    FreeMemoryFloat(Three);
    FreeMemoryFloat(One);
    FreeMemoryFloat(InverseGuess);

    return Y;
}


char *ToString(BigNumber *Number)
{
   char *String;
    if(Number->IsNegative)  //Allocate memory depending on sign
      {
        String=malloc(Number->NrOfDigits+2);
        if(String==NULL){
          perror("Allocating Mem for ToString Failed");
          exit(-2);
        }
        String[0]='-';
        unsigned int index=0;
        for(index=0;index<Number->NrOfDigits;index++) //as it is stored in reverse we need to display in reverse
            String[index+1]=Number->Digits[Number->NrOfDigits-index-1];
        String[index+1]='\0';
        return String;
      }
    else
      {
        String=malloc(Number->NrOfDigits+1);
        if(String==NULL){
          perror("Allocating Mem for ToString Failed");
          exit(-2);
        }
        unsigned int index=0;
        for(index=0;index<Number->NrOfDigits;index++) //as it is stored in reverse we need to display in reverse
            String[index]=Number->Digits[Number->NrOfDigits-index-1];
        String[index]='\0';
        return String;
      }
}

bool* GetBinaryRepresentation(BigNumber* Number, int* OutNrOfBits, int *OutHammingWeight)
{
    if(Number==NULL || Number->IsNegative==true) return NULL;

    unsigned int MaxBits=Number->NrOfDigits * 4; //We safely allocate more space than needed than reallocate at the end
    bool* Bits=malloc(sizeof(bool)*MaxBits);
    if(Bits==NULL)
      {
        printf("Allocating bits array for binary reprezentation failed");
        exit(-1);
      }

    unsigned int NrOfBits=0;                        
    unsigned int HammingWeight = 0; //Hamming weight is simply the number of 1`s in the binary representation, will be used in deciding which ModularExponentiation algo to use
    
    BigNumber* Copy = CloneBigNumber(Number);
    BigNumber* Zero = Init("0");
    
    if (IsEqual(Copy, Zero) == true)
    {
        Bits[0] = 0;
        NrOfBits = 1;
        HammingWeight = 0;
        goto BinaryLabel;
    }

    while (IsEqual(Copy, Zero) == false)  //Binary Representation is also stored in reverse
    {
        bool bit=IsOdd(Copy);
        Bits[NrOfBits] = bit;
        HammingWeight += bit;
        NrOfBits++;
        DivizionBy2(Copy);
    }
    
    BinaryLabel:
    if(OutHammingWeight != NULL)
      *OutHammingWeight = HammingWeight;

    if(OutNrOfBits != NULL)
      *OutNrOfBits = NrOfBits;

    bool *TempBits= realloc(Bits,sizeof(bool)*NrOfBits);
    if(TempBits==NULL)
      {
        printf("Realloction failed for Bit Representation");
        free(Bits);
        exit(-1);
      }
    Bits=TempBits;

    FreeMemory(Copy);
    FreeMemory(Zero);

    return Bits;
}

void PrintBinaryRepresentation(bool *BinaryRepresentation, int NrOfBits)
{
    if(BinaryRepresentation==NULL || NrOfBits <=0) 
    {
      printf("BINARY REPRESENTATION IS NULL");
      return;
    }

    for(int i=NrOfBits-1;i>=0;i--)
      {
         printf("%d",BinaryRepresentation[i]); //Binary Representation is also stored in reverse
      }
}

void PrintBigNumber(BigNumber *Number)
{
    if(Number==NULL)
     {
      printf("NULL ");
      return;
     }

    if(Number->IsNegative==true)
      printf("-");
    for(unsigned int index=0;index<Number->NrOfDigits;index++) //as it is stored in reverse we need to display in reverse
      printf("%c",Number->Digits[Number->NrOfDigits-index-1]);
    printf(" ");
}

void PrintBigFloatNumber(BigFloatNumber *Number)
{
   if(Number==NULL)
     {
      printf("NULL ");
      return;
     }

   if(Number->Mantissa->IsNegative==true)
      printf("-");
    
   if(Number->Exponent>=0)
     {
       for(unsigned int index=0;index<Number->Mantissa->NrOfDigits;index++) //as it is stored in reverse we need to display in reverse
          printf("%c",Number->Mantissa->Digits[Number->Mantissa->NrOfDigits-index-1]);
       
        for(unsigned int index=0;index<Number->Exponent;index++)
          printf("0");

       printf(" ");
     }
   else
     {
        long int AbsoluteValueExponent=(-1)*Number->Exponent;
        if(AbsoluteValueExponent >= Number->Mantissa->NrOfDigits)
         {
            printf("0.");
            for(unsigned int i=0;i<AbsoluteValueExponent-Number->Mantissa->NrOfDigits; i++)
            {
                printf("0");
            }
            for(unsigned int index=0;index<Number->Mantissa->NrOfDigits;index++) //as it is stored in reverse we need to display in reverse
            {
              printf("%c",Number->Mantissa->Digits[Number->Mantissa->NrOfDigits-index-1]);
            }
            printf(" ");
         }
        else
          {
              for(unsigned int index=0;index<Number->Mantissa->NrOfDigits;index++) //as it is stored in reverse we need to display in reverse
            {
              if(index == Number->Mantissa->NrOfDigits-AbsoluteValueExponent)
                printf(".");
              printf("%c",Number->Mantissa->Digits[Number->Mantissa->NrOfDigits-index-1]);
            }
            printf(" ");
          }
 
     }
}

BigFloatNumber* CalculatePiGaussLegendre(unsigned int precision) //Calculates precision digits of pi using Gauss Legendre Algorithm
{
    //Precision Setting
    unsigned int GuardBuffer=30;
    unsigned int InternalPrecision = precision + GuardBuffer;
    unsigned int CurrentPrecision =1;
    unsigned int IterationCount = 0;

    //Initial Value Setting
    BigFloatNumber* A_Current=InitFloat("1");
    BigFloatNumber* Two=InitFloat("2");
    BigFloatNumber* T_Current=InitFloat("0.25");
    BigFloatNumber* B_Current=InverseSquareRoot(Two,InternalPrecision);
    
      do  //Forces to do at least one Gauss-Legendre Iteration
       {
          CurrentPrecision*=2;  //Quadratic Convergence

          BigFloatNumber *A_Next=SumFloat(A_Current,B_Current);
          DivizionBy2Float(A_Next);
          RoundFloat(A_Next, InternalPrecision);

          BigFloatNumber* AB=MultiplyFloat(A_Current,B_Current);
          BigFloatNumber* B_Next=SquareRoot(AB,InternalPrecision);
          RoundFloat(B_Next, InternalPrecision);

          BigFloatNumber* DeltaA=SubtractFloat(A_Current,A_Next);
          BigFloatNumber* DeltaASquared=MultiplyFloat(DeltaA,DeltaA);
          for(unsigned int i=0;i<IterationCount;i++)
            {
              MultiplyBy2(DeltaASquared->Mantissa);
            }
          CompressFloatInPlace(DeltaASquared);
          RoundFloat(DeltaASquared, InternalPrecision);
          BigFloatNumber* T_Next=SubtractFloat(T_Current,DeltaASquared);

          FreeMemoryFloat(DeltaA);
          FreeMemoryFloat(DeltaASquared);
          FreeMemoryFloat(AB);

          FreeMemoryFloat(A_Current);A_Current=A_Next;
          FreeMemoryFloat(B_Current);B_Current=B_Next;
          FreeMemoryFloat(T_Current);T_Current=T_Next;    

          IterationCount++;
       }while(CurrentPrecision<InternalPrecision);

    BigFloatNumber *SumAB=SumFloat(A_Current,B_Current);
    BigFloatNumber *SumABSquared=MultiplyFloat(SumAB,SumAB);

    MultiplyBy2(T_Current->Mantissa);
    MultiplyBy2(T_Current->Mantissa);
    CompressFloatInPlace(T_Current);

    BigFloatNumber *InverseT=Inverse(T_Current,InternalPrecision);
    BigFloatNumber *PI=MultiplyFloat(InverseT,SumABSquared);

    FreeMemoryFloat(SumAB);
    FreeMemoryFloat(SumABSquared);
    FreeMemoryFloat(InverseT);
    FreeMemoryFloat(Two);
    FreeMemoryFloat(A_Current);
    FreeMemoryFloat(B_Current);
    FreeMemoryFloat(T_Current);

    RoundFloat(PI,precision);
    return PI;
}

// AGM Methods For Finding Ln(10) and Ln(Number)
// AGM(A0,B0) :=  A_(N+1)=(A_N+B_N)/2 and B_(N+1)=SQRT(A_N*B_N), In the limit as N-> A_N=B_N :=AGM(A0,B0)
// LN(S)~Pi/(2*AGM(1,4/S) where S should be a massive number
// S is a BigFloat => S=x*10^M |ln()  => Ln(S)=Ln(x)+M*Ln(10) =>Ln(x)=Ln(S)-m*Ln(10)
// For finding Ln(x) we need to scale X with M zeros of precision 
static BigFloatNumber* AGM(BigFloatNumber* A, BigFloatNumber* B, unsigned int precision)
{
    // Calculate the leading zero sink inside AGM
    long int magA = (long int)A->Mantissa->NrOfDigits - 1 + A->Exponent;
    long int magB = (long int)B->Mantissa->NrOfDigits - 1 + B->Exponent;
    long int diff_mag = magA - magB;
    if (diff_mag < 0) diff_mag = -diff_mag; // Absolute value
    
    // Total precision = requested precision + leading zero loss + GuardBuffer
    unsigned int InternalPrecision = precision + (unsigned int)diff_mag + 30; 
    
    bool HasConverged = false;
    unsigned int IterationCount = 0;

    BigFloatNumber *A0 = CloneBigNumberFloat(A);
    BigFloatNumber *B0 = CloneBigNumberFloat(B);

    while(HasConverged == false && IterationCount < 25)
    {
        BigFloatNumber* ProductA0B0 = MultiplyFloat(A0,B0);
        BigFloatNumber* B1 = SquareRoot(ProductA0B0,InternalPrecision);
        RoundFloat(B1, InternalPrecision);

        BigFloatNumber* A1 = SumFloat(A0,B0);
        DivizionBy2Float(A1);
        RoundFloat(A1, InternalPrecision);

        BigFloatNumber* DeltaA = SubtractFloat(A0,A1);
        if (DeltaA->Mantissa->NrOfDigits == 1 && DeltaA->Mantissa->Digits[0] == '0') 
        {
            HasConverged = true; // Perfect 0
        }
        else
        {
            long int diff_magnitude = (long int)DeltaA->Mantissa->NrOfDigits - 1 + DeltaA->Exponent;
            
            // It safely converges exactly to what the caller requested
            if (diff_magnitude <= -(long int)precision) 
            {
                 HasConverged = true; 
            }
        }

        FreeMemoryFloat(ProductA0B0);
        FreeMemoryFloat(DeltaA);
        FreeMemoryFloat(A0); A0 = A1;
        FreeMemoryFloat(B0); B0 = B1;

        IterationCount++;
    }
    
    FreeMemoryFloat(B0);
    return A0; 
}

BigFloatNumber* GenerateLn10Constant(BigFloatNumber* Global_Pi, unsigned int precision)
{
    unsigned int m = (precision / 2) + 5;
    unsigned int TargetPrecision = precision + 10; 
    
    // O(1) construction of B0 = 4 / 10^(m+1)
    BigNumber* FourMantissa = Init("4");
    BigFloatNumber* B0 = PrivateConstructorFloat(FourMantissa, -(long int)(m + 1));

    // Run the AGM
    BigFloatNumber* A0 = InitFloat("1");
    BigFloatNumber* AgmResult = AGM(A0, B0, TargetPrecision);

    // Calculate Ln(S) = Pi / (2 * AGM)
    BigFloatNumber* Pi_Over_2 = CloneBigNumberFloat(Global_Pi);
    DivizionBy2Float(Pi_Over_2);
    
    BigFloatNumber* InverseAgm = Inverse(AgmResult, TargetPrecision);
    BigFloatNumber* Ln_S = MultiplyFloat(Pi_Over_2, InverseAgm);

    // Ln(10) = Ln(S) / (m+1)
    BigNumber* DivisorInt = FromUnsignedIntegerToBigNum(m + 1);
    BigFloatNumber* DivisorFloat = PrivateConstructorFloat(DivisorInt, 0);
    
    BigFloatNumber* InverseDivisor = Inverse(DivisorFloat, TargetPrecision);
    BigFloatNumber* Ln10 = MultiplyFloat(Ln_S, InverseDivisor); 
    RoundFloat(Ln10, precision);

    FreeMemoryFloat(B0); 
    FreeMemoryFloat(A0); 
    FreeMemoryFloat(AgmResult);
    FreeMemoryFloat(Pi_Over_2); 
    FreeMemoryFloat(InverseAgm);
    FreeMemoryFloat(Ln_S); 
    FreeMemoryFloat(DivisorFloat); 
    FreeMemoryFloat(InverseDivisor);

    return Ln10;
}

BigFloatNumber* Ln(BigFloatNumber* X, unsigned int precision)
{
    if(X==NULL ||INTERNAL_GLOBAL_PI==NULL || INTERNAL_GLOBAL_LN10==NULL) return NULL;

    if (IsNegative(X->Mantissa) || (X->Mantissa->NrOfDigits == 1 && X->Mantissa->Digits[0] == '0'))
    {
        printf("Mathematical Error: ln(x) is undefined for x <= 0\n");
        return NULL;
    }

    //Ln(1) = 0
    BigFloatNumber* One = InitFloat("1");
    if (IsEqual(X->Mantissa, One->Mantissa) && X->Exponent == 0)
    {
        FreeMemoryFloat(One);
        return InitFloat("0");
    }
    FreeMemoryFloat(One);

    // s only needs to reach 10^(precision / 2) to satisfy the error bounds
    long int target_magnitude = (precision / 2) + 5;
    long int current_magnitude = (long int)X->Mantissa->NrOfDigits - 1 + X->Exponent;
    
    long int m = target_magnitude - current_magnitude;
    if (m < 0) m = 0; // X is already massive enough!


    unsigned int CancellationGuard = 0;
    long int temp_m = m;
    while(temp_m > 0) { CancellationGuard++; temp_m /= 10; }
    
    unsigned int DecimalPlacesNeeded = precision + 10 + CancellationGuard;

    // Instead of inverting a massive S, we invert X and shift the exponent
    BigFloatNumber* B0 = Inverse(X, DecimalPlacesNeeded);
    MultiplyBy2(B0->Mantissa);MultiplyBy2(B0->Mantissa);
    CompressFloatInPlace(B0);
    
    B0->Exponent -= m; // Scales by 10^-m 
    RoundFloat(B0, DecimalPlacesNeeded);

    // Run AGM
    BigFloatNumber* A0 = InitFloat("1");
    BigFloatNumber* AgmResult = AGM(A0, B0, DecimalPlacesNeeded);

    // Ln(S) = (Pi / 2) * (1 / AGM)
    BigFloatNumber* Pi_Over_2 = CloneBigNumberFloat(INTERNAL_GLOBAL_PI);
    DivizionBy2Float(Pi_Over_2);
    
    BigFloatNumber* InverseAgm = Inverse(AgmResult, DecimalPlacesNeeded);
    BigFloatNumber* Ln_S = MultiplyFloat(Pi_Over_2, InverseAgm);

    // Un-scale: Ln(X) = Ln(S) - (m * Ln(10))
    BigFloatNumber* Result = NULL;
    if (m > 0) 
    {
        BigNumber* M_Int = FromUnsignedIntegerToBigNum((unsigned int)m);
        BigFloatNumber* M_Float = PrivateConstructorFloat(M_Int, 0);
        
        BigFloatNumber* SubtractionTerm = MultiplyFloat(M_Float, INTERNAL_GLOBAL_LN10);
        Result = SubtractFloat(Ln_S, SubtractionTerm);
        
        FreeMemoryFloat(M_Float);
        FreeMemoryFloat(SubtractionTerm);
    }
    else 
    {
        Result = CloneBigNumberFloat(Ln_S);
    }

    RoundFloat(Result, precision);

    FreeMemoryFloat(B0); 
    FreeMemoryFloat(A0);
    FreeMemoryFloat(AgmResult);
    FreeMemoryFloat(Pi_Over_2);
    FreeMemoryFloat(InverseAgm);
    FreeMemoryFloat(Ln_S); 

    return Result;
}

long int BigNumToLong(BigNumber* Number)
{
    long int value = 0;
    long int multiplier = 1;
    // Extract up to 15 digits (safe for 64-bit signed integers)
    for(unsigned int i = 0; i < Number->NrOfDigits && i < 15; i++)
    {
        value += (Number->Digits[i] - '0') * multiplier;
        multiplier *= 10;
    }
    if (Number->IsNegative) value = -value;
    return value;
}

BigFloatNumber* ExpInitialGuess(BigFloatNumber* Y)
{
    unsigned int len = Y->Mantissa->NrOfDigits;

// ==============================================================================
// 128-BIT ARCHITECTURE SUPPORT (Quadruple Precision Float)
// ==============================================================================
#if defined(__SIZEOF_INT128__) && defined(__GNUC__)
    
    __float128 y_hw = 0.0;
    __float128 place = 1.0;
    unsigned int DigitsToExtract = (len > 34) ? 34 : len;

    for (unsigned int i = 0; i < DigitsToExtract; i++) {
        unsigned int index = len - 1 - i;
        y_hw += (__float128)(Y->Mantissa->Digits[index] - '0') / place;
        place *= 10.0;
    }

    long int hardware_exponent = Y->Exponent + (long int)len - 1;
    if (hardware_exponent > 0) {
        for (long int i = 0; i < hardware_exponent; i++) y_hw *= 10.0;
    } else if (hardware_exponent < 0) {
        for (long int i = 0; i < -hardware_exponent; i++) y_hw /= 10.0;
    }

    __float128 hw_result = 1.0; 
    __float128 term = 1.0;
    for (int i = 1; i <= 46; i++) {
        term = (term * y_hw) / (__float128)i;
        hw_result += term;
    }

   char buffer[128]; //we will manually store the number into a string without quad_snprintf
   //Number will always be under 10 so will fit into int 
   //Extract the integer part
    int integer_part = (long long)hw_result;
    __float128 fractional_part = hw_result - (__float128)integer_part;

    // Write the integer part and the decimal point
    int pos = sprintf(buffer, "%d.", integer_part);

    // Extract exactly 33 fractional digits
    for (int i = 0; i < 33; i++) {
        fractional_part *= 10.0;
        int digit = (int)fractional_part;
        buffer[pos++] = digit + '0';
        fractional_part -= (__float128)digit;
    }
    buffer[pos] = '\0';

    return InitFloat(buffer);

// ==============================================================================
// 64-BIT ARCHITECTURE (Extended Precision / Long Double)
// ==============================================================================
#elif UINTPTR_MAX == 0xffffffffffffffff || defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
    
    long double y_hw = 0.0;
    long double place = 1.0;
    unsigned int DigitsToExtract = (len > 18) ? 18 : len;

    for (unsigned int i = 0; i < DigitsToExtract; i++) {
        unsigned int index = len - 1 - i;
        y_hw += (long double)(Y->Mantissa->Digits[index] - '0') / place;
        place *= 10.0;
    }

    long int hardware_exponent = Y->Exponent + (long int)len - 1;
    if (hardware_exponent > 0) {
        for (long int i = 0; i < hardware_exponent; i++) y_hw *= 10.0;
    } else if (hardware_exponent < 0) {
        for (long int i = 0; i < -hardware_exponent; i++) y_hw /= 10.0;
    }

    long double hw_result = 1.0; 
    long double term = 1.0;
    for (int i = 1; i <= 29; i++) {
        term = (term * y_hw) / (long double)i;
        hw_result += term;
    }

    char buffer[64];
    sprintf(buffer, "%.18Lf", hw_result);
    return InitFloat(buffer);

// ==============================================================================
// 32-BIT ARCHITECTURE (Standard Double)
// ==============================================================================
#else

    double y_hw = 0.0;
    double place = 1.0;
    unsigned int DigitsToExtract = (len > 14) ? 14 : len;

    for (unsigned int i = 0; i < DigitsToExtract; i++) {
        unsigned int index = len - 1 - i;
        y_hw += (double)(Y->Mantissa->Digits[index] - '0') / place;
        place *= 10.0;
    }

    long int hardware_exponent = Y->Exponent + (long int)len - 1;
    if (hardware_exponent > 0) {
        for (long int i = 0; i < hardware_exponent; i++) y_hw *= 10.0;
    } else if (hardware_exponent < 0) {
        for (long int i = 0; i < -hardware_exponent; i++) y_hw /= 10.0;
    }

    double hw_result = 1.0; 
    double term = 1.0;
    for (int i = 1; i <= 24; i++) {
        term = (term * y_hw) / (double)i;
        hw_result += term;
    }

    char buffer[64];
    sprintf(buffer, "%.14f", hw_result);
    return InitFloat(buffer);

#endif
// ==============================================================================
}

BigFloatNumber* Exp(BigFloatNumber* X, unsigned int precision)
{
    if(X == NULL || INTERNAL_GLOBAL_PI == NULL || INTERNAL_GLOBAL_LN10 == NULL) return NULL;

    // e^0 = 1
    BigFloatNumber* Zero = InitFloat("0");
    if(IsEqual(X->Mantissa, Zero->Mantissa) && X->Exponent == 0)
    {
        FreeMemoryFloat(Zero);
        return InitFloat("1");
    }
    FreeMemoryFloat(Zero);

    // Negative exponent handler: e^-x = 1 / e^x
    BigFloatNumber* AbsX = CloneBigNumberFloat(X); //Clone X making exp thread safe
    bool IsNegativeExp = AbsX->Mantissa->IsNegative;

    // Force the local clone to be strictly positive for the math
    AbsX->Mantissa->IsNegative = false;

    // ==============================================================================
    // ARGUMENT REDUCTION: X = K * Ln(10) + Y
    // ==============================================================================
    long int X_Magnitude = (long int)AbsX->Mantissa->NrOfDigits - 1 + AbsX->Exponent;
    if (X_Magnitude < 0) X_Magnitude = 0;
    
    // Boost precision during reduction to prevent cancellation!
    unsigned int ReductionPrecision = precision + X_Magnitude + 10;

    BigFloatNumber* R = DivisionFloat(AbsX, INTERNAL_GLOBAL_LN10, ReductionPrecision);
    BigNumber* K_Int = Floor(R);
    BigFloatNumber* K_Float = PrivateConstructorFloat(CloneBigNumber(K_Int), 0);

    BigFloatNumber* K_Times_Ln10 = MultiplyFloat(K_Float, INTERNAL_GLOBAL_LN10);
    BigFloatNumber* Y = SubtractFloat(AbsX, K_Times_Ln10);
    RoundFloat(Y, precision + 10); 

    // ==============================================================================
    // ARCHITECTURE-AWARE PRECISION SETUP
    // ==============================================================================
    unsigned int InternalPrecision = precision + 10;
    unsigned int CurrentPrecision = 14; // Default to 32BIT

    #if defined(__SIZEOF_INT128__) && defined(__GNUC__)
        CurrentPrecision = 33;          // ON 128BIT SUPPORT
    #elif UINTPTR_MAX == 0xffffffffffffffff || defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
        CurrentPrecision = 18;          // ON 64BIT 
    #endif

    BigFloatNumber* E_N = ExpInitialGuess(Y); 

    // If the hardware guess is already perfect we skip Newton-Raphson
    if(CurrentPrecision >= InternalPrecision)
    {
        RoundFloat(E_N, InternalPrecision);
    }
    else
    {
        // ==============================================================================
        // NEWTON-RAPHSON LOOP: E_{n+1} = E_n + E_n * (Y - Ln(E_n))
        // ==============================================================================
        while (CurrentPrecision < InternalPrecision)
        {
            unsigned int StepPrecision = (CurrentPrecision * 2) + 5;
            if (StepPrecision > InternalPrecision) StepPrecision = InternalPrecision;

            BigFloatNumber* Ln_En = Ln(E_N,StepPrecision);
            BigFloatNumber* Difference = SubtractFloat(Y, Ln_En);
            BigFloatNumber* Product = MultiplyFloat(E_N, Difference);
            BigFloatNumber* NEXT_EN = SumFloat(E_N, Product);
            RoundFloat(NEXT_EN, StepPrecision);

            FreeMemoryFloat(Ln_En);
            FreeMemoryFloat(Difference);
            FreeMemoryFloat(Product);
            FreeMemoryFloat(E_N);

            E_N = NEXT_EN;
            CurrentPrecision *= 2;
        }
    }

    // ==============================================================================
    // BASE-10 SCALING: e^X = e^Y * 10^K
    // ==============================================================================
    long int K_Value = BigNumToLong(K_Int);
    E_N->Exponent += K_Value; 

    RoundFloat(E_N, precision);

    // If the original request was e^-x, invert the final result
    if (IsNegativeExp)
    {
        BigFloatNumber* FinalResult = Inverse(E_N, precision);
        FreeMemoryFloat(E_N);
        E_N = FinalResult;
      
    }

    // Cleanup
    FreeMemoryFloat(AbsX); //free the clone`s memory
    FreeMemoryFloat(R); 
    FreeMemory(K_Int); 
    FreeMemoryFloat(K_Float);
    FreeMemoryFloat(K_Times_Ln10); 
    FreeMemoryFloat(Y); 

    RoundFloat(E_N,precision);
    return E_N;
}


void InitializeBigNumberSupport(unsigned int precision)
{
    // If the user calls it twice, free the old memory first to prevent leaks!
    if (INTERNAL_GLOBAL_PI != NULL) FreeMemoryFloat(INTERNAL_GLOBAL_PI);
    if (INTERNAL_GLOBAL_LN10 != NULL) FreeMemoryFloat(INTERNAL_GLOBAL_LN10);

    // We calculate the constants to slightly higher precision to absorb truncation noise
    INTERNAL_GLOBAL_PRECISION = precision + 10; 

    printf("Initializing BigNumber Environment (Precision: %u digits)...\n", INTERNAL_GLOBAL_PRECISION);

    INTERNAL_GLOBAL_PI = CalculatePiGaussLegendre(INTERNAL_GLOBAL_PRECISION);
    INTERNAL_GLOBAL_LN10 = GenerateLn10Constant(INTERNAL_GLOBAL_PI, INTERNAL_GLOBAL_PRECISION);

    printf("BigNumber Environment Ready!\n");
}

void InitializeBigNumberLibrary(unsigned int precision)
{
    if (INTERNAL_GLOBAL_PI != NULL) FreeMemoryFloat(INTERNAL_GLOBAL_PI);
    if (INTERNAL_GLOBAL_LN10 != NULL) FreeMemoryFloat(INTERNAL_GLOBAL_LN10);

    INTERNAL_GLOBAL_PRECISION = precision + 10; 

    printf("Initializing BigNumber Environment (MAX Precision: %u digits)...\n", INTERNAL_GLOBAL_PRECISION);

    INTERNAL_GLOBAL_PI = CalculatePiGaussLegendre(INTERNAL_GLOBAL_PRECISION);
    INTERNAL_GLOBAL_LN10 = GenerateLn10Constant(INTERNAL_GLOBAL_PI, INTERNAL_GLOBAL_PRECISION);

    printf("BigNumber Environment Ready!\n");
}

void DistroyBigNumberLibrary()
{
    if (INTERNAL_GLOBAL_PI != NULL) {
        FreeMemoryFloat(INTERNAL_GLOBAL_PI);
        INTERNAL_GLOBAL_PI = NULL;
    }
    if (INTERNAL_GLOBAL_LN10 != NULL) {
        FreeMemoryFloat(INTERNAL_GLOBAL_LN10);
        INTERNAL_GLOBAL_LN10 = NULL;
    }
    INTERNAL_GLOBAL_PRECISION = 0;
}

BigFloatNumber* GetConstantPi(unsigned int precision)
{
    if (INTERNAL_GLOBAL_PI == NULL) {
        printf("Error: BigNumber Support not initialized!\n");
        return NULL;
    }
    
    BigFloatNumber* Pi=CloneBigNumberFloat(INTERNAL_GLOBAL_PI);
    RoundFloat(Pi,precision);
    return Pi;
}

BigFloatNumber* GetConstantLn10(unsigned int precision)
{
    if (INTERNAL_GLOBAL_LN10 == NULL) {
        printf("Error: BigNumber Support not initialized!\n");
        return NULL;
    }

    BigFloatNumber* LN10=CloneBigNumberFloat(INTERNAL_GLOBAL_LN10);
    RoundFloat(LN10,precision);
    return LN10;
}

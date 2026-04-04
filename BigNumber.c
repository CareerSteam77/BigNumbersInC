#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<stdbool.h>
#include<ctype.h>
#include<pthread.h>

#define Karatsuba_BOUND 64 //at how many digits should standard multiplication be used instead of Karatsuba

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

typedef struct{
    BigNumber *Number;
    BigNumber *Result;
    unsigned short int Iterations;
}ThreadArgumentKaratsubaSquared;

//Using the Compress Function 
//Exponent < 0: It has decimals
//Exponent == 0: It is an integer ending in 1-9 
//Exponent > 0: It is an integer ending in 0`s 


void CleanTrailingZeros(BigNumber* Number)  //Eliminates Unecesary 0`s from the number
{
    while (Number->NrOfDigits>1 && (Number->Digits[Number->NrOfDigits - 1] == '0'))
    {
      Number->NrOfDigits--;
    }
    Number->Digits[Number->NrOfDigits] = '\0';
}

void CompressFloatInPlace(BigFloatNumber *Number) //Every tail 0 gets removed and added to the exponent thus shortening the Mantissa lenght facilitating faster arithmetic operations
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

bool GetIsNegativeFromAnInt(int number)
 {
    if(number>=0)
       return false;
    else
       return true;
 }

bool VerifyStringIsNumber(const char *Value)
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

bool VerifyStringIsDecimal(const char *Value)
{
  //check for string of the form "-x1x2..'.'x3..." and "x1x2..'.'x3..." where xi is a digit and it need to have one decimal
    if(Value==NULL || Value[0]=='\0')  //check for NULL and Empty string
       return false;

    if(!isdigit(Value[0]) && Value[0]!='-')
       return false;
    
    short int DecimalPointCounter=0; //it should be always 1 or 0 ,otherwise Init fails  
    for(unsigned int index=1;index<strlen(Value);index++)
       {
        if(isdigit(Value[index])==false && (Value[index]!='.'))
          return false;

        if(Value[index]=='.')
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

void ShiftRightNPositions(BigNumber *Number,unsigned int N) //For multiplication by positive integer powers of 10 and for Long Division
{  //DOESNT PRODUCE A NEW BIGINT, changes the argument in memory
   if(N<=0) return; //no change needed

   char *NewDigits=malloc(sizeof(char)*(Number->NrOfDigits+N+1)); //Prev Digits + positions +'\0'
   strcpy(NewDigits,Number->Digits);

   unsigned int index=0;
   for(index = 0; index < N; index++) //add the zerous
    {
        NewDigits[index] = '0';
    }

   strcpy(NewDigits+N,Number->Digits);  //copy prev digits and deallocate memory
   free(Number->Digits);

   Number->Digits=NewDigits;
   Number->NrOfDigits+=N;  //increment by positions

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

    unsigned int DigitsToChop =CurentPrecision-MaxPrecision; //  Calculate how many digits we need to chop off from the least significant side
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
  if (Number1 == NULL || Number2 == NULL) return NULL;
  //Is equivalent to Number1+MultiplyByNegativeOne(Number2)  a-b == a+ (-b)
  MultiplyByNegativeOne(Number2); //change the sign
  BigNumber *Rezult=Sum(Number1,Number2);
  MultiplyByNegativeOne(Number2); //reverse the change of sign
  return Rezult;
}

BigFloatNumber* SubtractFloat(BigFloatNumber* Number1, BigFloatNumber* Number2)
{
  if (Number1 == NULL || Number2 == NULL) return NULL;
  //Is equivalent to Number1+MultiplyByNegativeOne(Number2)  a-b == a+ (-b)
  MultiplyByNegativeOne(Number2->Mantissa); //change the sign of the second`s Mantissa
  BigFloatNumber *Rezult=SumFloat(Number1,Number2);
  MultiplyByNegativeOne(Number2->Mantissa); //reverse the change of sign of the second`s Mantissa
  CompressFloatInPlace(Rezult);
  return Rezult;
}

BigNumber* StandardSquare(BigNumber* Number) 
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

BigNumber* StandardMultiply(BigNumber* Number1, BigNumber* Number2) //O(Size(Number1)*Size(Number2))
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

BigNumber* Karatsuba(BigNumber *Number1,BigNumber *Number2)
{
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

BigNumber* KaratsubaMultiThreaded(BigNumber* Number1, BigNumber* Number2, unsigned short int NumberOfIterations);
void* KaratsubaTheadFunc(void *argument)
{
    ThreadArgumentKaratsuba *args = (ThreadArgumentKaratsuba*)argument;
    args->Result = KaratsubaMultiThreaded(args->Number1, args->Number2, args->Iterations); 
    return NULL;
}

BigNumber* KaratsubaMultiThreaded(BigNumber* Number1,BigNumber*Number2,unsigned short int NumberOfIterations) // For the 0,1,2,3... iterations of Karatsuba spawn for every recursive call a thread to compute Z0,Z1,Z3
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

BigNumber* KaratsubaSquared(BigNumber*Number)
{
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

BigNumber* KaratsubaSquaredMultiThreaded(BigNumber* Number ,unsigned short int NumberOfIterations);
void* KaratsubaSquaredTheadFunc(void *argument)
{
    ThreadArgumentKaratsubaSquared* args=( ThreadArgumentKaratsubaSquared*)argument;
    args->Result = KaratsubaSquaredMultiThreaded(args->Number, args->Iterations); 
    return NULL;
}

BigNumber*  KaratsubaSquaredMultiThreaded(BigNumber*Number,unsigned short int NumberOfIterations)
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
        BigNumber *CloneNumber2=CloneBigNumber(Number2);
        MultiplyByNegativeOne(CloneNumber2);
        return CloneNumber2;
      }
    if(IsEqual(Number1,NegativeOne)==true)
      {
         BigNumber *CloneNumber1=CloneBigNumber(Number1);
         MultiplyByNegativeOne(CloneNumber1);
         return CloneNumber1;
      }
    FreeMemory(NegativeOne);

    if(IsEqual(Number1,Number2)==true)
      {
          BigNumber *Result=KaratsubaSquaredMultiThreaded(Number1,3);
          return Result;
      }

    BigNumber *Result=KaratsubaMultiThreaded(Number1,Number2,3);
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

BigNumber* LongDivision(BigNumber* Dividend, BigNumber* Divisor,BigNumber *Remainder)  //Time Complexity O(Divident.size * Divizor.size) 
{
    BigNumber* Zero=Init("0");

    if(BigNumberCompareAbsoluteValue(Divisor,Zero)==0)  //If Divizor is 0
      {
        printf("DIVIZION BY 0, RETURNED NULL");
        free(Zero);
        return NULL;
      }

    if(BigNumberCompareAbsoluteValue(Dividend,Divisor)==-1)   //If Divizor is less than Divident than Quotint is 0 and Remainder= Divizor
       {
          return Zero;
       }
    
    bool IsDivizorNegative=Divisor->IsNegative;
    if(IsDivizorNegative)
      MultiplyByNegativeOne(Divisor); //If it is not turned into a pozitive ,Long Division diverges to +inf
      
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

    bool IsNegative=false;
    if(Dividend->IsNegative !=  IsDivizorNegative)
       IsNegative=true;
    BigNumber* FinalQuotient = PrivateConstructor(QuotientString, ActualDigits,IsNegative);
    
    if(Remainder==NULL)
      {
        FreeMemory(CurrentRemainder);
      }
    else
      {
        //If user wants to retain the Remaider we copy its values from CurentRemainder and Deallocate his memory
        Remainder->Digits=CurrentRemainder->Digits;
        Remainder->NrOfDigits=CurrentRemainder->NrOfDigits;
        Remainder->IsNegative=CurrentRemainder->IsNegative;
        free(CurrentRemainder);
      }
    
    if(IsDivizorNegative)
      {MultiplyByNegativeOne(Divisor);} //revert back to the original divisor
    
    return FinalQuotient;
}

BigFloatNumber* InverseInitialGuess(BigFloatNumber* Divisor)
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

BigNumber* Modulo(BigNumber * Dividend, BigNumber *Modulus)  // Divident mod Modulus Complexity O(N^1.58) Avg and O(1) when Modulus =0,1,2 and Dividend<Modulus
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
    if(IsEqual(Modulus,Two)==true)    //Considering a mod 2 as an edge case that can be found in O(1)
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

BigFloatNumber *DivizionSetPrecision(BigFloatNumber *Divident,BigFloatNumber *Divizor, unsigned int precision) //Precision means number of decimal digits that the user wants
{
    if(Divident==NULL || Divizor==NULL )  //Also consider divizion by 0, in the future suport for +-Infinity could be added
       return NULL;
    
    //We need to normalize the divident to have the exponent equal to the precision
    //Then Use the Divizion Algorithm for BigINT to calculate the mantissa
    //Remark : Reminder will be set to NULL
    
    long int ShiftNeededInDividentExponent=Divident->Exponent - Divizor->Exponent + precision; //calculate shift for normalization
    BigNumber *CopyDivident=CloneBigNumber(Divident->Mantissa);  //create a new BIGINT ,DOESNT ALTER DIVIDENT
    ShiftRightNPositions(CopyDivident,ShiftNeededInDividentExponent); //Normalize it

    BigFloatNumber* Quotient=malloc(sizeof(BigFloatNumber));
    if(Quotient==NULL)
      {
        perror("Allocating memory for QuotientFloat failed");
        exit(-1);
      }
    Quotient->Mantissa=LongDivision(CopyDivident,Divizor->Mantissa,NULL);  //Perform Divizion to calculate Quotient, REMAINDER SET TO NULL
    Quotient->Exponent=Divident->Exponent - Divizor->Exponent - ShiftNeededInDividentExponent;

    FreeMemory(CopyDivident);
    
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
              
              BigNumber *SquaredNumber=StandardSquare(CopyNumber);
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

    BigFloatNumber* CopyNumber=CloneBigNumberFloat(Number);
    BigFloatNumber* CopyPower=CloneBigNumberFloat(Power);
    if(Power->Exponent>=0) //If Power is an integer use Exponentiation by squaring as above with the caveat that if Power is negative do x^-n= (1/x)^n
      {
          // Implementation of a GuardBuffer in order to manage the increasing decimal digits induced by the multiplication operation
          // GuardBuffer wil be used in Exponentiation By Squaring at each itteration of the while loop 
          unsigned int GuardBuffer= Power->Mantissa->NrOfDigits + 1;  //G= floor(log_10(Power))+ 2= Power->NrOfDigits+1
          unsigned int InternalPrecision = precision + GuardBuffer;

          ShiftRightNPositions(CopyPower->Mantissa, CopyPower->Exponent);
          CopyPower->Exponent = 0;

          BigFloatNumber* Y=InitFloat("1");
          if(IsNegative(Power->Mantissa)==true) //x^-n= (1/x)^n
            {
              BigFloatNumber *DivideByOne=DivizionSetPrecision(One,Number,InternalPrecision);// Calculate 1/x with InternalPrecision
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
        //TO DO: IMPLEMENT EXPONENTIATION WHEN POWER IS IN R\Z
        FreeMemoryFloat(CopyNumber);
        FreeMemoryFloat(CopyPower);
        FreeMemoryFloat(Zero);
        FreeMemoryFloat(One);
        return NULL;
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

BigFloatNumber* SquareRootInitialGuess(BigFloatNumber* Number)
{
    if (Number == NULL || Number->Mantissa == NULL) return NULL;
    if (Number->Mantissa->IsNegative) 
    {
        perror("Mathematical Error: Cannot calculate square root of a negative number!");
        return NULL;
    }

    // 1. Calculate the true mathematical base-10 magnitude
    long int E_total = (long int)Number->Mantissa->NrOfDigits - 1 + Number->Exponent;
    
    // 2. Safely handle integer division for negative numbers to mimic mathematical floor()
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

    // 3. Extract the top 1 or 2 digits based on parity
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
    // 5. Construct the 1-digit Mantissa
    char* GuessString = malloc(2 * sizeof(char));
    if (GuessString == NULL) return NULL;
    
    GuessString[0] = RootDigit + '0';
    GuessString[1] = '\0';
    
    BigNumber* MantissaGuess = PrivateConstructor(GuessString, 1, false);

    // 6. Return the fully packaged BigFloat
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
    BigFloatNumber* Y=DivizionSetPrecision(One,InverseGuess,InternalPrecision);
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
    BigFloatNumber* Y=DivizionSetPrecision(One,InverseGuess,InternalPrecision);
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
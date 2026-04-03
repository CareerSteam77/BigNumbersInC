#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<stdbool.h>
#include<ctype.h>

#define Karatsuba_BOUND 10 //at how many digits should standard multiplication be used instead of Karatsuba

typedef struct{
    char* Digits;  //Digits are stored in reverse order for easier arithmetic operations
    unsigned int NrOfDigits;  //Numbers can have a max number of digits of 4,294,967,295 (max value of unsigned int)
    bool IsNegative; //True if the number is negative, false otherwise
}BigNumber;

typedef struct{
   BigNumber *Mantissa; //significand
   long int Exponent; 
}BigFloatNumber;  // BigFloatNumber= significand* 10^Exponent ;Exemple: 123.45= 12345*10^(-2)

//Using the Compress Function 
//Exponent < 0: It is a fraction 
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

    BigNumber *Result=Karatsuba(Number1,Number2);
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

BigNumber* FromUnsignedIntegerToBigNum(unsigned int number)
{
    //4,294,967,295 (max value of unsigned int) -10 digits needed +'\0'
    char* Digits=malloc(sizeof(char)*11);
    short int digit=0;
    short int NrOfDigits=0;
    if (number == 0) 
    {
      char* Digits=malloc(sizeof(char)*2);
      Digits[0]='0'; Digits[1]='\0';
      return PrivateConstructor(Digits, 1, false);
    }
    while(number>0)  //construcs number in reverse order directly
      {
        digit=number%10;
        Digits[NrOfDigits]=digit+'0';
        number/=10;
        NrOfDigits++;
      }
    
    Digits[NrOfDigits]='\0';
    return PrivateConstructor(Digits,NrOfDigits,false);
}

BigNumber* FromSignedIntegerToBigNum(int number)
{
    //2,147,483,648 (min value of unsigned int) -10 digits needed +'\0'
    char* Digits=malloc(sizeof(char)*11);
    short int digit=0;
    short int NrOfDigits=0;
    if (number==0) 
    {
      char* Digits=malloc(sizeof(char)*2);
      Digits[0]='0'; Digits[1]='\0';
      return PrivateConstructor(Digits, 1, false);
    }
    bool IsNegative=GetIsNegativeFromAnInt(number);
    if(IsNegative)
      number=number*(-1);
    while(number>0)  //construct number in reverse order directly
      {
        digit=number%10;
        Digits[NrOfDigits]=digit+'0';
        number/=10;
        NrOfDigits++;
      }
    
    Digits[NrOfDigits]='\0';
    return PrivateConstructor(Digits,NrOfDigits,IsNegative);
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
      {MultiplyByNegativeOne(Divisor);} //If it is not turned into a pozitive ,Long Division diverges to +inf
      
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
              
              BigNumber *SquaredNumber=Multiply(CopyNumber,CopyNumber);
              SwapNumbersInMemory(&SquaredNumber,&CopyNumber);  //x=x^2
              FreeMemory(SquaredNumber);

              BigNumber *TempPower=LongDivision(CopyPower,Two,NULL);
              SwapNumbersInMemory(&CopyPower,&TempPower);  //  n=n/2
              FreeMemory(TempPower);
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
          BigFloatNumber* Two=InitFloat("2");
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

              BigNumber *HalvedMantissa = LongDivision(CopyPower->Mantissa, Two->Mantissa, NULL);
              SwapNumbersInMemory(&(CopyPower->Mantissa), &HalvedMantissa);
              FreeMemory(HalvedMantissa);
           }

         FreeMemoryFloat(Zero);
         FreeMemoryFloat(Two);
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
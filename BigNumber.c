#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<stdbool.h>
#include<ctype.h>

typedef struct{
    char* Digits;  //Digits are stored in reverse order for easier arithmetic operations
    unsigned int NrOfDigits;  //Numbers can have a max number of digits of 4,294,967,295 (max value of unsigned int)
    bool IsNegative; //True if the number is negative, false otherwise
}BigNumber;

void CleanTrailingZeros(BigNumber* Number)  //Eliminates Unecesary 0`s from the number
{
    while (Number->NrOfDigits>1 && (Number->Digits[Number->NrOfDigits - 1] == '0'))
    {
      Number->NrOfDigits--;
    }
    Number->Digits[Number->NrOfDigits] = '\0';
}

void SwapNumbersInMemory(BigNumber **Number1,BigNumber **Number2)
{
   BigNumber *Temporal=*Number1;
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

void FreeMemory(BigNumber *Number)
{
    free(Number->Digits);
    free(Number);
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

void Increment(BigNumber* Number)
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

BigNumber* Sum(BigNumber* Number1, BigNumber* Number2) 
{
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


BigNumber* Subtract(BigNumber* Number1, BigNumber* Number2)
{
  //Is equivalent to Number1+MultiplyByNegativeOne(Number2)  a-b == a+ (-b)
  MultiplyByNegativeOne(Number2); //change the sign
  BigNumber *Rezult=Sum(Number1,Number2);
  MultiplyByNegativeOne(Number2); //reverse the change of sign
  return Rezult;
}

BigNumber* Multiply(BigNumber* Number1, BigNumber* Number2) //O(Size(Number1)+Size(Number2))
{
    char *RezultProductString=calloc(Number1->NrOfDigits+Number2->NrOfDigits+1,sizeof(char));
    unsigned int MaxPossibleDigits=Number1->NrOfDigits+Number2->NrOfDigits+1;
    bool IsNegative=false;
    if(Number1->IsNegative != Number2->IsNegative)
       IsNegative=true;

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
    BigNumber* RezultProduct=PrivateConstructor(RezultProductString,NrOfDigits,IsNegative); 

    return RezultProduct;
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

BigNumber* ShiftRightNPositions(BigNumber *Number,unsigned int N) //For multiplication by positive integer powers of 10 and for Long Division
{
   if(N<=0) return Number; //no change needed

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

   return Number;
}

BigNumber* LongDivision(BigNumber* Dividend, BigNumber* Divisor,BigNumber *Remainder)  //Time Complexity O(Divident.size+Divizor.size) ,same as Multiplicity
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
        CurrentRemainder=ShiftRightNPositions(CurrentRemainder,1);
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

BigNumber* Factorial(unsigned int Number)
{
    BigNumber* FactorialRezult=Init("1");
    unsigned int index=2;
    for(index=2;index<=Number;index++)
      {
         FactorialRezult=Multiply(FactorialRezult,FromUnsignedIntegerToBigNum(index));
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
    if(Number->IsNegative==true)
      printf("-");
    for(unsigned int index=0;index<Number->NrOfDigits;index++) //as it is stored in reverse we need to display in reverse
      printf("%c",Number->Digits[Number->NrOfDigits-index-1]);
    printf(" ");
}

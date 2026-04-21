#include "BigNumber.h"
#include <stdlib.h>
#include <stdio.h> 
#include <time.h> //used for rand()

BigNumber *Constructor(char *Digits,unsigned int NrOfDigits,bool IsNegative)  //Construct a BigNumber WITHOUT reversing the string, used in Arithemic operations
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

BigNumber* PrecomputeBarrettMu(BigNumber* M)
{
  if(M==NULL) return NULL;

  //Mu=floor(10^2k/M) where k=NumberOfDecimalDigits in M
  unsigned int k=M->NrOfDigits;
  unsigned int ZerosCount =2*k;

  char *PowerOfTenString=malloc(sizeof(char)*(ZerosCount+1+1)); //2k zerous + '1' +string terminator
  if(PowerOfTenString==NULL)
    {
      perror("Allocating memory for Barrett Constant failed");
      exit(-1);
    }

  for(unsigned int index=0;index<ZerosCount;index++) // "00...00001" BigNumber stores digits in reverse
    PowerOfTenString[index]='0';
  PowerOfTenString[ZerosCount]='1';
  PowerOfTenString[ZerosCount+1]='\0';

  BigNumber* TenPower=Constructor(PowerOfTenString,ZerosCount+1,false);
  BigNumber* Mu=LongDivision(TenPower,M,NULL);

  FreeMemory(TenPower);

  return Mu;
}

BigNumber* BarrettModulo(BigNumber* Number,BigNumber* Modulus,BigNumber *Mu) //Optimization for ModularExponentiation, Barrett only works when Base < Modulus 
{
    if(Number == NULL || Modulus == NULL || Mu == NULL) return NULL;

    //If Number> 10^2k Berrett is not suitable, using another algoritm for Modulo
    if (Number->NrOfDigits > 2 * Modulus->NrOfDigits)
       return Modulo(Number,Modulus);

    if (BigNumberCompareAbsoluteValue(Number, Modulus) == -1)  //if Number<Modulus return Copy of Number
    {
      return CloneBigNumber(Number);
    }

    unsigned int k = Modulus->NrOfDigits;
    BigNumber* TempQuotient=Multiply(Number,Mu);
    DividByPowerOf10(TempQuotient,2*k);

    BigNumber* QTimesM=Multiply(TempQuotient,Modulus);
    BigNumber* Remainder=Subtract(Number,QTimesM);

    //Corection Step (Maximum 2 times)
    while(IsNegative(Remainder)==false && BigNumberCompareAbsoluteValue(Remainder,Modulus)>=0)
      {
        BigNumber* Temp=Subtract(Remainder,Modulus);
        FreeMemory(Remainder);Remainder=Temp;
      }
    
    FreeMemory(TempQuotient);
    FreeMemory(QTimesM);

    return Remainder;
    
}

BigNumber* ModularExponetiation(BigNumber *Base,BigNumber *Exponent,BigNumber *Modulus) //Calculates (Base^Exponent) mod Modulus in an efficient way
{
    if(Base==NULL || Exponent==NULL || Modulus== NULL) return NULL;

    //Main Lemma used : (ab) mod m = [(a mod m)*(b mod m)](mod m)
    //Then applying the classic Exponentiation by squaring and the above lemma and Barrett Reduction for Modulus yield the result

    BigNumber *One=Init("1");
    BigNumber *Zero=Init("0");
    if(BigNumberCompare(Modulus,One)==0)  //a mod 1 ==0
      {
         FreeMemory(One);
         return Zero;
      }
    
    BigNumber* Mu = PrecomputeBarrettMu(Modulus); //Precomputing Barrett Constants
    BigNumber* NewResult=Init("1");  //Result is initially 1
    BigNumber* CopyExponent=CloneBigNumber(Exponent); //Clone so the initial pointer doesnt get alterred

    BigNumber* NewBase=BarrettModulo(Base,Modulus,Mu);                            //Base=Base mod Modulus
    while(BigNumberCompare(CopyExponent,Zero)==1)                       //Exponent >0
      {
        if(IsOdd(CopyExponent))                                         //If exponent mod 2==0 then
          {                                                             //Result=(Result*Base) mod Modulus
            BigNumber* Product=Multiply(NewResult,NewBase);            
            FreeMemory(NewResult);NewResult=BarrettModulo(Product,Modulus,Mu);
            FreeMemory(Product);
          }
        
        DivizionBy2(CopyExponent);                                      //Exponent=Exponent/2
        if(IsEqual(CopyExponent, Zero)==true)                           //Early stop is exponent gets to 0
            break;

        BigNumber* BaseSquared=Multiply(NewBase,NewBase);
        FreeMemory(NewBase);NewBase=BarrettModulo(BaseSquared,Modulus,Mu);         //Base=Base^2 mod Modulus
        FreeMemory(BaseSquared);
      }
    
    FreeMemory(Mu);
    FreeMemory(CopyExponent);
    FreeMemory(NewBase);
    FreeMemory(Zero);
    FreeMemory(One);

    return NewResult;
}

BigNumber *GCD(BigNumber *A, BigNumber *B) //Implementation of Stein`s BinaryGCD algorithm 
{
    if(A==NULL || B==NULL) return NULL;

    BigNumber* Zero = Init("0");
    if(IsEqual(A, Zero)==true) return Zero;
    if(IsEqual(B, Zero)==true) return Zero;

    BigNumber* u = CloneBigNumber(A);
    BigNumber* v = CloneBigNumber(B);
    unsigned int commonPowerOfTwoFactor = 0;

    //Extract common factors of 2
    while (IsEven(u) && IsEven(v)) {
        DivizionBy2(u);
        DivizionBy2(v);
        commonPowerOfTwoFactor++;
    }

    //At least one of them is Odd now
    // We will keep 'u' as the working even number (if any) and 'v' as the odd one.
    while (IsEqual(u, Zero) == false) 
    {
        //u is even, v is odd (we established v is odd or will be made odd)
        while (IsEven(u)) {
            DivizionBy2(u);
        }

        //v is even
        while (IsEven(v)) {
            DivizionBy2(v);
        }

        //Both are Odd
        // u = |u - v| / 2; v = min(u, v)
        if (BigNumberCompareAbsoluteValue(u, v) >= 0) 
        {
            BigNumber* diff = Subtract(u, v);
            FreeMemory(u);
            u = diff;
            DivizionBy2(u);
        } 
        else 
        {
            BigNumber* diff = Subtract(v, u);
            FreeMemory(v);
            v = diff;
            DivizionBy2(v);
            SwapNumbersInMemory(&u, &v); // Swap so v remains the strictly odd minimum
        }
    }

    // Reapply the common factor of 2: Result=v * 2^commonTwos
    for (unsigned int i = 0; i < commonPowerOfTwoFactor; i++) 
    {
        MultiplyBy2(v);
    }

    FreeMemory(u);
    FreeMemory(Zero);
    
    return v;
}

BigNumber* ExtendedEuclidean(BigNumber *A, BigNumber *B, BigNumber **X, BigNumber **Y)
{   
    if(X==NULL && Y==NULL)
      {
        return GCD(A,B); 
      }

    if(A==NULL || B==NULL) return NULL;

    BigNumber* Zero =Init("0");
    BigNumber* R0   =CloneBigNumber(A);
    BigNumber* R1   =CloneBigNumber(B);

    BigNumber* X0 = Init("1"); BigNumber* X1 = Init("0");
    BigNumber* Y0 = Init("0"); BigNumber* Y1 = Init("1");

    while(IsEqual(R1,Zero)==false)
      {
        //Calculate new Quotient and Remainder
        BigNumber *RNew=Init("0");
        BigNumber* Q = LongDivision(R0, R1, RNew);

        //Calculate x_new = x0 - q * x1
        BigNumber* QTimesX1 = Multiply(Q, X1);
        BigNumber* XNew = Subtract(X0, QTimesX1);
        FreeMemory(QTimesX1);

        //Calculate y_new = y0 - q * y1
        BigNumber* QTimesY1 = Multiply(Q, Y1);
        BigNumber* YNew = Subtract(Y0, QTimesY1);
        FreeMemory(QTimesY1);

        //Update R,X,Y
        FreeMemory(R0); 
        R0 = R1; 
        R1 = RNew;

        FreeMemory(X0); 
        X0 = X1; 
        X1 = XNew;

        FreeMemory(Y0); 
        Y0 = Y1; 
        Y1 = YNew;
      }

    FreeMemory(Zero);
    FreeMemory(R1); 
    FreeMemory(X1); 
    FreeMemory(Y1);

    //Assign Bezout Coefficients
    if(X!=NULL)
        {*X=X0;}
    else  
       {FreeMemory(X0);}
    
    if(Y!=NULL)
        {*Y=Y0;}
    else  
       {FreeMemory(Y0);}
    
    return R0;
}

BigNumber *LCM(BigNumber *A,BigNumber *B)
{
    if(A==NULL || B==NULL)  return NULL;

    //Using the formula lcm(A,B) = |A*B|/gcd(A,B) =|A| * (|B|/gdc(A,B)) 
    // |A| * (|B|/gdc(A,B)) reduces the size of the numbers

    BigNumber *GreatestCommonDivizor=GCD(A,B);
    BigNumber *CopyA=CloneBigNumber(A);
    BigNumber *CopyB=CloneBigNumber(B);
    if(IsNegative(CopyA)==true) MultiplyByNegativeOne(CopyA);
    if(IsNegative(CopyB)==true) MultiplyByNegativeOne(CopyB);

    BigNumber *BDividedByGCD=LongDivision(CopyB,GreatestCommonDivizor,NULL);
    BigNumber *Result=Multiply(BDividedByGCD,CopyA);

    FreeMemory(CopyA);
    FreeMemory(CopyB);
    FreeMemory(GreatestCommonDivizor);
    FreeMemory(BDividedByGCD);

    return Result;

}

BigNumber *GenerateRandomPositiveBigNumber(unsigned int NrOfDigits) //Generates a Random Positive BigNumber of NrOfDigits digits , if NrOfDigits is <=0 return NULL
{   
    static bool is_seeded = false; //NOT THREAD SAFE
    if (!is_seeded) 
    {
        srand((unsigned int)time(NULL));
        is_seeded = true;
    }

    if(NrOfDigits<=0)
      return NULL;
    
    char *Digits=malloc(sizeof(char)*(NrOfDigits+1));
    if(Digits==NULL)
      {
        perror("Allocating Storage for Digits in GenerateRandomBigNumber failed\n");
        exit(-1);
      }
    
    Digits[0]=(rand() % 9) + 1 + '0';
    for(unsigned int index=1;index<NrOfDigits;index++)
      {
        Digits[index]=(rand()%10)+'0';
      }
    Digits[NrOfDigits]='\0';

    BigNumber *Random=Init(Digits);
    free(Digits);

    return Random;

}

bool MillerRabin(BigNumber *Number,unsigned int NrOfBases) //Probabilistic primality test ,return true if prime, false if composite
{
    if(Number==NULL)    return false;
    if(NrOfBases<=0)    return false;

    BigNumber *Two = Init("2");
    if (IsEqual(Number, Two)==true) 
      {
        FreeMemory(Two);
        return true;
      }

    if(IsEven(Number))  return false;
    
    BigNumber *One=Init("1");
    if(BigNumberCompare(Number,One)==-1) 
      {
        FreeMemory(One);
        return false;
      }
    
    BigNumber *NumberDecremented=Subtract(Number,One);
    BigNumber *D=CloneBigNumber(NumberDecremented);
    unsigned int S=0;
    while(IsEven(D))
      {
        S++;
        DivizionBy2(D);
      }
    for(int i=0;i<NrOfBases;i++)
     {
        BigNumber *A=GenerateRandomPositiveBigNumber(Number->NrOfDigits-1);
        BigNumber *X=ModularExponetiation(A,D,Number);
        for(int j=0;j<S;j++)
          {
            BigNumber* XSquared=Multiply(X,X);
            BigNumber* Y=Modulo(XSquared,Number);
            if(IsEqual(Y,One)==true && IsEqual(X,One)==false && IsEqual(X,NumberDecremented)==false)
                {
                  FreeMemory(A);
                  FreeMemory(X);
                  FreeMemory(XSquared);
                  FreeMemory(Y);
                  FreeMemory(One);
                  FreeMemory(D);
                  FreeMemory(NumberDecremented);
                  return false; //Number is Composite
                }
            
            FreeMemory(XSquared);
            FreeMemory(X);X=Y;
          }
        
        if(IsEqual(X,One)==false)
         {
            FreeMemory(A);
            FreeMemory(X);
            FreeMemory(One);
            FreeMemory(D);
            FreeMemory(NumberDecremented);
            return false; // Number is Composite
         }

        FreeMemory(A);
        FreeMemory(X);
     }

    FreeMemory(One);
    FreeMemory(D);
    FreeMemory(NumberDecremented);

    return true; //Number is PROBABLY Prime with Probability 1-4^(-NrOfBases)
    
}

BigNumber* GenerateRandomTrialNumberForRSA(unsigned int NrOfDigits)
{   
    static bool is_seeded = false; //NOT THREAD SAFE
    if (!is_seeded) 
    {
        srand((unsigned int)time(NULL));
        is_seeded = true;
    }

    if(NrOfDigits<=0)
      return NULL;
    
    char *Digits=malloc(sizeof(char)*(NrOfDigits+1));
    if(Digits==NULL)
      {
        perror("Allocating Storage for Digits in GenerateRandomBigNumber failed\n");
        exit(-1);
      }
    
    Digits[0]=(rand() % 9) + 1 + '0';                                         //first digit [1,9]
    const char OddDigits[] = "13579";                             
    Digits[NrOfDigits-1] = OddDigits[rand() % 5];                             //last digit [1,3,5,7,9]

    for(unsigned int index=1;index<NrOfDigits-1;index++)
      {
        Digits[index]=(rand()%10)+'0';
      }
    Digits[NrOfDigits]='\0';

    BigNumber *Random=Init(Digits);
    free(Digits);

    return Random;

}

unsigned int ModuloSmallInt(BigNumber* Number, unsigned int divisor) // A fast modulo calculator for integer divizors O(N) time O(1) memory
{
    if (Number == NULL || divisor == 0) return 0;

    unsigned long long int remainder = 0; 
    for (long int i = Number->NrOfDigits - 1; i >= 0; i--) 
    {
        unsigned int current_digit = Number->Digits[i] - '0';
        remainder = (remainder * 10 + current_digit) % divisor;
    }

    return (unsigned int)remainder;
}

BigNumber *GeneratePrime(unsigned int NrOfDigits)  //Generates a Prime Number with NrOfDigits digits
{
    if(NrOfDigits<=0)
      return NULL;
    
    //Function generates random odd BigNumbers then uses a filter to quickly check modulo with first (size_of_list) primes then sending the number into MillerRabin with 20 Bases
    //Thus generating a Number that has P=1-4^(-Bases) (99.999904632 chance) of being an actual prime number

    //A list of the first (100) prime numbers that filters about ~87% of guesses
    //Question: What is the perfect size of the list?
    const unsigned int PrimeArray[]= {3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,103,107,109,113,
                                      127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,229,233,239,241,
                                      251,257,263,269,271,277,281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,
                                      389,397,401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,491,499,503,509,521,523,
                                      541,547};
    unsigned int size=sizeof(PrimeArray)/sizeof(unsigned int);

    bool FoundAPrime=false; 
    BigNumber* PrimeNumber=NULL;
    do
    {
      BigNumber* PotentialPrimeNumber=GenerateRandomTrialNumberForRSA(NrOfDigits);

      //Filter
      bool HasPassedTheFilter=true;
      for(unsigned int index=0;index<size;index++)
        {
          unsigned int TestPrime=PrimeArray[index];
          if(ModuloSmallInt(PotentialPrimeNumber,TestPrime)==0) 
            {
                HasPassedTheFilter=false;
                break; 
            }
        }
      
      if(HasPassedTheFilter==true)
        {
            //Using MillerRabin after the filter 
            bool IsPrime=MillerRabin(PotentialPrimeNumber,10);
            if(IsPrime==true)
              {
                PrimeNumber=PotentialPrimeNumber;
                FoundAPrime=true;
              }
            else
              {
                FreeMemory(PotentialPrimeNumber); 
              }
        }

    } while(FoundAPrime==false);

  return PrimeNumber;
}
#include "BigNumber.h"
#include <stdlib.h>
#include <stdio.h> 
#include <time.h> //used for rand()

#define MAX_UINT128 (~((__uint128_t)0))

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
  BigNumber* Mu=Division(TenPower,M,NULL);

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
        BigNumber *RNew;
        BigNumber* Q = Division(R0, R1, &RNew);

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

    BigNumber *BDividedByGCD=Division(CopyB,GreatestCommonDivizor,NULL);
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
    FreeMemory(Two);

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
        BigNumber *A=GenerateRandomPositiveBigNumber(Number->NrOfDigits-2);
        BigNumber *X=ModularExponetiation(A,D,Number);
        FreeMemory(A);

        if (IsEqual(X, One) == true || IsEqual(X, NumberDecremented) == true)
        {
            FreeMemory(X);
            continue; // Try next base.
        }

        bool passed_this_base = false;

        for(unsigned int j = 1; j < S; j++)
        {
            BigNumber* XSquared = Multiply(X, X);
            BigNumber* Y = Modulo(XSquared, Number);

            FreeMemory(XSquared);
            FreeMemory(X);
            X = Y;

            if(IsEqual(X, NumberDecremented) == true)
            {
                passed_this_base = true;
                break;
            }
            
            if(IsEqual(X, One) == true)
            {
                passed_this_base = false;
                break;
            }
        }

        FreeMemory(X);

        if(passed_this_base == false)
        {
            FreeMemory(One);
            FreeMemory(D);
            FreeMemory(NumberDecremented);
            return false; 
        }
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

unsigned int ModuloUnsignedInt(BigNumber* Number, unsigned int Divisor) // A fast modulo calculator for unsigned integer divizors O(N) time O(1) memory
{
    if (Number == NULL || Divisor == 0) return 0;

    unsigned long long int remainder = 0; 
    for (long int i = Number->NrOfDigits - 1; i >= 0; i--) 
    {
        unsigned int current_digit = Number->Digits[i] - '0';
        remainder = (remainder * 10 + current_digit) % Divisor;
    }

    return (unsigned int)remainder;
}

__uint128_t Modulo128Bit(BigNumber* Number, __uint128_t Divisor)  // A fast modulo calculator for 128 bit integer divizors O(N) time O(1) memory
{
    if (Number == NULL || Divisor == 0) return 0;

    //Divisor > MAX_UINT128 / 10 An overflow cant happen if this condition is NOT met
    //We are forcing this condition in Miller Rabin

    __uint128_t remainder = 0; 
    for (long int i = Number->NrOfDigits - 1; i >= 0; i--) 
    {
        unsigned int current_digit = Number->Digits[i] - '0';
        remainder = (remainder * 10 + current_digit) % Divisor;
    }

    return remainder;
}


BigNumber *GeneratePrime(unsigned int NrOfDigits)  //Generates a Prime Number with NrOfDigits digits
{
    if(NrOfDigits<=0)
      return NULL;
    
    //Function generates random odd BigNumbers then uses a filter to quickly check modulo with first (size_of_list) primes then sending the number into MillerRabin with 20 Bases
    //Thus generating a Number that has P=1-4^(-Bases) (99.999904632 chance) of being an actual prime number

    //A list of the first (468) prime numbers that filters about ~94% of guesses
    //Question: What is the perfect size of the list?

    const unsigned int PrimeArray[] = {
    3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 
    101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 
    211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 
    307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 
    401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 
    503, 509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 
    601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 
    701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797, 
    809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 
    907, 911, 919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997, 
    1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069, 1087, 1091, 1093, 1097, 
    1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163, 1171, 1181, 1187, 1193, 
    1201, 1213, 1217, 1223, 1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291, 1297, 
    1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373, 1381, 1399, 
    1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 
    1511, 1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, 1597, 
    1601, 1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657, 1663, 1667, 1669, 1693, 1697, 1699, 
    1709, 1721, 1723, 1733, 1741, 1747, 1753, 1759, 1777, 1783, 1787, 1789, 
    1801, 1811, 1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, 1879, 1889, 
    1901, 1907, 1913, 1931, 1933, 1949, 1951, 1973, 1979, 1987, 1993, 1997, 1999, 
    2003, 2011, 2017, 2027, 2029, 2039, 2053, 2063, 2069, 2081, 2083, 2087, 2089, 2099, 
    2111, 2113, 2129, 2131, 2137, 2141, 2143, 2153, 2161, 2179, 2203, 2207, 2213, 2221, 2237, 2239, 2243, 2251, 2267, 2269, 2273, 2281, 2287, 2293, 2297, 
    2309, 2311, 2333, 2339, 2341, 2347, 2351, 2357, 2371, 2377, 2381, 2383, 2389, 2393, 2399, 
    2411, 2417, 2423, 2437, 2441, 2447, 2459, 2467, 2473, 2477, 2503, 2521, 2531, 2539, 2543, 2549, 2551, 2557, 2579, 2591, 2593, 
    2609, 2617, 2621, 2633, 2647, 2657, 2659, 2663, 2671, 2677, 2683, 2687, 2689, 2693, 2699, 
    2707, 2711, 2713, 2719, 2729, 2731, 2741, 2749, 2753, 2767, 2777, 2789, 2791, 2797, 
    2801, 2803, 2819, 2833, 2837, 2843, 2851, 2857, 2861, 2879, 2887, 2897, 
    2903, 2909, 2917, 2927, 2939, 2953, 2957, 2963, 2969, 2971, 2999, 
    3001, 3011, 3019, 3023, 3037, 3041, 3049, 3061, 3067, 3079, 3083, 3089, 
    3109, 3119, 3121, 3137, 3163, 3167, 3169, 3181, 3187, 3191, 
    3203, 3209, 3217, 3221, 3229, 3251, 3253, 3257, 3259, 3271, 3299, 
    3301, 3307, 3313, 3319, 3323, 3329, 3331, 3343, 3347, 3359, 3361, 3371, 3373, 3389, 3391, 
    3407, 3413, 3433, 3449, 3457, 3461, 3463, 3467, 3469, 3491, 3499
   };
    unsigned int size=sizeof(PrimeArray)/sizeof(unsigned int);

    bool FoundAPrime=false; 
    BigNumber* PrimeNumber=NULL;
    do
    {
      BigNumber* PotentialPrimeNumber=GenerateRandomTrialNumberForRSA(NrOfDigits);

      //Filter
      bool HasPassedTheFilter=true;
      unsigned int index = 0;
      
      //128-BIT GMP Wheel Grouping Filter
      
      //Math Proof:
      //ProductOfFirstNPrimes = 2*3*4*5*....n ,GroupRemainder= N mod ProductOfFirstNPrimes
      //By Euclid Theorem N=ProductOfFirstNPrimes * scalar + GroupRemainder 
      //Consider p a prime number contained in ProductOfFirstNPrimes then N mod p = ProductOfFirstNPrimes * scalar + GroupRemainder (mod p) =  GroupRemainder (mod p)
      //Finally for all p in ProductOfFirstNPrimes N mod p =GroupRemainder mod p 
      //QED

      __uint128_t SafeLimit128 = MAX_UINT128 / 10; 

      while (index < size && HasPassedTheFilter)
        {
            __uint128_t GroupProduct = 1;
            unsigned int StartIndex = index;
            
            // Add primes to the product until limit is reached
            while (index < size)
            {
                // Break if limit is reached
                if (GroupProduct > SafeLimit128 / PrimeArray[index]) 
                {
                    break;
                }
                GroupProduct *= PrimeArray[index];
                index++;
            }
            
            //Calculate the Remainder in O(n) then find if the rest for each prime in array in O(1)
            __uint128_t GroupRemainder = Modulo128Bit(PotentialPrimeNumber, GroupProduct);
            
            for (unsigned int p = StartIndex; p < index; p++)
            {
                if (GroupRemainder % PrimeArray[p] == 0)
                {
                    HasPassedTheFilter = false;
                    break;
                }
            }
        }
      
      if(HasPassedTheFilter==true)
        {
            //Using MillerRabin after the filter (around 6% of trials go here)

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
      else
        {
           FreeMemory(PotentialPrimeNumber);
        }

    } while(FoundAPrime==false);

  return PrimeNumber;
}
